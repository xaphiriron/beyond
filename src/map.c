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
	 * In either case, if data isn't NULL to begin with it stores hx (mapRadius) values..
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

static SUBHEX
	* Poles = NULL;
static char
	* PoleNames = NULL;
static int
	PoleCount = 0;

// did i mean to have these as globals? - xph 2011-05-21
unsigned char
	mapRadius = 8,
	mapSpan = 4;

static bool coordinatesOverflow (const signed int x, const signed int y, const signed int * const coords);

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
	Poles = xph_alloc (sizeof (union hexOrSubdiv) * count);
	while (count > 0)
	{
		count--;
		Poles[count] = mapSubhexCreate (NULL, 0, 0);
	}
	
	return FALSE;
}

bool worldExists ()
{
	if (Poles == NULL || PoleNames == NULL)
		return FALSE;
	return TRUE;
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

/*
	diff = 0;
	problems = 0;
	while (!problems)
	{
		DEBUG ("connections[%d]: %d", diff, connections[diff]);
		if (connections[diff] == -1)
			problems++;
		diff++;
	}
*/

	return connections;
}

void mapForceSubdivide (SUBHEX subhex)
{
	unsigned int
		childCount = hx (mapRadius);
	signed int
		x = 0,
		y = 0;
	unsigned int
		r = 0,
		k = 0,
		i = 0,
		offset;
	SUBHEX (*func)(const SUBHEX, signed int, signed int) = NULL;
	if (subhex->type != HS_SUB)
		return;
	if (subhex->sub.data == NULL)
		subhex->sub.data = xph_alloc (sizeof (SUBHEX) * childCount);
	func = subhex->sub.span == 1
		? mapTileCreate
		: mapSubhexCreate;
	while (r <= mapRadius)
	{
		hex_rki2xy (r, k, i, &x, &y);
		offset = hex_linearCoord (r, k, i);
		hex_nextValidCoord (&r, &k, &i);
		if (subhex->sub.data[offset] != NULL)
			continue;
		subhex->sub.data[offset] = func (subhex, x, y);
	}
}

void mapForceGrowAtLevelForDistance (SUBHEX subhex, unsigned char spanLevel, unsigned int distance)
{
	SUBHEX
		centre = subhex;
	unsigned char
		currentSpan;
	if (spanLevel > mapSpan)
	{
		ERROR ("Got meaningless request to grow above the max span level; discarding. (max: %d; request: %d)", mapSpan, spanLevel);
		return;
	}
	currentSpan = subhexSpanLevel (centre);
	if (currentSpan < spanLevel)
	{
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
	}
	else if (currentSpan > spanLevel)
	{
		while (currentSpan > spanLevel)
		{
			if (centre->sub.data == NULL)
				centre->sub.data = xph_alloc (sizeof (SUBHEX) * hx (mapRadius));
			if (subhexData (centre, 0) == NULL)
				centre->sub.data[0] = mapSubhexCreate (centre, 0, 0);
			centre = subhexData (centre, 0);
			currentSpan--;
		}
	}
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
}

/***
 * SIMPLE CREATION AND INITIALIZATION FUNCTIONS
 */

SUBHEX mapTileCreate (const SUBHEX parent, signed int x, signed int y)
{
	SUBHEX
		sh = xph_alloc (sizeof (union hexOrSubdiv));
	sh->type = HS_HEX;
	sh->hex.parent = parent;
	if (parent == NULL)
		WARNING ("Hex generated without a parent; this is probably a mistake. (It's got a memory address of %p)", sh);
	sh->hex.x = x;
	sh->hex.y = y;

	return sh;
}

SUBHEX mapSubhexCreate (const SUBHEX parent, signed int x, signed int y)
{
	SUBHEX
		sh = xph_alloc (sizeof (union hexOrSubdiv));
	sh->type = HS_SUB;
	sh->sub.parent = parent;
	sh->sub.x = x;
	sh->sub.y = y;

	if (parent != NULL)
		sh->sub.span = parent->sub.span - 1;
	else
		sh->sub.span = mapSpan;
	sh->sub.data = NULL;
	return sh;
}

/***
 * MAP TRAVERSAL FUNCTIONS
 */

SUBHEX mapGetRelativePosition (const SUBHEX subhex, const signed char span, const signed int x, const signed int y)
{
	signed char
		spanDiff = span;
	SUBHEX
		pos = subhex;
	signed int
		posX = 0,
		posY = 0,
		relX = x,
		relY = y;
	if (subhex == NULL)
	{
		ERROR ("Passed unexpected NULL argument for map traversal.", NULL);
		return NULL;
	}
	if (spanDiff > 0)
	{
		while (spanDiff != 0)
		{
			if (pos->type != HS_SUB)
			{
				WARNING ("Hit tile layer unexpectedly while traversing map; wanted %d more level%s (down)", spanDiff, spanDiff == 1 ? "" : "s");
				mapScaleCoordinates (spanDiff, x, y, &relX, &relY);
				break;
			}
			if (pos->sub.data == NULL || pos->sub.data[0] == NULL)
			{
				WARNING ("Hit unexpanded layer while traversing map; wanted %d more level%s (down)", spanDiff, spanDiff == 1 ? "" : "s");
				mapScaleCoordinates (spanDiff, x, y, &relX, &relY);
				break;
			}
			pos = pos->sub.data[0];
			spanDiff--;
		}
	}
	else if (spanDiff < 0)
	{
		while (spanDiff != 0)
		{
			if (pos->sub.parent == NULL)
			{
				WARNING ("Hit pole layer unexpectedly while traversing map; wanted %d more level%s (up)", spanDiff * -1, spanDiff == -1 ? "" : "s");
				mapScaleCoordinates (spanDiff, x, y, &relX, &relY);
				break;
			}
			pos = pos->sub.parent;
			spanDiff++;
		}
	}
	subhexLocalCoordinates (pos, &posX, &posY);
	DEBUG ("Got subhex at the right span level: at %d, with local coordinates of %d,%d; need to get to subhex %d,%d distant", subhexSpanLevel (pos), posX, posY, relX, relY);

	// TODO: whatever comes next???

	return NULL;
}

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

/* TODO: returns a dynarr of all subhexes within {distance} steps (of the same span level as {subhex}) of {subhex}, substituting NULL values and subhexes one step up if the landscape isn't fully generated
 *
 * okay actually "substituting NULL values and subhexes one step up if the landscape isn't fully generated" is really vague. substituting how? i had the vague idea that values would be NULL except at the position corresponding to local 0,0, whick would be the next level up, except it's quite possible that the local 0,0 won't be within the distance and thus there will just be a lot of mysterious NULL values. substituting the smallest generated subhex in all cases would be better, but it would mean anything using this function would have to make sure to check every subhex's span level and calculate out the right local position itself
 */
Dynarr mapAdjacentSubhexes (const SUBHEX subhex, unsigned int distance)
{
	return NULL;
}

/* NOTE: this has been coded but not tested; don't assume it works right (and beware overflow with very large offset values [although technically that's the fault of hex_space2coord]) */
SUBHEX mapSubhexAtVectorOffset (const SUBHEX subhex, const VECTOR3 * offset)
{
	signed int
		x = 0,
		y = 0;
	hex_space2coord (offset, &x, &y);
	return mapGetRelativePosition (subhex, 1, x, y);
}


/* NOTE: this has been coded but not tested; don't assume it works right */
VECTOR3 mapDistanceFromSubhexCenter (const unsigned char spanLevel, const signed int x, const signed int y)
{
	signed int
		* coords = mapSpanCentres (spanLevel);
	if (coordinatesOverflow (x, y, coords))
		return vectorCreate (0.0, 0.0, 0.0);
	return hex_xyCoord2Space
	(
		x * coords[0] + y * coords[2],
		x * coords[1] + y * coords[3]
	);
}

/***
 * given a set of coordinates (the values x and y), translate the values to a
 * different span level (denoted by relativeSpan) and change the values *xp
 * and *yp to the new coordinates.
 */
bool mapScaleCoordinates (signed char relativeSpan, signed int x, signed int y, signed int * xp, signed int * yp)
{
	signed int
		* centres;
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
	
	if (relativeSpan < 0)
	{
		/* step down in span, which means coordinate values gain resolution
		 * but lose range, and non-zero values get larger
		 */
		centres = mapSpanCentres (abs (relativeSpan));
		if (coordinatesOverflow (x, y, centres))
		{
			ERROR ("\t(tried to shift %d span level%s down)", relativeSpan * -1, relativeSpan == -1 ? "" : "s");
			return FALSE;
		}
		*xp = x * centres[0] + y * centres[2];
		*yp = x * centres[1] + y * centres[3];
		return TRUE;
	}
	else
	{
		// step up in span, which means coordinate values gain range but lose resolution, and non-zero values get smaller and may be truncated.
		centres = mapSpanCentres (relativeSpan);
		// TODO: write this.
	}

	return FALSE;
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
		match = span - 1,
		i;
	//INFO ("%s (%d)", __FUNCTION__, span);
	if (centreCache == NULL)
		centreCache = dynarr_create (span, sizeof (signed int **));
	if ((r = *(signed int **)dynarr_at (centreCache, span - 1)) != NULL)
		return r;
	while (match > 0 && (s = *(signed int **)dynarr_at (centreCache, match)) == NULL)
		match--;
	if (match == 0)
	{
		first = xph_alloc (sizeof (signed int) * 12);
		i = 0;
		while (i < 6)
		{
			hex_centerDistanceCoord (mapRadius, 0, &first[i * 2], &first[i * 2 + 1]);
			i++;
		}
		dynarr_assign (centreCache, match, first);
		match++;
	}
	else
		first = *(signed int **)dynarr_at (centreCache, 0);
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
	}
	r = t;
	return r;
}

static bool coordinatesOverflow (const signed int x, const signed int y, const signed int * const coords)
{
	if (x * coords[0] < x || x * coords[1] < x
		|| y * coords[2] < y || y * coords[3] < y)
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
		sh = sh,
		parent;
	int
		i = strlen (PoleNames);
	parent = (sh->type == HS_SUB)
		? sh->sub.parent
		: sh->hex.parent;
	while (parent != NULL)
	{
		sh = parent;
		parent = (sh->type == HS_SUB)
			? sh->sub.parent
			: sh->hex.parent;
	}
	while (i > 0)
	{
		i--;
		if (Poles[i] == sh)
			return PoleNames[i];
	}
	return '?';
}

SUBHEX subhexData (const SUBHEX subhex, unsigned int offset)
{
	if (subhex->type != HS_SUB)
		return NULL;
	if (offset > hx (mapRadius))
	{
		WARNING ("Can't get offset %d from subhex; maximum offset is %d", offset, hx(mapRadius));
		return NULL;
	}
	if (subhex->sub.data == NULL)
		return NULL;
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
		*xp = subhex->type == HS_SUB
			? subhex->sub.x
			: subhex->hex.x;
	if (yp != NULL)
		*yp = subhex->type == HS_HEX
			? subhex->sub.y
			: subhex->hex.y;
	return TRUE;
}

/* TODO: calculates and returns the vector offset between the two subhexes, from
 * {subhex} to {offset}
 * ALSO TODO: the current outline would perform /outstandingly/ badly on
 * adjacent tiles that happen to be on different sides of a pole edge.
 * calculation would go all the way up to the pole level and thus come out
 * completely imprecise when the real distance would be in the low double
 * digits.
 */
VECTOR3 subhexVectorOffset (const SUBHEX subhex, const SUBHEX offset)
{
	WORLDHEX
		from = subhexGeneratePosition (subhex),
		to = subhexGeneratePosition (subhex),
		* smaller = NULL;
	int
		spanEqualTarget,
		i;
/*
	VECTOR3
		fromDistance,
		toDistance;
*/
	/* traversal has to go AT LEAST until the span levels are equal */
	if (from->spanDepth > to->spanDepth)
	{
		spanEqualTarget = to->spanDepth;
		smaller = &from;
	}
	else
	{
		spanEqualTarget = from->spanDepth;
		smaller = &to;
	}
	while ((*smaller)->spanDepth != spanEqualTarget)
	{
		spanEqualTarget--;
	}

	/* compare the worldhexes to see after which step traversal upwards can
	 * stop e.g., if one is a span 0 hex and the other is a span 1 subdiv, but
	 * they're both on the same span 2 subdiv, then traversal upwards can stop
	 * once both reach span level 2, since their relative distance doesn't
	 * need further calculation.
	 * in fact, further calculation would make the result less precise, since
	 * once the pole level is reached there are significant float resolution
	 * problems, and if the subhexes are actually close together then the
	 * relative error would be unacceptably high.
	 */
	

	/* NOTE: subhex ->x and ->y arrays have the most significant coordinates
	 * first; e.g., the values at [0] are pole level and the values at
	 * [->spanDepth - 1] are at the subhex's highest resolution
	 */
	i = spanEqualTarget;
	while (i > 0)
	{
		i--;

	}
	i = spanEqualTarget;
	while (i > 0)
	{
		i--;

	}
	return vectorCreate (0, 0, 0);
}

/*
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
*/


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
static unsigned char
	AbsoluteViewLimit = 8;
static SUBHEX
	PlayerSubhex = NULL;


void worldSetRenderCacheCentre (SUBHEX origin)
{
	if (subhexSpanLevel (origin) != 1)
		WARNING ("The rendering origin isn't at maximum resolution; this may be a problem", NULL);
	if (RenderCache != NULL)
		dynarr_destroy (RenderCache);
	RenderCache = mapAdjacentSubhexes (origin, AbsoluteViewLimit);
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
	SUBHEX
		sub;
	unsigned int
		tier1Detail = hx (AbsoluteViewLimit / 4),
		tier2Detail = hx (AbsoluteViewLimit / 2),
		tier3Detail = hx (AbsoluteViewLimit),
		i = 0;
	if (RenderCache == NULL)
	{
		ERROR ("Cannot draw map: Render cache is uninitialized.", NULL);
		return;
	}
	while (i < tier1Detail)
	{
		sub = *(SUBHEX *)dynarr_at (RenderCache, i);
		centreOffset = subhexVectorOffset (PlayerSubhex, sub);
		i++;
		if (sub == NULL || subhexSpanLevel (sub) != 1)
			continue;
		subhexDraw (&sub->sub, centreOffset);
	}
	while (i < tier2Detail)
	{
		i++;
	}
	while (i < tier3Detail)
	{
		i++;
	}
}

void subhexDraw (const SUBDIV sub, const VECTOR3 offset)
{
	SUBHEX
		hex;
	unsigned int
		i = 0,
		max = hx (mapRadius);
	while (i < max)
	{
		hex = sub->data[i];
		hexDraw (&hex->hex, offset);
		i++;
	}
}

void hexDraw (const HEX hex, const VECTOR3 centreOffset)
{
	VECTOR3
		hexOffset = hex_xyCoord2Space (hex->x, hex->y),
		totalOffset = vectorAdd (&centreOffset, &hexOffset);
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
}