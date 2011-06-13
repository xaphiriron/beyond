#include "map.h"

#include <SDL/SDL_opengl.h>

#include <stdio.h>
#include "xph_memory.h"
#include "xph_log.h"

#include "hex_utility.h"

struct hexSubdivided
{
	enum hexOrSubdivType
		type;
	unsigned char
		span;
	SUBHEX
		parent;	// the subdiv this subdiv is a part of (with span + 1), or NULL if this subdiv is a pole
	signed int
		x, y;	// the x,y offset of this subdiv within its parent, or 0,0 if this subdiv is a pole
	/* data is hex iff span == 1; data is subs iff span > 1; in either case
	 * the value may be NULL if the world hasn't been generated down to that
	 * level yet. furthermore if it's subs it's possible for only some of the
	 * subdivs to be non-NULL.
	 * In either case, if data isn't NULL to begin with it stores hx (mapRadius + 1) values.
     */
	SUBHEX
		* data;

	/* actual map data goes here */
};

struct hexTile
{
	enum hexOrSubdivType
		type;
	SUBHEX
		parent;
	signed int
		x, y;

	/* actual hex data goes here */
	unsigned int
		centre,
		edgeBase[12];
	unsigned char
		corners[3];
	unsigned char
		light;
};

union hexOrSubdiv		// SUBHEX is union hexOrSubdiv *
{
	enum hexOrSubdivType
		type;
	struct hexSubdivided
		sub;
	struct hexTile
		hex;
};

struct hexWorldPosition // WORLDHEX
{
	SUBHEX
		subhex;
	signed int
		* x,
		* y;
	char
		pole;
	unsigned char
		spanDepth;
};

struct hexRelativePosition // RELATIVEHEX
{
	/* both these values have (maxSpan - minSpan) + 1 indices allocated
	 */
	signed int
		* x,
		* y;
	unsigned char
		minSpan,
		maxSpan;
	VECTOR3
		distance;	// TODO: blah blah but consider the overflow
	/* TODO: i suspect either of these subhexes could easily become dangling pointers if a hexRelativePosition struct is kept for more than one tick, since they could be unloaded any time the subhex unloading code is called (this creates the question of why these values would be calculated to begin with, so... think about that for a while)
	 * - xph 2011-05-27
	 */
	SUBHEX
		origin,		// i.e., from origin x,y steps away is the subhex target
		target;
};

static SUBHEX
	* Poles = NULL;
static char
	* PoleNames = NULL;
static int
	PoleCount = 0;

static unsigned char
	mapRadius = 8,
	mapSpan = 4;

static bool coordinatesOverflow (const signed int x, const signed int y, const signed int * const coords);

static void subhexInitData (SUBHEX subhex);
static void subhexSetData (SUBHEX subhex, signed int x, signed int y, SUBHEX child);

bool mapSetSpanAndRadius (const unsigned char span, const unsigned char radius)
{
	if (Poles != NULL || PoleNames != NULL)
	{
		ERROR ("Can't switch map size while there's a map loaded; keeping old values of %d/%d (instead of new values of %d/%d)", mapSpan, mapRadius, span, radius);
		return FALSE;
	}
	// FIXME: add a check to make sure radius is a power of two, since things will break if it's not
	mapRadius = radius;
	mapSpan = span;
	return TRUE;
}

bool mapGeneratePoles (const enum mapPoleTypes type)
{
	int
		count = type;
	// blah blah warn/error here if the pole values aren't already null
	Poles = NULL;
	PoleNames = NULL;
	switch (type)
	{
		case POLE_TRI:
			PoleNames = xph_alloc (count + 1);
			strcpy (PoleNames, "rgb");
			break;
		default:
			ERROR ("yeah pole count value of %d isn't supported, sry", type);
			return FALSE;
	}
	PoleCount = count;
	Poles = xph_alloc (sizeof (SUBHEX) * count);
	while (count > 0)
	{
		count--;
		Poles[count] = mapSubdivCreate (NULL, 0, 0);
	}
	
	return FALSE;
}

bool worldExists ()
{
	if (Poles == NULL || PoleNames == NULL)
		return FALSE;
	return TRUE;
}

void worldDestroy ()
{
	int
		 i = PoleCount;
	while (i > 0)
	{
		i--;
		subhexDestroy (Poles[i]);
	}
	xph_free (Poles);
	xph_free (PoleNames);
	Poles = NULL;
	PoleNames = NULL;
	PoleCount = 0;
}


void mapForceSubdivide (SUBHEX subhex)
{
	signed int
		x = 0,
		y = 0;
	unsigned int
		r = 0,
		k = 0,
		i = 0;
	SUBHEX (*func)(const SUBHEX, signed int, signed int) = NULL;
	if (subhex->type != HS_SUB)
		return;
	if (subhex->sub.data == NULL)
		subhexInitData (subhex);
	func = subhex->sub.span == 1
		? mapHexCreate
		: mapSubdivCreate;
	while (r <= mapRadius)
	{
		hex_rki2xy (r, k, i, &x, &y);
		/* DEBUG ("radius: %d; xy: %d,%d; rki: %d %d %d; offset: %d; [%d]: &%p/%p", mapRadius, x, y, r, k, i, offset, offset, &subhex->sub.data[offset], subhex->sub.data[offset]); */
		hex_nextValidCoord (&r, &k, &i);
		if (subhexData (subhex, x, y) == NULL)
			func (subhex, x, y);
	}
}

void mapForceGrowAtLevelForDistance (SUBHEX subhex, unsigned char spanLevel, unsigned int distance)
{
	SUBHEX
		centre = subhex,
		newSub;
	RELATIVEHEX
		rel;
	unsigned char
		currentSpan;
	unsigned int
		r = 1,
		k = 0,
		i = 0,
		offset = 0;
	signed int
		x, y;
	if (spanLevel > mapSpan)
	{
		ERROR ("Got meaningless request to grow above the max span level; discarding. (max: %d; request: %d)", mapSpan, spanLevel);
		return;
	}
	currentSpan = subhexSpanLevel (centre);
	while (currentSpan < spanLevel)
	{
		centre = subhexParent (centre);
		if (centre == NULL)
		{
			ERROR ("Unexpectedly hit pole layer (or broken map) at span %d, when it should only occur at %d.", currentSpan, mapSpan);
			exit (2);
		}
		currentSpan++;
	}

	while (currentSpan > spanLevel)
	{
		if (centre->sub.data == NULL)
			subhexInitData (centre);
		if (subhexData (centre, 0, 0) == NULL)
			mapSubdivCreate (centre, 0, 0);
		centre = subhexData (centre, 0, 0);
		currentSpan--;
	}

	subhexLocalCoordinates (centre, &x, &y);
	DEBUG ("our centre value is at span level %d, at local coordinates %d, %d (and our target span is %d)", subhexSpanLevel (centre), x, y, spanLevel);

	/* if we've gotten here, {centre} is at the right span level (i.e.,
	 * spanLevel) and we're ready to start attempting to grow outwards.
	 * things could be in several states here:
	 * - if we had to change the span level, the centre value will definitely
	 *   be at local 0,0
	 * - if we /didn't/ have to change the span level, the centre value could
	 *   be at local anywhere
	 * - if (distance + the local coordinate magnitude) > mapRadius we have to
	 *   deal with traversing across other subdivs, but if the local magnitude
	 *   isn't 0 (see above) then we may not have to be eqilateral in our
	 *   traversal.
	 * and as usual the subdiv traversal has to be recursive or else things
	 * will break all over but most especially near the pole edges. the growth
	 * also might have to affect higher span levels; if we have to traverse to
	 * other subdivs but they don't exist yet, we'll have to create them.
	 *  - xph 2011-05-21
	 */

	while (r <= distance)
	{
		hex_rki2xy (r, k, i, &x, &y);
		DEBUG ("generating relative hex from centre at offset %d, %d", x, y);
		rel = mapRelativeSubhexWithCoordinateOffset (centre, 0, x, y);
		newSub = mapRelativeTarget (rel);
		DEBUG ("target has span %d", subhexSpanLevel (newSub));
		/* it would be really nice to have a magical function here called "recursiveAddMapRelativeMarkerOntoLoadQueue" that would recursively add the map relative marker struct onto a load queue. it'd take the exact coordinate offset stored internally in the RELATIVEHEX struct and compare that to the subhex best-match target stored internally in the RELATIVEHEX struct, and if they don't match up (i.e., the target doesn't go down to the same span level as the coordinate offset since it's not loaded or generated yet; this is trivial to calculate since it's just target->span == rel->minSpan) then it will load/gen the next step in the chain, recursively creating new subhexes from the top down until it finishes. these subhexes-to-create would be pushed onto a priority queue, and the priority would depend on 1. the number of subhexes that push the same coordinate (i.e., many to-be-loaded subhexes that are all within the same unloaded subhex one span level up would all add the same value) and 2. the coordinate distance from the player, and it would keep adding values as parents are loaded up in the background without needing to be re-called (in practice it would add all the entries at once but the priority queue would be set up so the priority of a parent was strictly greater than the priority of any of its kids, probably by weighing the loader vastly towards subhexes with larger span)
		 *
		 * in the mean time we do nothing
		 */

		/* the 'force' part of the function name: let's just create a blank subdiv and hope for the best. definitely take this out soon.
		 *  - xph 2011-05-30 */
		//WARNING ("%s: attempting dangerous altering of a relative hex struct (%p).", __FUNCTION__, rel);
		while (!isPerfectFidelity (rel))
		{
			currentSpan = subhexSpanLevel (rel->target);
			offset = currentSpan - rel->minSpan - 1;
			DEBUG ("trying offset at span %d (i: %d), which is %d, %d", currentSpan, offset, rel->x[offset], rel->y[offset]);
			// this is wrong if the current span level is 1; it should be maphexcreate instead
			newSub = mapSubdivCreate (rel->target, rel->x[offset], rel->y[offset]);
			assert (newSub != NULL);
			rel->target = newSub;
		}
		DEBUG ("Done at %d, %d", x, y);

		mapRelativeDestroy (rel);
		hex_nextValidCoord (&r, &k, &i);
	}
}

/***
 * SIMPLE CREATION AND INITIALIZATION FUNCTIONS
 */

SUBHEX mapHexCreate (const SUBHEX parent, signed int x, signed int y)
{
	SUBHEX
		sh;
	unsigned int
		r, k, i;
	if (hexMagnitude (x, y) > mapRadius)
	{
		ERROR ("Can't create subhex child: coordinates given were %d, %d, which are %d step%s out of bounds.", x, y, hexMagnitude (x, y) - mapRadius, hexMagnitude (x, y) - mapRadius == 1 ? "" : "s");
		return NULL;
	}
	sh = xph_alloc (sizeof (union hexOrSubdiv));
	sh->type = HS_HEX;
	sh->hex.parent = parent;
	if (parent == NULL)
		WARNING ("Hex generated without a parent; this is probably a mistake. (It's got a memory address of %p)", sh);
	else
		subhexSetData (parent, x, y, sh);
	sh->hex.x = x;
	sh->hex.y = y;
	hex_xy2rki (x, y, &r, &k, &i);
	sh->hex.light = (k % 2) << 0 | (r % 2 ^ k % 2) << 1 | (!i && r) << 2;

	return sh;
}

SUBHEX mapSubdivCreate (SUBHEX parent, signed int x, signed int y)
{
	SUBHEX
		sh;
	if (hexMagnitude (x, y) > mapRadius)
	{
		ERROR ("Can't create subhex child: coordinates given were %d, %d, which are %d step%s out of bounds.", x, y, hexMagnitude (x, y) - mapRadius, hexMagnitude (x, y) - mapRadius == 1 ? "" : "s");
		return NULL;
	}
	sh = xph_alloc (sizeof (union hexOrSubdiv));
	sh->type = HS_SUB;
	sh->sub.parent = parent;
	sh->sub.x = x;
	sh->sub.y = y;
	sh->sub.data = NULL;
	if (parent != NULL && parent->type != HS_SUB)
	{
		ERROR ("Given invalid parent (%p); cannot use a hex as a parent of a subdiv!", parent);
		parent = NULL;
	}

	//INFO ("Creating subdiv %p @ %p, %d, %d", sh, parent, x, y);

	if (parent != NULL)
	{
		sh->sub.span = parent->sub.span - 1;
		subhexSetData (parent, x, y, sh);
	}
	else
		sh->sub.span = mapSpan;

	// a span-one subdiv is REQUIRED to have all its hexes generated
	if (sh->sub.span == 1)
		mapForceSubdivide (sh);

	return sh;
}


static void subhexInitData (SUBHEX subhex)
{
	assert (subhex != NULL);
	assert (subhex->type == HS_HEX || subhex->type == HS_SUB);
	if (subhex->type == HS_HEX)
		return;
	/* FIXME: see note in hex_utility.c about hx (); long story short these lines can drop the + 1 once that's fixed */
	subhex->sub.data = xph_alloc (sizeof (SUBHEX) * hx (mapRadius + 1));
	memset (subhex->sub.data, '\0', sizeof (SUBHEX) * hx (mapRadius + 1));
}

static void subhexSetData (SUBHEX subhex, signed int x, signed int y, SUBHEX child)
{
	unsigned int
		r, k, i,
		offset;
	assert (subhex != NULL);
	if (subhex->sub.data == NULL)
		subhexInitData (subhex);
	if (hexMagnitude (x, y) > mapRadius)
	{
		ERROR ("Can't place subhex child: coordinates given were %d, %d, which are %d step%s out of bounds.", x, y, hexMagnitude (x, y) - mapRadius, hexMagnitude (x, y) - mapRadius == 1 ? "" : "s");
		return;
	}
	hex_xy2rki (x, y, &r, &k, &i);
	offset = hex_linearCoord (r, k, i);
	subhex->sub.data[offset] = child;
}

void subhexDestroy (SUBHEX subhex)
{
	int
		i = 0,
		max = hx (mapRadius + 1); // FIXME: as usual see hex_utils.c:hx
	if (subhex->type == HS_SUB)
	{
		if (subhex->sub.data != NULL)
		{
			while (i < max)
			{
				if (subhex->sub.data[i] != NULL)
					subhexDestroy (subhex->sub.data[i]);
				i++;
			}
			xph_free (subhex->sub.data);
			subhex->sub.data = NULL;
		}
	}
	else if (subhex->type == HS_HEX)
	{
	}
	else
	{
		ERROR ("Can't free subhex %p: Invalid type of %d", subhex, subhex->type);
		return;
	}
	xph_free (subhex);
}

/***
 * MAP TRAVERSAL FUNCTIONS
 */

SUBHEX mapPole (const char poleName)
{
	int
		len;
	if (Poles == NULL || PoleNames == NULL)
	{
		WARNING ("Can't get pole: no map exists.", NULL);
		return NULL;
	}
	len = strlen (PoleNames);
	while (len > 0)
	{
		len--;
		if (PoleNames[len] == poleName)
			return Poles[len];
	}
	ERROR ("No such pole with name \'%c\' exists", poleName);
	return NULL;
}


signed int * mapPoleConnections (const char a, const char b)
{
	signed int
		* connections = NULL;
	unsigned char
		problems = 0;
	signed char
		av, bv,
		diff;
	switch (PoleCount)
	{
		case 3:
			if (a != 'r' && a != 'g' && a != 'b')
			{
				WARNING ("Can't get pole connections: first arg is '%c', which isn't a valid pole.", a);
				problems = 1;
			}
			if (b != 'r' && b != 'g' && b != 'b')
			{
				WARNING ("Can't get pole connections: first arg is '%c', which isn't a valid pole.", b);
				problems = 1;
			}
			if (a == b)
			{
				problems = 1;
			}
			if (problems)
				break;
			connections = xph_alloc (sizeof (signed int) * 4);
			av = (a == 'r')
				? 1
				: a == 'g'
					? 2
					: 3;
			bv = (b == 'r')
				? 1
				: b == 'g'
					? 2
					: 3;
			diff = av - bv;
			if (diff == 2)
				diff = -1;
			else if (diff == -2)
				diff = 1;
			if (diff == 1)
			{
				connections[0] = 0;
				connections[1] = 2;
				connections[2] = 4;
			}
			else
			{
				connections[0] = 1;
				connections[1] = 3;
				connections[2] = 5;
			}
			connections[3] = -1;
			problems = 0;
			break;
		default:
			
			break;
	}
	if (problems)
	{
		connections = xph_alloc (sizeof (signed int));
		connections[0] = -1;
	}
	return connections;
}

char mapPoleTraversal (char p, signed int x, signed int y)
{
	const char
		pn[3] = {'r', 'g', 'b'};
	unsigned short
		ri = 0;
	while (pn[ri] != p)
	{
		ri++;
		if (ri > 2)
			return '?';
	}

	while (x > 0)
	{
		ri += 2;
		x--;
	}
	while (x < 0)
	{
		ri += 1;
		x++;
	}
	while (y > 0)
	{
		ri += 1;
		y--;
	}
	while (y < 0)
	{
		ri += 2;
		y++;
	}
	ri = ri % 3;
	return pn[ri];
}

/* NOTE: this has been coded but not tested; don't assume it works right
 */
Dynarr mapAdjacentSubhexes (const SUBHEX subhex, unsigned int distance)
{
	unsigned int
		r = 0,
		k = 0,
		i = 0;
	signed int
		x, y;
	Dynarr
		arr = dynarr_create (hx (distance + 1) + 1, sizeof (RELATIVEHEX));

	while (r <= distance)
	{
		hex_rki2xy (r, k, i, &x, &y);
		dynarr_push (arr, mapRelativeSubhexWithCoordinateOffset (subhex, 0, x, y));
		hex_nextValidCoord (&r, &k, &i);
	}
	DEBUG ("%s: done, returning dynarr w/ %d entries", __FUNCTION__, dynarr_size (arr));
	return arr;
}


/* TODO: write these three functions.
 * the vectorOffset version can fairly simply convert the vector into coordinates, which can then be passed to the coordinateOffset version (using hex_space2xy; alternately there's a hypothetical different version of that that would take an x axis vector and a y axis vector and use those to convert the space vector into coordinates; this would be used to convert to higher span levels)
 * the coordinateOffset version would have to traverse the world subdiv hierarchy: if (hexMagnitude (trav->localX + x, trav->localY + y) < mapRadius) then the target is within the current subdiv and can easily be gotten. if it's not, convert the trav->...+y coordinate pair one span level up and repeat one step higher on the hierarchy. there's a special case for pole level: pretend the current pole is at 0,0 and take the x,y values plus the one-span-difference values and add those to see which pole in which direction is closest. this is guaranteed to return a coordinate within 2 steps of the origin.
 * (not really super sure how to do the WithSubhex version, aside from "somehow the inverse of the coordinateOffset version")
 */

/* NOTE: this has been coded but not tested; don't assume it works right (and beware overflow with very large offset values [although technically that's the fault of hex_space2coord]) */
RELATIVEHEX mapRelativeSubhexWithVectorOffset (const SUBHEX subhex, const VECTOR3 * offset)
{
	signed int
		x, y;
	signed char
		span;
	bool
		valid = hex_space2coord (offset, &x, &y);
	RELATIVEHEX
		rel;
	if (valid && !x && !y)
	{
		// coordinate distance is 0,0; this is trivial
		rel = xph_alloc (sizeof (struct hexRelativePosition));
		rel->origin = subhex;
		rel->target = subhex;
		rel->distance = vectorCreate (0.0, 0.0, 0.0);
		rel->x = xph_alloc (sizeof (signed int));
		rel->y = xph_alloc (sizeof (signed int));
		rel->x[0] = 0;
		rel->y[0] = 0;
		rel->minSpan = subhexSpanLevel (subhex);
		rel->maxSpan = rel->minSpan;
		return rel;
	}
	else if (valid)
	{
		// hex_space2coord translates into individual hex coordinates, i.e., span-0 platters.
		span = subhexSpanLevel (subhex);
		DEBUG ("%s: converted %.2f, %.2f, %.2f to coordinates %d, %d at span level 0 (%d relative to the span %d of the subhex %p)", __FUNCTION__, offset->x, offset->y, offset->z, x, y, -span, span, subhex);
		return mapRelativeSubhexWithCoordinateOffset (subhex, -span, x, y);
	}
	/* TODO: (see also the TODO in hex_space2coord) it's possible that the offset vector could overflow the signed int type; in that case the x and y axis of the span 2 coordinates needs to be calculated (with any remainder using the span 1 cordinates), and that process repeats until the xy values have been specified with maximum specifity. if the values /don't/ overflow, though, this becomes much more simple: convert values and pass to the CoordinateOffset version. */
	ERROR ("Could not convert vector %f,%f,%f into coordinates.", offset->x, offset->y, offset->z);
	return NULL;
}

RELATIVEHEX mapRelativeSubhexWithCoordinateOffset (const SUBHEX subhex, const signed char relativeSpan, const signed int x, const signed int y)
{
	signed int
		netX,
		netY,
		cX, cY,
		lX = 0,
		lY = 0,
		xRemainder = 0,
		yRemainder = 0,
		i,
		goalMarker;
	signed int
		* distX = NULL,
		* distY = NULL;
	signed char
		spanDiff = relativeSpan;
	short int
		spanRange = abs (spanDiff) + 1;
	SUBHEX
		start,
		goal,
		trav;
	RELATIVEHEX
		rel = xph_alloc (sizeof (struct hexRelativePosition));
	Dynarr
		subhexStack = NULL;
	VECTOR3
		dirTemp;

	FUNCOPEN ();

	rel->origin = subhex;
	rel->target = NULL;

	// to minorly and uselessly reduce fragmentation this could allocate one address, double the size, and set -> to the halfway point. - xph 2011 06 02
	rel->x = xph_alloc (sizeof (signed int) * spanRange);
	rel->y = xph_alloc (sizeof (signed int) * spanRange);
	memset (rel->x, '\0', sizeof (signed int) * spanRange);
	memset (rel->y, '\0', sizeof (signed int) * spanRange);
	/* DEBUG ("allocated %d entr%s for coordinates", abs (spanDiff) + 1, spanDiff == 0 ? "y" : "ies"); */

	cX = x;
	cY = y;
	start = subhex;
	subhexLocalCoordinates (start, &lX, &lY);

	/* DEBUG ("starting with %p, at local %d, %d", start, lX, lY); */

	rel->minSpan = subhexSpanLevel (subhex);
	rel->maxSpan = rel->minSpan;
	DEBUG ("native span of %d w/ relative span req. of %d", rel->minSpan, spanDiff);
	if (spanDiff < 0)
		rel->minSpan = rel->maxSpan + spanDiff;
	else
		rel->maxSpan = rel->minSpan + spanDiff;
	DEBUG ("set min/max span to %d/%d, w/ a range of %d", rel->minSpan, rel->maxSpan, spanRange);

	while (spanDiff < 0)
	{
		if (start->type != HS_SUB)
		{
			WARNING ("Hit tile layer unexpectedly while traversing map; wanted %d more level%s (down)", spanDiff, spanDiff == 1 ? "" : "s");
			mapScaleCoordinates (spanDiff, x, y, &cX, &cY, NULL, NULL);
			break;
		}
		else if (start->sub.data == NULL || start->sub.data[0] == NULL)
		{
			WARNING ("Hit unexpanded layer while traversing map; wanted %d more level%s (down)", spanDiff, spanDiff == 1 ? "" : "s");
			mapScaleCoordinates (spanDiff, x, y, &cX, &cY, NULL, NULL);
			break;
		}
		// this is /so/ not the place for this assert - xph 2011-06-02
		assert (subhexParent (subhexData (start, 0, 0)) == start);
		start = subhexData (start, 0, 0);
		/* DEBUG ("%p: local coords %d, %d", trav, trav->sub.x, trav->sub.y); */
		spanDiff++;
	}
	while (spanDiff > 0)
	{
		if (subhexParent (start) == NULL)
		{
			WARNING ("Hit pole layer while traversing map; wanted %d more level%s (up)", spanDiff, spanDiff == 1 ? "" : "s");
			// it would be possible to store the remainder here but i don't know what it could possibly be used for - xph 2011 06 04
			mapScaleCoordinates (spanDiff, x, y, &cX, &cY, NULL, NULL);
			break;
		}
		start = subhexParent (start);
		spanDiff--;
	}

	subhexLocalCoordinates (start, &lX, &lY);
	netX = lX + cX;
	netY = lY + cY;

	DEBUG (" @ %p: local: %d, %d; move: %d, %d; net: %d, %d", start, lX, lY, cX, cY, netX, netY);

	// and now we hit the main bulk of the function: (this requires recursively traversing the subhex hierarchy if subhex edges are passed)

	subhexStack = dynarr_create (abs (spanDiff) + 2, sizeof (SUBHEX));
	i = 0;
	goal = NULL;
	while (hexMagnitude (netX, netY) > mapRadius)
	{
		DEBUG ("iterating on %d up the span hierarchy", i);
		assert (i < spanRange);
		if (subhexSpanLevel (start) == mapSpan)
		{
			break;
		}
		mapScaleCoordinates (1, netX, netY, &cX, &cY, &xRemainder, &yRemainder);
		DEBUG ("condensed coordinates %d, %d to %d, %d (with remainder %d, %d at index %d)", netX, netY, cX, cY, xRemainder, yRemainder, i);

		// these two values should be the remainder of the scale conversion (i think)
		rel->x[i] = xRemainder;
		rel->y[i] = yRemainder;

		i++;
		dynarr_push (subhexStack, start);

		start = subhexParent (start);
		/* DEBUG ("got parent %p; using that", start); */
		subhexLocalCoordinates (start, &lX, &lY);
		netX = lX + cX;
		netY = lY + cY;
	}
	DEBUG ("broke from traversal loop with a value of %p", start);
	DEBUG ("current x,y: %d, %d; local:  %d, %d; net: %d, %d", cX, cY, lX, lY, netX, netY);

	rel->x[i] = netX;
	rel->y[i] = netY;
	if (subhexSpanLevel (start) == mapSpan)
	{
		// at this point, no matter the magnitude, the target can be found since there are no data structures to overrun this high up
		goal = mapPole (mapPoleTraversal (subhexPoleName (start), netX, netY));
	}
	else
		goal = subhexData (subhexParent (start), netX, netY);
	/* TODO: i don't think this value is strictly necessary; i think the value is effectively stored in the span level of the goal value. maybe? the thing is that since the goal can be calculated from a bunch of different values i don't know if "the number of down-traverse loops" is the right value is strictly correct; it might actually be "the span level of the goal" and then this usage would be doubly wrong
	 * that being said this is used in the goal traversal loop near the bottom to line up the rel->x/y value index to the proper span level
	 *  - xph 2011 06 04
	 */
	goalMarker = i;

	/* we're using a seperate distX/Y instead of rel->x/y because if we change the rel values it becomes impossible to generate the target subhex from a low-precision RELATIVEHEX, since the rel-> values map directly to subhex local coordinates
	 * also we're doing this calculation seperately from the actual goal traversal step, below, because while we don't care if the goal is at low-resolution, it's pretty vitally important that we always have the coordinate and distance information set for every RELATIVEHEX
	 * - xph 2011 06 04
	 */
	distX = xph_alloc (sizeof (signed int) * spanRange * 2);
	distY = distX + spanRange;
	memcpy (distX, rel->x, sizeof (signed int) * spanRange);
	memcpy (distY, rel->y, sizeof (signed int) * spanRange);
	while (i > 0)
	{
		i--;
		DEBUG ("saved remainder coordinates: %d, %d", rel->x[i], rel->y[i]);
		trav = *(SUBHEX *)dynarr_pop (subhexStack);

		subhexLocalCoordinates (trav, &cX, &cY);
		if (i >= abs (spanDiff))
			mapScaleCoordinates (-1, netX, netY, &lX, &lY, NULL, NULL);
		else
			mapScaleCoordinates (-1, distX[i + 1], distY[i + 1], &lX, &lY, NULL, NULL);
		lX += distX[i];
		lY += distY[i];
		/* DEBUG ("goal distance from start center: %d, %d; start distance from start center: %d, %d", lX, lY, cX, cY);
		DEBUG ("net distance: %d, %d", lX - cX, lY - cY);*/
		// i don't know if this actually works right when there's more than one level of traversing going on here, but i /think/ it's right
		distX[i] = lX - cX;
		distY[i] = lY - cY;
	}

	rel->distance = vectorCreate (0.0, 0.0, 0.0);
	i = 0;
	while (i < spanRange)
	{
		dirTemp = mapDistanceFromSubhexCentre (rel->minSpan + i, distX[i], distY[i]);
		DEBUG ("iterating on %d to calculate vector distance (@ span %d: %d, %d); for this step: %.2f, %.2f, %.2f", i, rel->minSpan + i, distX[i], distY[i], dirTemp.x, dirTemp.y, dirTemp.z);
		rel->distance = vectorAdd (&rel->distance, &dirTemp);
		i++;
	}
	DEBUG ("final distance measure: %f, %f, %f", rel->distance.x, rel->distance.y, rel->distance.z);
	dynarr_destroy (subhexStack);
	xph_free (distX);



	if (goal == NULL)
	{
		INFO ("Traversal early exit at span %d: perfect-match goal doesn't exist; best match might be the parent of the start (%p)", subhexSpanLevel (start), subhexParent (start));
		goal = subhexParent (start);
	}
	else
	{
		i = goalMarker;
		while (i > 0)
		{
			i--;
			trav = subhexData (goal, rel->x[i], rel->y[i]);
			DEBUG ("down-traversal to goal: level %d @ %d, %d: %p", subhexSpanLevel (goal), rel->x[i], rel->y[i], trav);
			if (trav == NULL)
			{
				INFO ("Traversal early exit at span %d: perfect-match target doesn't exist;", subhexSpanLevel (goal) - 1);
				break;
			}
			goal = trav;
		}
	}

	rel->target = goal;
	FUNCCLOSE ();
	return rel;
}

RELATIVEHEX mapRelativeSubhexWithSubhex (const SUBHEX subhex, const SUBHEX target)
{
	SUBHEX
		trav;
	const SUBHEX
		* smaller;
	unsigned char
		spanGoal,
		spanDiff,
		i;
	signed int
		* subnormalX,
		* subnormalY;
	if (subhex == NULL || target == NULL)
	{
		ERROR ("Can't calculate subhex offset: passed NULL subhex!", NULL);
		return NULL;
	}
	spanDiff = abs (subhexSpanLevel (target) - subhexSpanLevel (subhex));
	if (spanDiff > 0)
	{
		if (subhexSpanLevel (subhex) < subhexSpanLevel (target))
		{
			smaller = &subhex;
			spanGoal = subhexSpanLevel (target);
		}
		else
		{
			smaller = &target;
			spanGoal = subhexSpanLevel (subhex);
			spanDiff = subhexSpanLevel (subhex) - subhexSpanLevel (target);
		}
		subnormalX = xph_alloc (sizeof (signed int) * spanDiff);
		subnormalY = xph_alloc (sizeof (signed int) * spanDiff);
		trav = *smaller;
		i = spanDiff;
		while (i > 0)
		{
			i--;
			subhexLocalCoordinates (trav, &subnormalX[i], &subnormalY[i]);
			trav = subhexParent (trav);
		}
	}



	// LOL THIS IS DEBUG TEXT; IGNORE THIS
	if (spanDiff)
	{
		INFO ("the subhexes started at span level %d and %d, but now both are at %d", subhexSpanLevel (subhex), subhexSpanLevel (target), subhexSpanLevel (*smaller));
		i = 0;
		INFO ("Subnormal offsets for the smaller subhex are as follows:", NULL);
		while (i < spanDiff)
		{
			INFO ("span %d: %d, %d", spanGoal - (i + 1), subnormalX[i], subnormalY[i]);
			i++;
		}
	}
	else
	{
		INFO ("the two subhexes are at the same initial span level (%d)", subhexSpanLevel (subhex));
	}
	
	return NULL;
}


SUBHEX mapRelativeTarget (const RELATIVEHEX relativePosition)
{
	assert (relativePosition != NULL);
	return relativePosition->target;
}

VECTOR3 mapRelativeDistance (const RELATIVEHEX relPos)
{
	assert (relPos != NULL);
	return relPos->distance;
}

bool isPerfectFidelity (const RELATIVEHEX relPos)
{
	return relPos->minSpan == subhexSpanLevel (relPos->target);
}

void mapRelativeDestroy (RELATIVEHEX rel)
{
	if (!rel)
		return;
	xph_free (rel->x);
	xph_free (rel->y);
	xph_free (rel);
}


/* NOTE: this has been coded but not tested; don't assume it works right */
VECTOR3 mapDistanceFromSubhexCentre (const unsigned char spanLevel, const signed int x, const signed int y)
{
	FUNCOPEN ();
	signed int
		* coords = mapSpanCentres (spanLevel);
	if (coordinatesOverflow (x, y, coords))
		return vectorCreate (0.0, 0.0, 0.0);
	FUNCCLOSE ();
	return hex_xyCoord2Space
	(
		x * coords[0] + y * coords[2],
		x * coords[1] + y * coords[3]
	);
}

/* BUG: this only works right when span == 1; the coordinates need to be converted recursively if span is otherwise, and that's not done yet
 *  - xph 2011 06 12
 */
bool mapVectorOverrunsPlatter (const unsigned char span, const VECTOR3 * vector)
{
	signed int
		x = 0,
		y = 0;
	hex_space2coord (vector, &x, &y);
	if (hexMagnitude (x, y) > mapRadius)
		return TRUE;
	return FALSE;
}

/***
 * given a set of coordinates (the values x and y), translate the values to a
 * different span level (denoted by relativeSpan) and change the values *xp
 * and *yp to the new coordinates.
 */
bool mapScaleCoordinates (signed char relativeSpan, signed int x, signed int y, signed int * xp, signed int * yp, signed int * xRemainder, signed int * yRemainder)
{
	signed int
		* centres;
	signed int
		scaledX = 0,
		scaledY = 0,
		i = 0;
	if (xRemainder)
		*xRemainder = 0;
	if (yRemainder)
		*yRemainder = 0;
	if (xp == NULL || yp == NULL)
		return FALSE;
	else if (relativeSpan == 0)
	{
		*xp = x;
		*yp = y;
		return TRUE;
	}
	else if (x == 0 && y == 0)
	{
		*xp = 0;
		*yp = 0;
		return TRUE;
	}
	
	/* DEBUG ("starting w/ %d, %d", x, y); */
	if (relativeSpan < 0)
	{
		/* step down in span, which means coordinate values gain resolution
		 * but lose range, and non-zero values get larger
		 */
		centres = mapSpanCentres (abs (relativeSpan));
		if (coordinatesOverflow (x, y, centres))
		{
			ERROR ("\t(tried to shift %d span level%s down)", relativeSpan * -1, relativeSpan == -1 ? "" : "s");
			*xp = 0;
			*yp = 0;
			return FALSE;
		}
		*xp = x * centres[0] + y * centres[2];
		*yp = x * centres[1] + y * centres[3];
		//DEBUG ("downscale values turns %d, %d into %d, %d", x, y, *xp, *yp);
		return TRUE;
	}
	// step up in span, which means coordinate values gain range but lose resolution, and non-zero values get smaller and may be truncated.
	centres = mapSpanCentres (relativeSpan);
	while (i < 6)
	{
		/* there's a slight simplification that i'm not using here since
		 * idk how to put it in a way that actually simplifies the code:
		 * if index i can be subtracted, then either index i + 1 % 6 or
		 * index i - 1 % 6 can also be subtracted, and no other values
		 * can. since we iterate from 0..5, if 0 hits the other value may
		 * be 1 OR 5; in all other cases if i is a hit then i + 1 % 6 is
		 * the other hit
		 *  - xph 2011-05-29
		 */
		while (hexMagnitude (x - centres[i * 2], y - centres[i * 2 + 1]) < hexMagnitude (x, y))
		{
			x -= centres[i * 2];
			y -= centres[i * 2 + 1];
			scaledX += XY[i][X];
			scaledY += XY[i][Y];
			/* DEBUG ("scaled to %d, %d w/ rem %d, %d", scaledX, scaledY, x, y); */
		}
		i++;
	}
	*xp = scaledX;
	*yp = scaledY;
	if (xRemainder)
		*xRemainder = x;
	if (yRemainder)
		*yRemainder = y;
	if (xRemainder == NULL && yRemainder == NULL && (x || y))
		INFO ("%s: lost %d,%d to rounding error at relative span %d", __FUNCTION__, x, y, relativeSpan);
	return TRUE;
}

/***
 * ??? what was the point of this function anyway; it seems like it'd be useful for calculating out of bound coordinates but only up to one step away, at which point it would being silently returning the wrong values since it can only return a single direction value, not the higher-span coordinates.
 */
/*
bool mapBridge (const signed int x, const signed int y, signed int * xp, signed int * yp, signed char * dir)
{
	return FALSE;
}
*/


/* FIXME: this crashes when an attempt is made to get mapSpanCentres (0)
 * this is mostly useless; the values returned would be the same values as are
 * stored in XY in hex_utility.c. but it should still work right, since it is
 * called that way in practice quite frequently
 */
static Dynarr
	centreCache =  NULL;
signed int * const mapSpanCentres (const unsigned char span)
{
	signed int
		* first,
		* r,
		* s = NULL,
		* t,
		x, y;
	unsigned short
		match = span,
		i;
	LOG (E_FUNCLABEL, "%s (%d)...", __FUNCTION__, span);
	if (centreCache == NULL)
		centreCache = dynarr_create (span + 1, sizeof (signed int **));
	if ((r = *(signed int **)dynarr_at (centreCache, span)) != NULL)
		return r;
	while (match > 0 && (s = *(signed int **)dynarr_at (centreCache, match)) == NULL)
		match--;
	if (match <= 1)
	{
		// we care about the 1:1 span-0 mapping because it makes this function consistant; calling it with 0 will lead to expected behavior instead of crashing or errors
		s = xph_alloc (sizeof (signed int) * 12);
		i = 0;
		while (i < 12)
		{
			s[i] = XY[i/2][i%2];
			i++;
		}
		dynarr_assign (centreCache, 0, s);

		DEBUG ("generating first centres", NULL);
		first = xph_alloc (sizeof (signed int) * 12);
		i = 0;
		while (i < 6)
		{
			hex_centerDistanceCoord (mapRadius, i, &first[i * 2], &first[i * 2 + 1]);
			i++;
		}
		dynarr_assign (centreCache, 1, first);
		match = 2;

		/* vvv LOL DEBUG PRINTING vvv *
		i = 0;
		DEBUG ("Generated %d-th span level centre values:", match - 1);
		while (i < 6)
		{
			DEBUG ("%d: %d,%d", i, first[i * 2], first[i * 2 + 1]);
			i++;
		}
		 * ^^^ LOL DEBUG PRINTING ^^^ */

	}
	else
		first = *(signed int **)dynarr_at (centreCache, 1);
	t = first;
	s = NULL;
	while (match < span)
	{
		s = xph_alloc (sizeof (signed int) * 12);
		i = 0;
		while (i < 6)
		{
			x = t[i * 2] * 2 + t[((i + 1) % 6) * 2];
			y = t[i * 2 + 1] * 2 + t[((i + 1) % 6) * 2 + 1];
			/* i'm not totally sure about the following lines; i'm pretty sure
			 * the above lines correctly calculate one step of distance,
			 * but... i /think/ the correct thing to do here is realize that
			 * since *first is the one-level-difference values, one step's
			 * worth of x,y (as calculated above) values need to be multiplied
			 * by the expanded span level difference values in first that are
			 * below, since 1,0 on span level n is first[0],first[1] on span
			 * level n-1 and 0,1 on span level n is first[2],first[3] on span
			 * level n-1.
			 * i think.
			 */
			s[i * 2]     = x * first[0] + y * first[2];
			s[i * 2 + 1] = x * first[1] + y * first[3];
			i++;
		}
		dynarr_assign (centreCache, match, s);
		match++;
		t = s;

		/* vvv LOL DEBUG PRINTING vvv *
		i = 0;
		DEBUG ("Generated %d-th span level centre values:", match - 1);
		while (i < 6)
		{
			DEBUG ("%d: %d,%d", i, s[i * 2], s[i * 2 + 1]);
			i++;
		}
		 * ^^^ LOL DEBUG PRINTING ^^^ */

	}
	r = t;
	FUNCCLOSE ();
	return r;
}

static bool coordinatesOverflow (const signed int x, const signed int y, const signed int * const coords)
{
	signed int
		ax = abs (x),
		ay = abs (y),
		cxx = abs (coords[0]),
		cxy = abs (coords[1]),
		cyx = abs (coords[2]),
		cyy = abs (coords[3]);
	if ((cxx && ax * cxx < ax)
		|| (cxy && ax * cxy < ax)
		|| (cyx && ay * cyx < ay)
		|| (cyy && ay * cyy < ay))
	{
		ERROR ("Coordinate overflow while shifting coordinates %d, %d down; using axes %d,%d and %d,%d; coordinate not representable!", x, y, coords[0], coords[1], coords[2], coords[3]);
		return TRUE;
	}
	return FALSE;
}

/***
 * INFORMATIONAL / GETTER FUNCTIONS
 */

unsigned char subhexSpanLevel (const SUBHEX subhex)
{
	if (subhex == NULL)
	{
		ERROR ("%s: Passed NULL subhex!", __FUNCTION__);
		return 0;
	}
	if (subhex->type == HS_HEX)
		return 0;
	return subhex->sub.span;
}

char subhexPoleName (const SUBHEX subhex)
{
	SUBHEX
		sh = subhex,
		parent;
	int
		i;
	if (Poles == NULL)
	{
		ERROR ("Can't get pole name: no poles!", NULL);
		return '!';
	}
	else if (subhex == NULL)
	{
		ERROR ("Can't get pole name: NULL subhex", NULL);
		return '?';
	}
	i = PoleCount;
	parent = subhexParent (subhex);
	while (parent != NULL)
	{
		sh = parent;
		parent = subhexParent (parent);
	}
	/* DEBUG ("got pole value %p from %p", sh, subhex); */
	while (i > 0)
	{
		i--;
		if (Poles[i] == sh)
			return PoleNames[i];
	}
	return '?';
}

SUBHEX subhexData (const SUBHEX subhex, signed int x, signed int y)
{
	unsigned int
		r, k, i,
		offset = 0;
	assert (subhex != NULL);
	if (subhex->type != HS_SUB)
		return NULL;
	if (subhex->sub.data == NULL)
		return NULL;
	hex_xy2rki (x, y, &r, &k, &i);
	offset = hex_linearCoord (r, k, i);
	if (offset >= hx (mapRadius + 1))
	{
		WARNING ("Can't get offset %d from subhex; maximum offset is %d", offset, hx(mapRadius));
		return NULL;
	}
	return subhex->sub.data[offset];
}

SUBHEX subhexParent (const SUBHEX subhex)
{
	return subhex->type == HS_SUB
		? subhex->sub.parent
		: subhex->hex.parent;
}

bool subhexLocalCoordinates (const SUBHEX subhex, signed int * xp, signed int * yp)
{
	if (subhex == NULL)
		return FALSE;
	if (xp != NULL)
		*xp = (subhex->type == HS_SUB)
			? subhex->sub.x
			: subhex->hex.x;
	if (yp != NULL)
		*yp = (subhex->type == HS_SUB)
			? subhex->sub.y
			: subhex->hex.y;
	return TRUE;
}

WORLDHEX subhexGeneratePosition (const SUBHEX subhex)
{
	WORLDHEX
		whx;
	SUBHEX
		pole;
	signed int
		i,
		depth;
	FUNCOPEN ();
	if (subhex == NULL)
	{
		ERROR ("%s: Passed NULL subhex!", __FUNCTION__);
		return NULL;
	}
	whx = xph_alloc (sizeof (struct hexWorldPosition));
	whx->subhex = subhex;
	depth = mapSpan - subhexSpanLevel (subhex);
	whx->spanDepth = depth;
	if (depth < 0)
	{
		ERROR ("Cannot create worldhex: invalid subhex; has span of %d when map limit is %d; can't create worldhex and also other things are likely going to break soon.", subhexSpanLevel (subhex), mapSpan);
		xph_free (whx);
		return NULL;
	}
	else if (depth == 0)
	{
		whx->x = NULL;
		whx->y = NULL;
		pole = subhex;
	}
	else
	{
		whx->x = xph_alloc (sizeof (signed int) * whx->spanDepth);
		whx->y = xph_alloc (sizeof (signed int) * whx->spanDepth);
		depth--;
		whx->x[depth] = subhex->sub.x;
		whx->y[depth] = subhex->sub.y;
		pole = subhex;
		while (depth > 0)
		{
			pole = subhex->sub.parent;
			depth--;
			if (pole == NULL)
			{
				ERROR ("Cannot create worldhex: unexpectedly hit pole level while traversing up from subhex %p; expected %d more level%s.", subhex, depth, depth == 1 ? "" : "s");
				return NULL;
			}
			whx->x[depth] = pole->sub.x;
			whx->y[depth] = pole->sub.y;
		}
	}
	i = 0;
	while (i < PoleCount)
	{
		if (pole == Poles[i])
		{
			whx->pole = PoleNames[i];
			break;
		}
		i++;
	}
	FUNCCLOSE ();
	return whx;
}

WORLDHEX worldhexDuplicate (const WORLDHEX whx)
{
	WORLDHEX
		dup = xph_alloc (sizeof (struct hexWorldPosition));
	size_t
		size;
	memcpy (dup, whx, sizeof (struct hexWorldPosition));
	if (whx->spanDepth == 0)
	{
		return dup;
	}
	size = sizeof (signed int) * dup->spanDepth;
	dup->x = xph_alloc (size);
	dup->y = xph_alloc (size);
	memcpy (dup->x, whx->x, size);
	memcpy (dup->y, whx->y, size);
	return dup;
}

static char
	WorldhexPrintBuffer[128];
const char * const worldhexPrint (const WORLDHEX whx)
{
	return WorldhexPrintBuffer;
}

void worldhexDestroy (WORLDHEX whx)
{
	if (whx->x != NULL)
		xph_free (whx->x);
	if (whx->y != NULL)
		xph_free (whx->y);
	xph_free (whx);
}

SUBHEX worldhexSubhex (const WORLDHEX whx)
{
	return whx->subhex;
}

char worldhexPole (const WORLDHEX whx)
{
	return whx->pole;
}


/***
 * RENDERING FUNCTIONS
 */

int __v, __n;
#define GETCORNER(p, n)		(__n = (n), __v = (__n % 2 ? p[__n/2] & 0x0f : (p[__n/2] & 0xf0) >> 4), (__v & 8) ? 0 - ((__v & 7)) : __v)
#define SETCORNER(p, n, v)	(__n = (n), __v = (v), __v = (__v < 0 ? ((~__v & 15) + 1) | 8 : __v & 7), p[__n/2] = (__n % 2 ? (p[__n/2] & 0x0f) | (__v << 4) : (p[__n/2] & 0xf0) | __v))

#define FULLHEIGHT(hex, i)		(__v = GETCORNER (hex->corners, i), (__v < 0 && hex->centre < abs (__v)) ? 0 : ((__v > 0 && hex->centre > (UINT_MAX - __v)) ? UINT_MAX : hex->centre + __v))

/* keep these cast as float no matter what they are; since hex heights are
 * unsigned ints we really don't want to force a signed int * unsigned int
 * multiplication when we render the hexes (or whatever); just accept that
 * we're going to lose precision when we do it. so don't do it until you
 * render.
 *  -xph 2011-04-03
 */
#define HEX_SIZE	(float)30
#define HEX_SIZE_4	(float)7.5

static Dynarr
	RenderCache = NULL;
SUBHEX
	RenderOrigin = NULL;
static unsigned char
	AbsoluteViewLimit = 8;


void worldSetRenderCacheCentre (SUBHEX origin)
{
	if (origin == RenderOrigin)
	{
		INFO ("%s called with repeat origin (%p). already set; ignoring", origin);
		return;
	}
	assert (subhexSpanLevel (origin) == 1);
	if (subhexSpanLevel (origin) == 0)
	{
		WARNING ("The rendering origin is a specific tile; using its parent instead", NULL);
		origin = subhexParent (origin);
	}
	else if (subhexSpanLevel (origin) != 1)
		ERROR ("The rendering origin isn't at maximum resolution; this may be a problem", NULL);
	if (RenderCache != NULL)
	{
		dynarr_wipe (RenderCache, (void (*)(void *))mapRelativeDestroy);
		dynarr_destroy (RenderCache);
	}
	RenderCache = mapAdjacentSubhexes (origin, AbsoluteViewLimit);
	DEBUG ("set render cache to %p", RenderCache);
}

/*
void worldUpdateRenderCache (Entity player)
{
	SUBHEX
		new = position_getGround (player);
	if (PlayerSubhex == new)
		return;
	if (subhexSpanLevel (new) != 1)
		WARNING ("The player's location isn't at maximum resolution; this may be a problem", NULL);
	if (RenderCache != NULL)
		dynarr_destroy (RenderCache);
	RenderCache = mapAdjacentSubhexes (PlayerSubhex, AbsoluteViewLimit);
}
*/

void mapDraw ()
{
	VECTOR3
		centreOffset;
	RELATIVEHEX
		rel;
	SUBHEX
		sub;
	unsigned int
		tier1Detail = hx (AbsoluteViewLimit + 1),/*
		tier2Detail = hx (AbsoluteViewLimit / 2),
		tier3Detail = hx (AbsoluteViewLimit),*/
		i = 0;
	if (RenderCache == NULL)
	{
		ERROR ("Cannot draw map: Render cache is uninitialized.", NULL);
		return;
	}
	while (i < tier1Detail)
	{
		rel = *(RELATIVEHEX *)dynarr_at (RenderCache, i);
		//DEBUG ("trying to render the subhex %d-th in the cache (relativehex val: %p)", i, rel);
		centreOffset = mapRelativeDistance (rel);
		//DEBUG (" - centre offset: %f %f %f", centreOffset.x, centreOffset.y, centreOffset.z);
		sub = mapRelativeTarget (rel);
		//DEBUG (" - target: %p", sub);
		i++;
		if (sub == NULL || subhexSpanLevel (sub) != 1)
		{
			DEBUG ("skipping value; subhex is NULL (%p) or span level isn't 1 (%p)", sub, sub == NULL ? -1 : subhexSpanLevel (sub));
			continue;
		}
		subhexDraw (&sub->sub, centreOffset);
	}/*
	while (i < tier2Detail)
	{
		i++;
	}
	while (i < tier3Detail)
	{
		i++;
	}*/
}

void subhexDraw (const SUBDIV sub, const VECTOR3 offset)
{
	SUBHEX
		hex;
	unsigned int
		i = 0,
		max = hx (mapRadius + 1);

	//FUNCOPEN ();
	assert (sub->data != NULL);
	while (i < max)
	{
		hex = sub->data[i];
		assert (hex != NULL);
		hexDraw (&hex->hex, offset);
		i++;
	}
	//FUNCCLOSE ();
}

void hexDraw (const HEX hex, const VECTOR3 centreOffset)
{
	//FUNCOPEN ();

	VECTOR3
		hexOffset = hex_xyCoord2Space (hex->x, hex->y),
		totalOffset = vectorAdd (&centreOffset, &hexOffset);
	float
		lR = 0.0,
		lG = 0.0,
		lB = 0.0;
	unsigned int
		corners[6] = {
			FULLHEIGHT (hex, 0),
			FULLHEIGHT (hex, 1),
			FULLHEIGHT (hex, 2),
			FULLHEIGHT (hex, 3),
			FULLHEIGHT (hex, 4),
			FULLHEIGHT (hex, 5)
		};
	signed int
		i, j;

	// light will be a value between 0 and 7
	
	lR = 0.5 + (0.1428 * 0.5 * ((hex->light & 0x01) + (hex->light & 0x02)));
	lG = 0.5 + (0.1428 * 0.5 * ((hex->light & 0x02) + (hex->light & 0x04)));
	lB = 0.5 + (0.1428 * 0.5 * ((hex->light & 0x01) + (hex->light & 0x04)));

	glColor3f (lR, lG, lB);

	//DEBUG ("drawing hex based at %.2f, %.2f, %.2f", totalOffset.x, hex->centre * HEX_SIZE_4, totalOffset.z);
	glBegin (GL_TRIANGLE_FAN);
	glVertex3f (totalOffset.x, hex->centre * HEX_SIZE_4, totalOffset.z);
	glVertex3f (totalOffset.x + H[0][0], corners[0] * HEX_SIZE_4, totalOffset.z + H[0][1]);
	glVertex3f (totalOffset.x + H[5][0], corners[5] * HEX_SIZE_4, totalOffset.z + H[5][1]);
	glVertex3f (totalOffset.x + H[4][0], corners[4] * HEX_SIZE_4, totalOffset.z + H[4][1]);
	glVertex3f (totalOffset.x + H[3][0], corners[3] * HEX_SIZE_4, totalOffset.z + H[3][1]);
	glVertex3f (totalOffset.x + H[2][0], corners[2] * HEX_SIZE_4, totalOffset.z + H[2][1]);
	glVertex3f (totalOffset.x + H[1][0], corners[1] * HEX_SIZE_4, totalOffset.z + H[1][1]);
	glVertex3f (totalOffset.x + H[0][0], corners[0] * HEX_SIZE_4, totalOffset.z + H[0][1]);
	glEnd ();
	i = 0;
	j = 1;
	while (i < 6)
	{
		if (hex->edgeBase[i * 2] >= corners[i] &&
			hex->edgeBase[i * 2 + 1] >= corners[j])
		{
			i++;
			j = (i + 1) % 6;
			continue;
		}
		glBegin (GL_TRIANGLE_STRIP);
		glVertex3f (totalOffset.x + H[i][0], corners[i] * HEX_SIZE_4, totalOffset.z + H[i][1]);
		glVertex3f (totalOffset.x + H[j][0], corners[j] * HEX_SIZE_4, totalOffset.z + H[j][1]);
		glVertex3f (totalOffset.x + H[i][0], (hex->edgeBase[i * 2]) * HEX_SIZE_4, totalOffset.z + H[i][1]);
		glVertex3f (totalOffset.x + H[j][0], (hex->edgeBase[i * 2 + 1]) * HEX_SIZE_4, totalOffset.z + H[j][1]);
		glEnd ();
		i++;
		j = (i + 1) % 6;
	}
	//FUNCCLOSE ();
}
