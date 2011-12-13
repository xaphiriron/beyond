#include "map.h"

#include <SDL/SDL_opengl.h>

#include <stdio.h>
#include "xph_memory.h"
#include "xph_log.h"

#include "matrix.h"

#include "hex_utility.h"

#include "map_internal.h"
#include "component_position.h"

SUBHEX
	* Poles = NULL;
char
	* PoleNames = NULL;
int
	PoleCount = 0;

unsigned char
	MapRadius = 8,
	MapSpan = 4;

static bool coordinatesOverflow (const signed int x, const signed int y, const signed int * const coords);

static void subhexInitData (SUBHEX subhex);
static void subhexSetData (SUBHEX subhex, signed int x, signed int y, SUBHEX child);

static struct mapData * mapDataCreate ();
static struct mapData * mapDataCopy (const struct mapData * md);
static void mapDataDestroy (struct mapData * mb);


static void mapCheckLoadStatusAndImprint (void * rel_v);
static void mapBakeHex (HEX hex);

static void map_posRecalcPlatters (hexPos pos);

/***
 * MAP LOADING INTERNAL FUNCTION DECLARATIONS
 */
static void mapQueueLoadAround (SUBHEX origin);
static signed int mapUnloadDistant ();



bool mapSetSpanAndRadius (const unsigned char span, const unsigned char radius)
{
	if (Poles != NULL || PoleNames != NULL)
	{
		ERROR ("Can't switch map size while there's a map loaded; keeping old values of %d/%d (instead of new values of %d/%d)", MapSpan, MapRadius, span, radius);
		return false;
	}
	// FIXME: add a check to make sure radius is a power of two, since things will break if it's not
	MapRadius = radius;
	MapSpan = span;
	return true;
}

unsigned char mapGetSpan ()
{
	return MapSpan;
}

unsigned char mapGetRadius ()
{
	return MapRadius;
}

bool mapGeneratePoles ()
{
	int
		count = 3;
	// blah blah warn/error here if the pole values aren't already null
	Poles = NULL;
	PoleNames = NULL;
	PoleNames = xph_alloc (count + 1);
	strcpy (PoleNames, "rgb");
	PoleCount = count;
	Poles = xph_alloc (sizeof (SUBHEX) * count);
	while (count > 0)
	{
		count--;
		Poles[count] = mapSubdivCreate (NULL, 0, 0);
	}
	
	return false;
}

bool worldExists ()
{
	if (Poles == NULL || PoleNames == NULL)
		return false;
	return true;
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
	while (r <= MapRadius)
	{
		hex_rki2xy (r, k, i, &x, &y);
		/* DEBUG ("radius: %d; xy: %d,%d; rki: %d %d %d; offset: %d; [%d]: &%p/%p", MapRadius, x, y, r, k, i, offset, offset, &subhex->sub.data[offset], subhex->sub.data[offset]); */
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
	if (spanLevel > MapSpan)
	{
		ERROR ("Got meaningless request to grow above the max span level; discarding. (max: %d; request: %d)", MapSpan, spanLevel);
		return;
	}
	currentSpan = subhexSpanLevel (centre);
	while (currentSpan < spanLevel)
	{
		centre = subhexParent (centre);
		if (centre == NULL)
		{
			ERROR ("Unexpectedly hit pole layer (or broken map) at span %d, when it should only occur at %d.", currentSpan, MapSpan);
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
	 * - if (distance + the local coordinate magnitude) > MapRadius we have to
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
		DEBUG ("generating relative hex from centre at offset {%u %u %u} %d, %d", r, k, i, x, y);
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
	DEBUG ("done iterating to grow in %s", __FUNCTION__);
}

void mapForceGrowChildAt (SUBHEX subhex, signed int x, signed int y)
{
	if (subhexData (subhex, x, y) == NULL)
	{
		mapSubdivCreate (subhex, x, y);
	}
}

void mapLoadAround (hexPos pos)
{
	SUBHEX
		platter = NULL;
	int
		i = MapSpan;
	while (i > pos->focus && pos->platter[i])
	{
		platter = pos->platter[i];
		i--;
	}
	i++;
	//printf ("focused on %p at index (span) %d\n", platter, i);
	if (!platter)
		return;

	while (subhexSpanLevel (platter) > pos->focus)
	{
		platter = map_posBestMatchPlatter (pos);
		assert (i >= 0);
		mapForceGrowChildAt (platter, pos->x[i], pos->y[i]);
		//printf ("growing %p (%d) at %d, %d\n", platter, subhexSpanLevel (platter), pos->x[i], pos->y[i]);
		i--;
	}
	//printf ("done\n");
}

/***
 * SIMPLE CREATION AND INITIALIZATION FUNCTIONS
 */

unsigned int
	hexColouring = 0;

SUBHEX mapHexCreate (const SUBHEX parent, signed int x, signed int y)
{
	SUBHEX
		sh;
	unsigned int
		r, k, i;
	if (hexMagnitude (x, y) > MapRadius)
	{
		ERROR ("Can't create subhex child: coordinates given were %d, %d, which are %d step%s out of bounds.", x, y, hexMagnitude (x, y) - MapRadius, hexMagnitude (x, y) - MapRadius == 1 ? "" : "s");
		return NULL;
	}
	sh = xph_alloc (sizeof (union hexOrSubdiv));
	memset (sh, 0, sizeof (union hexOrSubdiv));
	sh->type = HS_HEX;
	sh->hex.parent = parent;
	if (parent == NULL)
		WARNING ("Hex generated without a parent; this is probably a mistake. (It's got a memory address of %p)", sh);
	else
		subhexSetData (parent, x, y, sh);
	sh->hex.x = x;
	sh->hex.y = y;
	hex_xy2rki (x, y, &r, &k, &i);
/*
	sh->hex.light = 0;
	sh->hex.light = hexColouring & ((1 << 6) - 1);
	sh->hex.light ^= (((r > MapRadius / 2) ^ (i % 2 | !r)) * (hexColouring >> 6));
*/
	sh->hex.light = (r > MapRadius / 2) ^ (i % 2 | !r);

	sh->hex.steps = dynarr_create (8, sizeof (HEXSTEP));
	hexSetBase (&sh->hex, 0, NULL);

	return sh;
}

SUBHEX mapSubdivCreate (SUBHEX parent, signed int x, signed int y)
{
	SUBHEX
		sh,
		intHex1,
		intHex2;
	unsigned int
		r = 0, k = 0, i = 0,
		o = 0,
		otherDir,
		intVal1,
		intVal2;
	signed int
		* centres = mapSpanCentres (1);
	VECTOR3
		subdivLocation,
		intLoc1,
		intLoc2;
	Entity
		arch;
	hexPos
		pos;
	struct mapDataEntry
		* mapData = NULL;

	if (hexMagnitude (x, y) > MapRadius)
	{
		ERROR ("Can't create subhex child: coordinates given were %d, %d, which are %d step%s out of bounds.", x, y, hexMagnitude (x, y) - MapRadius, hexMagnitude (x, y) - MapRadius == 1 ? "" : "s");
		return NULL;
	}
	sh = xph_alloc (sizeof (union hexOrSubdiv));
	sh->type = HS_SUB;
	sh->sub.parent = parent;
	sh->sub.x = x;
	sh->sub.y = y;
	sh->sub.data = NULL;

	sh->sub.arches = dynarr_create (2, sizeof (Entity));

	sh->sub.imprintable = false;
	sh->sub.loaded = false;

	if (parent != NULL && parent->type != HS_SUB)
	{
		ERROR ("Given invalid parent (%p); cannot use a hex as a parent of a subdiv!", parent);
		parent = NULL;
	}

	//INFO ("Creating subdiv %p @ %p, %d, %d", sh, parent, x, y);

	if (parent == NULL)
	{
		sh->sub.imprintable = true;
		sh->sub.span = MapSpan;
		sh->sub.mapInfo = mapDataCreate ();
	}
	else
	{
		sh->sub.span = parent->sub.span - 1;
		subhexSetData (parent, x, y, sh);
		sh->sub.mapInfo = mapDataCopy (parent->sub.mapInfo);
		if (!(x == 0 && y == 0))
		{
			// TODO: this should /also/ check the map data vals of the two adjacent subhexes, in the event they have types that should be interpolated
			hex_xy2rki (x, y, &r, &k, &i);
			subdivLocation = hex_xyCoord2Space (x, y);
			if (i < r / 2.0)
				otherDir = (k + 5) % 6;
			else
				otherDir = (k + 1) % 6;
			intHex1 = mapHexAtCoordinateAuto (parent, 0, XY[k][X], XY[k][Y]);
			intHex2 = mapHexAtCoordinateAuto (parent, 0, XY[otherDir][X], XY[otherDir][Y]);
			//printf ("platters: %p, %p, %p\n", parent, intHex1, intHex2);
			intLoc1 = hex_xyCoord2Space (centres[k * 2], centres[k * 2 + 1]);
			intLoc2 = hex_xyCoord2Space (centres[otherDir * 2], centres[otherDir * 2 + 1]);

			o = 0;
			while ((mapData = *(struct mapDataEntry **)dynarr_at (sh->sub.mapInfo->entries, o++)) != NULL)
			{
				if (intHex1)
					intVal1 = mapDataGet (intHex1, mapData->type);
				else
					intVal1 = mapData->value;
				if (intHex2)
					intVal2 = mapDataGet (intHex2, mapData->type);
				else
					intVal2 = mapData->value;
				//printf ("sent {%u %u %u} w/ %d, %d and %d, %d (%d and %d) -- vals %u %u %u\n", r, k, i, XY[k][X], XY[k][Y], XY[otherDir][X], XY[otherDir][Y], k, otherDir, mapData->value, intVal1, intVal2);
				mapData->value = baryInterpolate (&subdivLocation, &intLoc1, &intLoc2, mapData->value, intVal1, intVal2);
				//printf ("  got %u\n", mapData->value);
			}
		}
		// take any arches that
		//  1. are more tightly focused than the resolution of the parent and
		//  2. actually apply to this subhex
		o = 0;
		while ((arch = *(Entity *)dynarr_at (parent->sub.arches, o++)))
		{
			pos = position_get (arch);
			//printf ("updating arch w/ focus level %d; currently at level %d\n", map_posFocusLevel (pos), parent->sub.span);
			map_posRecalcPlatters (pos);
			if (map_posBestMatchPlatter (pos) == sh)
			{
				subhexRemoveArch (parent, arch);
				subhexAddArch (sh, arch);
				//printf ("shifted arch down; was attached to a subdiv on level %d (%p), now attached to its %d,%d child on level %d (%p)\n", parent->sub.span, parent, x, y, sh->sub.span, sh);
			}
		}
	}

	// a span-one subdiv is REQUIRED to have all its hexes generated
	if (sh->sub.span == 1)
	{
		hexColouring = rand ();
		mapForceSubdivide (sh);
	}

	return sh;
}


static void subhexInitData (SUBHEX subhex)
{
	assert (subhex != NULL);
	assert (subhex->type == HS_HEX || subhex->type == HS_SUB);
	if (subhex->type == HS_HEX)
		return;
	/* FIXME: see note in hex_utility.c about hx (); long story short these lines can drop the + 1 once that's fixed */
	subhex->sub.data = xph_alloc (sizeof (SUBHEX) * hx (MapRadius + 1));
	memset (subhex->sub.data, '\0', sizeof (SUBHEX) * hx (MapRadius + 1));
}

static void subhexSetData (SUBHEX subhex, signed int x, signed int y, SUBHEX child)
{
	unsigned int
		r, k, i,
		offset;
	assert (subhex != NULL);
	if (subhex->sub.data == NULL)
		subhexInitData (subhex);
	if (hexMagnitude (x, y) > MapRadius)
	{
		ERROR ("Can't place subhex child: coordinates given were %d, %d, which are %d step%s out of bounds.", x, y, hexMagnitude (x, y) - MapRadius, hexMagnitude (x, y) - MapRadius == 1 ? "" : "s");
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
		max = hx (MapRadius + 1); // FIXME: as usual see hex_utils.c:hx
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
		mapDataDestroy (subhex->sub.mapInfo);
		dynarr_destroy (subhex->sub.arches);
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


static int hexStepSort (const void * a, const void * b);

static int hexStepSort (const void * a, const void * b)
{
	return (*(HEXSTEP *)a)->height - (*(HEXSTEP *)b)->height;
}


	// FIXME: this is such a dummy function; nothing about it works as it ought
HEXSTEP hexSetBase (HEX hex, unsigned int height, MATSPEC material)
{
	HEXSTEP
		base = *(HEXSTEP *)dynarr_front (hex->steps),
		next;
	if (base == NULL)
	{
		return hexCreateStep (hex, height, material);
	}
	base->height = height;
	base->material = material;
	if (dynarr_size (hex->steps) > 1)
	{
		next = *(HEXSTEP *)dynarr_at (hex->steps, 1);
		if (base->height > next->height)
		{
			ERROR ("INVALID BASE HIGHT FOR HEX COLUMN %p; base is %ud while next-highest is %ud; there is no possible way this will end well.", hex, base->height, next->height);
		}
	}
	return base;
}

HEXSTEP hexCreateStep (HEX hex, unsigned int height, MATSPEC material)
{
	HEXSTEP
		prev,
		step = xph_alloc (sizeof (struct hexStep));
	memset (step, 0, sizeof (struct hexStep));
	step->height = height;
	step->material = material;
	
	prev = *(HEXSTEP *)dynarr_back (hex->steps);

	dynarr_push (hex->steps, step);
	dynarr_sort (hex->steps, hexStepSort);
	return step;
}

unsigned char stepParam (HEXSTEP step, const char * param)
{
	if (step == NULL || step->material == NULL)
	{
		if (strcmp (param, "transparent") == 0)
			return true;
		return false;
	}
	return materialParam (step->material, param);
}

/***
 * LOCATIONS
 */

hexPos map_blankPos ()
{
	hexPos
		position = xph_alloc (sizeof (struct xph_world_hex_position));
	int
		spanLevels = mapGetSpan () + 1;
	// span level x goes from pole at 0 to individual hex at x so x + 1 slots are needed (x[0] and y[0] are useless and always 0)
	position->x = xph_alloc (sizeof (int) * spanLevels);
	position->y = xph_alloc (sizeof (int) * spanLevels);
	position->platter = xph_alloc (sizeof (SUBHEX) * spanLevels);
	memset (position->x, 0, sizeof (int) * spanLevels);
	memset (position->y, 0, sizeof (int) * spanLevels);
	memset (position->platter, 0, sizeof (SUBHEX) * spanLevels);
	position->focus = 0;

	return position;
}

hexPos map_randomPos ()
{
	hexPos
		position = map_blankPos ();
	int
		pole = rand () % 3,
		r, k, i,
		x, y,
		radius = mapGetRadius (),
		spanMax = mapGetSpan (),
		span = spanMax;
	
	position->platter[span] = mapPole
	(
		pole == 0
			? 'r'
			: pole == 1
			? 'g'
			: 'b'
	);

	while (span > 0)
	{
		k = i = 0;
		r = rand () % radius;
		if (r > 1)
			i = rand () % (r - 1);
		if (r > 0)
			k = rand () % 6;
		hex_rki2xy (r, k, i, &x, &y);

		position->x[span] = x;
		position->y[span] = y;
		span--;
	}

	position->focus = 0;
	position->from = vectorCreate (0.0, 0.0, 0.0);

	map_posRecalcPlatters (position);

	return position;
}

hexPos map_at (const SUBHEX at)
{
	hexPos
		pos = map_blankPos ();
	int
		x = 0,
		y = 0,
		span;
	SUBHEX
		platter = at;

	pos->focus = subhexSpanLevel (at);
	pos->platter[pos->focus] = at;
	span = subhexSpanLevel (at);
	while (span < MapSpan)
	{
		span++;
		subhexLocalCoordinates (platter, &x, &y);
		platter = subhexParent (platter);
		//printf ("writing to offset %d: %d, %d, %p\n", span, x, y, platter);
		pos->x[span] = x;
		pos->y[span] = y;
		//pos->platter[span] = platter;
	}
	pos->platter[MapSpan] = platter;
	assert (pos->platter[MapSpan] != NULL);
	assert (subhexParent (pos->platter[MapSpan]) == NULL);
	assert (subhexSpanLevel (pos->platter[MapSpan]) == MapSpan);

	map_posRecalcPlatters (pos);

	return pos;
}

hexPos map_from (const SUBHEX at, short relativeSpan, int x, int y)
{
	hexPos
		scratch = NULL;
	SUBHEX
		focus = at;

	short
		span;
/*
	int
		higherX = 0,
		higherY = 0,
		xRemainder = 0,
		yRemainder = 0;
*/

	while (relativeSpan < 0)
	{
		if (!subhexData (focus, 0, 0))
		{
			DEBUG ("Hit unloaded/invalid subdiv while focusing downwards from subhex %p (at span %d)", at, subhexSpanLevel (at));
			scratch = map_at (focus);
			scratch->focus -= relativeSpan;
			assert (scratch->focus <= mapGetSpan ());
			if (scratch->focus > mapGetSpan ())
			{
				ERROR ("Hit tile layer while focusing downwards from subhex %p (at span %d)", at, subhexSpanLevel (at));
				map_freePos (scratch);
				return NULL;
			}
			break;
		}
		focus = subhexData (focus, 0, 0);
		relativeSpan++;
	}
	while (relativeSpan > 0)
	{
		if (!subhexParent (focus))
		{
			ERROR ("Hit pole layer while focusing upwards from subhex %p (at span %d)", at, subhexSpanLevel (at));
			return NULL;
		}
		focus = subhexParent (focus);
		
		relativeSpan--;
	}

	if (!scratch)
		scratch = map_at (focus);
	span = scratch->focus;
	if (span < MapSpan)
	{
		scratch->x[span + 1] += x;
		scratch->y[span + 1] += y;
	}
	else
	{
		scratch->platter[MapSpan] = mapPole (mapPoleTraversal (subhexPoleName (scratch->platter[MapSpan]), x, y));
	}

/*
	// this code block isn't tested
	while (hexMagnitude (scratch->x[span], scratch->y[span]) > MapRadius)
	{
		printf (" - coordinate %d, %d outside map radius; rescaling\n", scratch->x[span], scratch->y[span]);
		mapScaleCoordinates
		(
			1,
			scratch->x[span], scratch->y[span],
			&higherX, &higherY,
			&xRemainder, &yRemainder
		);
		printf ("original: %d, %d; upscaled: %d, %d; remainder: %d, %d\n", scratch->x[span], scratch->y[span], higherX, higherY, xRemainder, yRemainder);
		// this isn't how you actually use the remainder i think - xph 2011 12 10
		scratch->x[span] = xRemainder;
		scratch->y[span] = yRemainder;
		span++;
		if (span == MapSpan)
		{
			scratch->platter[MapSpan] = mapPole (mapPoleTraversal (subhexPoleName (scratch->platter[MapSpan]), higherX, higherY));
			break;
		}
		scratch->x[span] += higherX;
		scratch->y[span] += higherY;
	}
*/

	map_posRecalcPlatters (scratch);
	return scratch;
}

void map_freePos (hexPos pos)
{
	xph_free (pos->x);
	xph_free (pos->y);
	xph_free (pos->platter);
	xph_free (pos);
}

static void map_posRecalcPlatters (hexPos pos)
{
	int
		span = MapSpan;
	while (span > pos->focus)
	{
		pos->platter[span - 1] = pos->platter[span]
			? subhexData (pos->platter[span], pos->x[span], pos->y[span])
			: NULL;
		//printf ("  calculated %p from %p & %d, %d\n", pos->platter[span - 1], pos->platter[span], pos->x[span], pos->y[span]);
		span--;
	}
}


unsigned char map_posFocusLevel (const hexPos pos)
{
	return pos->focus;
}

SUBHEX map_posFocusedPlatter (const hexPos pos)
{
	return pos->platter[pos->focus];
}

SUBHEX map_posBestMatchPlatter (const hexPos pos)
{
	unsigned char
		i = pos->focus;
	while (i <= MapSpan)
	{
		if (pos->platter[i])
			return pos->platter[i];
		i++;
	}
	ERROR ("Position exists with absolutely no loaded platters");
	return NULL;
}

void map_posSwitchFocus (hexPos pos, unsigned char focus)
{
	assert (pos);
	if (focus > MapSpan)
	{
		WARNING ("Attempt to set position focus past the span limits (%d; valid values are 0-%d).", focus, MapSpan);
		return;
	}
	// TODO: if pos->from is set (or even not set since even a 0,0 vector can encode useful position data), scale/move the vector to maintain the same point with the new focus?? maybe??
	pos->focus = focus;
	map_posRecalcPlatters (pos);
}

Dynarr map_posAround (const SUBHEX subhex, unsigned int distance)
{
	Dynarr
		arr = dynarr_create (fx (distance) + 1, sizeof (hexPos));
	unsigned int
		r = 0,
		k = 0,
		i = 0;
	signed int
		x, y;
	while (r <= distance)
	{
		hex_rki2xy (r, k, i, &x, &y);
		//printf (" getting offset {%d %d %d} (%d, %d)\n", r, k, i, x, y);
		dynarr_push (arr, map_from (subhex, 0, x, y));
		hex_nextValidCoord (&r, &k, &i);
	}
	return arr;
}

/***
 * MAP TRAVERSAL FUNCTIONS
 */

bool mapMove (const SUBHEX start, const VECTOR3 * const position, SUBHEX * finish, VECTOR3 * newPosition)
{
	signed int
		x = 0,
		y = 0;
	RELATIVEHEX
		rel;
	SUBHEX
		target = NULL;
	VECTOR3
		newSubhexCentre;
	*finish = NULL;
	*newPosition = vectorCreate (0, 0, 0);
	if (start == NULL || position == NULL)
	{
		ERROR ("Can't calculate map movement; starting subhex (%p) or position vector (%p) are NULL", start, position);
		return false;
	}

	hex_space2coord (position, &x, &y);
	if (hexMagnitude (x, y) <= MapRadius)
	{
		*finish = start;
		*newPosition = *position;
		return true;
	}
	mapScaleCoordinates (1, x, y, &x, &y, NULL, NULL);
	newSubhexCentre = mapDistanceFromSubhexCentre (1, x, y);
	*newPosition = vectorSubtract (position, &newSubhexCentre);

	rel = mapRelativeSubhexWithVectorOffset (start, position);
	target = mapRelativeSpanTarget (rel, 1);
	if (target == NULL)
	{
		*finish = mapRelativeTarget (rel);
		mapRelativeDestroy (rel);
		INFO ("Can't get span-1 fidelity for map movement from %p @ %.2f, %.2f, %.2f; got span-%d instead (addr: %p)", start, position->x, position->y, position->z, subhexSpanLevel (*finish), *finish);
		return false;
	}
	*finish = target;
	mapRelativeDestroy (rel);
	return true;
}




SUBHEX mapPole (const char poleName)
{
	int
		len;
	if (Poles == NULL || PoleNames == NULL)
	{
		WARNING ("Can't get pole: no map exists.");
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
 *
 * (okay this has been 'tested' in that it's a critical part of the current
 * rendering code but that doesn't necessarily mean it doesn't have a critical
 * flaw somewhere)
 *   - xph 2011 07 28
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
 * the coordinateOffset version would have to traverse the world subdiv hierarchy: if (hexMagnitude (trav->localX + x, trav->localY + y) < MapRadius) then the target is within the current subdiv and can easily be gotten. if it's not, convert the trav->...+y coordinate pair one span level up and repeat one step higher on the hierarchy. there's a special case for pole level: pretend the current pole is at 0,0 and take the x,y values plus the one-span-difference values and add those to see which pole in which direction is closest. this is guaranteed to return a coordinate within 2 steps of the origin.
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
		* startX = NULL,
		* startY = NULL,
		* goalX = NULL,
		* goalY = NULL;
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
	VECTOR3
		dirTemp;

	FUNCOPEN ();

	rel->origin = subhex;
	rel->target = NULL;

	// to minorly and uselessly reduce fragmentation this could allocate one address, double the size, and set -> to the halfway point. - xph 2011 06 02
	rel->x = xph_alloc (sizeof (signed int) * (MapSpan + 1));
	rel->y = xph_alloc (sizeof (signed int) * (MapSpan + 1));
	memset (rel->x, 0, sizeof (signed int) * (MapSpan + 1));
	memset (rel->y, 0, sizeof (signed int) * (MapSpan + 1));

	startX = xph_alloc (sizeof (signed int) * (MapSpan + 1) * 4);
	startY = startX + MapSpan + 1;
	goalX = startY + MapSpan + 1;
	goalY = goalX + MapSpan + 1;
	memset (startX, 0, sizeof (signed int) * (MapSpan + 1) * 4);
	/* DEBUG ("allocated %d entr%s for coordinates", abs (spanDiff) + 1, spanDiff == 0 ? "y" : "ies"); */

	cX = x;
	cY = y;
	start = subhex;
	subhexLocalCoordinates (start, &lX, &lY);

	DEBUG ("starting with %p, at local %d, %d, aiming for %d, %d with relative span level %d", start, lX, lY, x, y, relativeSpan);

	rel->minSpan = subhexSpanLevel (subhex);
	rel->maxSpan = rel->minSpan;
	//DEBUG ("native span of %d w/ relative span req. of %d", rel->minSpan, spanDiff);
	if (spanDiff < 0)
		rel->minSpan = rel->maxSpan + spanDiff;
	else
		rel->maxSpan = rel->minSpan + spanDiff;
	//DEBUG ("set min/max span to %d/%d, w/ a range of %d", rel->minSpan, rel->maxSpan, spanRange);

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
			if (start->sub.data == NULL)
				WARNING ("Hit completely unexpanded layer while traversing map; at %p with span %d; wanted %d more level%s (down)", start, start->sub.span, -spanDiff, spanDiff == -1 ? "" : "s");
			else
				WARNING ("Hit incomplete layer while traversing map; at %p with span %d; wanted %d more level%s (down)", start, start->sub.span, -spanDiff, spanDiff == -1 ? "" : "s");
			/* maybe this isn't the best idea in terms of grouping map areas together in terms of what's loaded at what level, or in terms of "traversal doesn't change the state of the world in any way" either, but without this the high-span map view doesn't work in most cases
			 *  - xph 2011 09 05
			 */
			mapForceGrowAtLevelForDistance (start, start->sub.span - 1, 0);
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

	startX[0] = lX;
	startY[0] = lY;
	goalX[0] = cX;
	goalY[0] = cY;

	//DEBUG (" @ %p: local: %d, %d; move: %d, %d; net: %d, %d", start, lX, lY, cX, cY, netX, netY);

	// and now we hit the main bulk of the function: (this requires recursively traversing the subhex hierarchy if subhex edges are passed)

	i = 0;
	goal = NULL;
	while (hexMagnitude (netX, netY) > MapRadius)
	{
		DEBUG ("iterating on %d up the span hierarchy", i);
		//assert (i < spanRange);
		if (subhexSpanLevel (start) == MapSpan)
		{
			INFO ("Hit pole level during traversal (net coordinates: %d, %d)", netX, netY);
			break;
		}
		mapScaleCoordinates (1, netX, netY, &cX, &cY, &xRemainder, &yRemainder);
		DEBUG ("condensed coordinates %d, %d to %d, %d (with remainder %d, %d at index %d)", netX, netY, cX, cY, xRemainder, yRemainder, i);

		rel->x[i] = goalX[i] = xRemainder;
		rel->y[i] = goalY[i] = yRemainder;
		subhexLocalCoordinates (start, &startX[i], &startY[i]);

		start = subhexParent (start);
		subhexLocalCoordinates (start, &lX, &lY);
		netX = lX + cX;
		netY = lY + cY;

		i++;
	}
	DEBUG ("broke from traversal loop with a value of %p", start);
	DEBUG ("current x,y: %d, %d; local:  %d, %d; net: %d, %d", cX, cY, lX, lY, netX, netY);

	rel->x[i] = goalX[i] = netX;
	rel->y[i] = goalY[i] = netY;
	subhexLocalCoordinates (start, &startX[i], &startY[i]);
	if (subhexSpanLevel (start) == MapSpan)
	{
		// at this point, no matter the magnitude, the target can be found since there are no data structures to overrun this high up
		goal = mapPole (mapPoleTraversal (subhexPoleName (start), netX, netY));
		DEBUG ("in pole special case; set goal to %p ('%c'), assuming moving %d, %d from pole '%c'", goal, subhexPoleName (goal), netX, netY, subhexPoleName (start));
	}
	else
	{
		goal = subhexData (subhexParent (start), netX, netY);
	}
	/* TODO: i don't think this [goalMarker] value is strictly necessary; i think the value is effectively stored in the span level of the goal value. maybe? the thing is that since the goal can be calculated from a bunch of different values i don't know if "the number of down-traverse loops" as the right value is strictly correct; it might actually be "the span level of the goal" and then this usage would be doubly wrong
	 * that being said this is used in the goal traversal loop near the bottom to line up the rel->x/y value index to the proper span level
	 *  - xph 2011 06 04
	 */
	/* okay i have no clue what this value represents at all ;_;
	 *  - xph 2011 06 14
	 */
	goalMarker = i;

	DEBUG ("Using %d for goal marker", i);
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
			DEBUG ("down-traversal to goal: level %d @ %d, %d", subhexSpanLevel (goal), rel->x[i], rel->y[i]);
			trav = subhexData (goal, rel->x[i], rel->y[i]);
			DEBUG ("down-traversal yielded %p", trav);
			if (trav == NULL)
			{
				INFO ("Traversal early exit at span %d: perfect-match target doesn't exist;", subhexSpanLevel (goal) - 1);
				break;
			}
			goal = trav;
		}
	}
	rel->target = goal;
	DEBUG ("finished with %p as goal", goal);

	/* we're using a seperate goalX/Y instead of rel->x/y because if we change the rel values it becomes impossible to generate the target subhex from a low-precision RELATIVEHEX, since the rel-> values map directly to subhex local coordinates
	 * also we're doing this calculation seperately from the actual goal traversal step, above, because while we don't care if the goal is at low-resolution, it's pretty vitally important that we always have the coordinate and distance information set for every RELATIVEHEX
	 * - xph 2011 06 04
	 * (as such this code should be spun out into its own function so it can
	 * be reused in other map traversal code)
	 * FIXME: i'm almost certain this will fail when crossing a pole
	 *  - xph 2011 08 09
	 */
	DEBUG ("*** BEGINNING RELATIVE DISTANCE CALCULATIONS (%d) ***", i);
	i = MapSpan - 1;
	while (i > 0)
	{
		mapScaleCoordinates (-1, goalX[i], goalY[i], &lX, &lY, NULL, NULL);

		i--;

		goalX[i] += lX - startX[i];
		goalY[i] += lY - startY[i];
	}

	DEBUG ("*** BEGINNING VECTOR CALCULATIONS (%d) ***", spanRange);
	rel->distance = vectorCreate (0.0, 0.0, 0.0);
	i = 0;
	while (i < spanRange)
	{
		dirTemp = mapDistanceFromSubhexCentre (rel->minSpan + i, goalX[i], goalY[i]);
		DEBUG ("iterating on %d to calculate vector distance (@ span %d: %d, %d); for this step: %.2f, %.2f, %.2f", i, i, goalX[i], goalY[i], dirTemp.x, dirTemp.y, dirTemp.z);
		rel->distance = vectorAdd (&rel->distance, &dirTemp);
		i++;
	}
	xph_free (startX);

	FUNCCLOSE ();
	return rel;
}

RELATIVEHEX mapRelativeSubhexWithSubhex (const SUBHEX start, const SUBHEX goal)
{
	SUBHEX
		sTrav = start,
		gTrav = goal;
	signed int
		x = 0,
		y = 0,
		* startX,
		* startY,
		* goalX,
		* goalY,
		i = 0,
		* pole = NULL;
	unsigned char
		levelSpan;
	RELATIVEHEX
		rel;

	if (start == NULL || goal == NULL)
	{
		ERROR ("Can't calculate subhex offset: passed NULL subhex! (got %p and %p)", start, goal);
		return NULL;
	}

	INFO ("%s: allocating space", __FUNCTION__);

	rel = xph_alloc (sizeof (struct hexRelativePosition));
	rel->origin = start;
	rel->target = goal;
	rel->x = xph_alloc (sizeof (signed int) * (MapSpan + 1));
	rel->y = xph_alloc (sizeof (signed int) * (MapSpan + 1));

	// is this shared memory really necessary
	//     or even correct?
	startX = xph_alloc (sizeof (signed int) * (MapSpan + 1) * 4);
	startY = startX + MapSpan + 1;
	goalX = startY + MapSpan + 1;
	goalY = goalX + MapSpan + 1;
	memset (startX, 0, sizeof (signed int) * (MapSpan + 1) * 4);

	INFO ("%s: attempting to level the start/goal subhexes to the same span level (start: %d; goal: %d)", __FUNCTION__, subhexSpanLevel (sTrav), subhexSpanLevel (gTrav));

	if (subhexSpanLevel (sTrav) < subhexSpanLevel (gTrav))
	{
		levelSpan = subhexSpanLevel (gTrav);
		while (subhexSpanLevel (sTrav) < levelSpan)
		{
			subhexLocalCoordinates (sTrav, &x, &y);
			startX[i] = x;
			startY[i] = y;
			sTrav = subhexParent (sTrav);
			i++;
		}
	}
	else
	{
		levelSpan = subhexSpanLevel (sTrav);
		while (subhexSpanLevel (gTrav) < levelSpan)
		{
			subhexLocalCoordinates (gTrav, &x, &y);
			rel->x[i] = x;
			rel->y[i] = y;
			gTrav = subhexParent (gTrav);
			i++;
		}
	}

	INFO ("%s: attempting to find common parent of subhexes", __FUNCTION__);

	// don't reset [i]; the values already stored in the x/y array are important
	while (gTrav != sTrav)
	{
		INFO (" using %p & %p at span %d and %d", sTrav, gTrav, subhexSpanLevel (sTrav), subhexSpanLevel (gTrav));
		subhexLocalCoordinates (sTrav, &x, &y);
		INFO (" %p: got %d, %d", sTrav, x, y);
		startX[i] = x;
		startY[i] = y;
		subhexLocalCoordinates (gTrav, &x, &y);
		INFO (" %p: got %d, %d", gTrav, x, y);
		rel->x[i] = x;
		rel->y[i] = y;
		if (subhexSpanLevel (sTrav) == MapSpan)
		{
			pole = mapPoleConnections (subhexPoleName (sTrav), subhexPoleName (gTrav));
			i++;
			startX[i] = XY[pole[0]][X];
			startY[i] = XY[pole[0]][Y];
			rel->x[i] = XY[(pole[0] + 3) % 6][X];
			rel->y[i] = XY[(pole[0] + 3) % 6][Y];
			WARNING ("%s: traversal hit pole level; not sure if everything works (got poles '%c' and '%c')", __FUNCTION__, subhexPoleName (sTrav), subhexPoleName (gTrav));
			break;
		}
		sTrav = subhexParent (sTrav);
		gTrav = subhexParent (gTrav);
		i++;
	}

	INFO ("%s: memcpy-ing something???", __FUNCTION__);

	memcpy (goalX, rel->x, sizeof (signed int) * (MapSpan + 1));
	memcpy (goalY, rel->y, sizeof (signed int) * (MapSpan + 1));
	// resolve the relative distance between the two subhexes
	i = subhexSpanLevel (sTrav);
	

	DEBUG ("%s: returning probably-broken relativehex %p", __FUNCTION__, rel);
	return rel;
}


SUBHEX mapHexAtCoordinateAuto (const SUBHEX subhex, const signed short relativeSpan, const signed int x, const signed int y)
{
	RELATIVEHEX
		rel = mapRelativeSubhexWithCoordinateOffset (subhex, relativeSpan, x, y);
	SUBHEX
		target = mapRelativeTarget (rel);
	if (isPerfectFidelity (rel))
	{
		mapRelativeDestroy (rel);
		return target;
	}
	mapRelativeDestroy (rel);
	return NULL;
}


SUBHEX mapRelativeTarget (const RELATIVEHEX relativePosition)
{
	assert (relativePosition != NULL);
	return relativePosition->target;
}

SUBHEX mapRelativeSpanTarget (const RELATIVEHEX relPos, unsigned char span)
{
	SUBHEX
		target;
	assert (relPos != NULL);
	target = relPos->target;
	while (subhexSpanLevel (target) < span)
		target = subhexParent (target);
	if (subhexSpanLevel (target) != span)
	{
		ERROR ("Using relativehex %p; couldn't get span-%d target (lowest target: %d)", relPos, span, subhexSpanLevel (relPos->target));
		return NULL;
	}
	return target;
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



/* FIXME: okay, so, /hypothetically/ this should replace the distance calculations in mapRelativeSubhexWithCoordinateOffset and thus allow for calculating distance in the other mapRelativeSubhexWith* functions. the problem is that the goal coordinates here (:goalCoords) aren't calculated in the same way as the goal coordinates in mapRelative[etc] (:goalX and :goalY there) and I'm not sure if it's possible to calculate them in that way outside of the context of having a coordinate target to aim for. this might just be because I've forgotten how I calculated the goal values in the first place.
 *  - xph 2011 08 09
 */
/*
static signed int * mapCoordinateDistanceStack (const SUBHEX a, const SUBHEX b)
{
	signed int
		* startCoords,
		* goalCoords;
	signed int
		scaledX,
		scaledY;
	unsigned char
		span;
	SUBHEX
		trav;
	startCoords = xph_alloc (sizeof (signed int) * (MapSpan + 1) * 2);
	goalCoords = xph_alloc (sizeof (signed int) * (MapSpan + 1) * 2);

	memset (startCoords, 0, sizeof (signed int) * (MapSpan + 1) * 2);
	memset (goalCoords, 0, sizeof (signed int) * (MapSpan + 1) * 2);

	trav = a;
	span = subhexSpanLevel (a);
	while (span <= MapSpan)
	{
		subhexLocalCoordinates (trav, &startCoords[span * 2], &startCoords[span * 2 + 1]);
		span++;
		trav = subhexParent (trav);
	}

	trav = b;
	span = subhexSpanLevel (b);
	while (span <= MapSpan)
	{
		subhexLocalCoordinates (trav, &goalCoords[span * 2], &goalCoords[span * 2 + 1]);
		span++;
		trav = subhexParent (trav);
	}

	bool
		LOUD = false;
	unsigned int
		sr;

	span = MapSpan - 1;
	while (span > 0)
	{
		if (span >= 2 && (goalCoords[span * 2] != 0 || goalCoords[span * 2 + 1] != 0 || startCoords[span * 2] != 0 || startCoords[span * 2 + 1] != 0))
		{
			LOUD = true;
			WARNING ("LOUD ON at %d,%d vs. %d,%d @ %d", goalCoords[span * 2], goalCoords[span * 2 + 1], startCoords[span * 2], startCoords[span * 2 + 1], span);
			sr = span;
			span = MapSpan;
			WARNING ("\tSTART\t\tGOAL", NULL);
			while (span > 0)
			{
				span--;
				WARNING ("%d\t%d,%d\t\t%d,%d", span, goalCoords[span * 2], goalCoords[span * 2 + 1], startCoords[span * 2], startCoords[span * 2 + 1]);
			}
			span = sr;
		}

		if (!mapScaleCoordinates (-1, goalCoords[span * 2], goalCoords[span * 2 + 1], &scaledX, &scaledY, NULL, NULL))
		{
			// FIXME: this should silently skip over attempting to convert this span level. returning now means the lower-span coordinates are going to be incorrect, though at the distance required for overflow to happen it won't matter much. - xph 2011 08 09
			xph_free (startCoords);
			return goalCoords;
		}
		if (LOUD)
			WARNING ("converting %d,%d @ %d to %d,%d @ %d", goalCoords[span * 2], goalCoords[span * 2 + 1], span, scaledX, scaledY, span-1);
		goalCoords [span * 2] = 0;
		goalCoords [span * 2 + 1] = 0;

		span--;

		if (LOUD)
			WARNING ("accumulating: %d += %d - %d, %d += %d - %d", goalCoords[span * 2], scaledX, startCoords[span * 2], goalCoords[span * 2 + 1], scaledY, startCoords[span * 2 + 1]);
		goalCoords[span * 2] += scaledX - startCoords[span * 2];
		goalCoords[span * 2 + 1] += scaledY - startCoords[span * 2 + 1];

	}

	xph_free (startCoords);
	return goalCoords;
}
*/

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
	if (hexMagnitude (x, y) > MapRadius)
		return true;
	return false;
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
		i = 0,
		failedDirections = 0;
	if (xRemainder)
		*xRemainder = 0;
	if (yRemainder)
		*yRemainder = 0;
	if (xp == NULL || yp == NULL)
		return false;
	else if (relativeSpan == 0)
	{
		*xp = x;
		*yp = y;
		return true;
	}
	else if (x == 0 && y == 0)
	{
		*xp = 0;
		*yp = 0;
		return true;
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
			return false;
		}
		*xp = x * centres[0] + y * centres[2];
		*yp = x * centres[1] + y * centres[3];
		//DEBUG ("downscale values turns %d, %d into %d, %d", x, y, *xp, *yp);
		return true;
	}
	// step up in span, which means coordinate values gain range but lose resolution, and non-zero values get smaller and may be truncated.
	centres = mapSpanCentres (relativeSpan);


	/* there's a slight simplification that i'm not using here since idk how
	 * to put it in a way that actually simplifies the code: if index i can be
	 * subtracted, then either index i + 1 % 6 or index i - 1 % 6 can also be
	 * subtracted, and no other values can. since we iterate from 0..5, if 0
	 * hits the other value may be 1 OR 5; in all other cases if i is a hit
	 * then i + 1 % 6 is the other hit
	 *  - xph 2011-05-29
	 */
	/* see above; there's a categorical way to do this that's not just "keep
	 * going until every direction fails" but i don't know how to write it
	 * yet.
	 *   - xph 2011 08 06
	 */
	while (failedDirections < 6)
	{
		failedDirections = 0;
		i = 0;
		while (i < 6)
		{
			if (hexMagnitude (x - centres[i * 2], y - centres[i * 2 + 1]) < hexMagnitude (x, y))
			{
				x -= centres[i * 2];
				y -= centres[i * 2 + 1];
				scaledX += XY[i][X];
				scaledY += XY[i][Y];
				DEBUG ("scaled to %d, %d w/ rem %d, %d", scaledX, scaledY, x, y);
			}
			else
				failedDirections++;
			i++;
		}
	}

	*xp = scaledX;
	*yp = scaledY;
	if (xRemainder)
		*xRemainder = x;
	if (yRemainder)
		*yRemainder = y;
	if (xRemainder == NULL && yRemainder == NULL && (x || y))
		INFO ("%s: lost %d,%d to rounding error at relative span %d", __FUNCTION__, x, y, relativeSpan);
	return true;
}


/* FIXME: this crashes when an attempt is made to get mapSpanCentres (0)
 * this is mostly useless; the values returned would be the same values as are
 * stored in XY in hex_utility.c. but it should still work right, since it is
 * called that way in practice quite frequently
 *
 * I'M PRETTY SURE I FIXED THIS WHY DIDN'T I UPDATE THIS NOTE???
 *   - XPH 2011 07 29
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

		DEBUG ("generating first centres");
		first = xph_alloc (sizeof (signed int) * 12);
		i = 0;
		while (i < 6)
		{
			hex_centerDistanceCoord (MapRadius, i, &first[i * 2], &first[i * 2 + 1]);
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
		return true;
	}
	return false;
}

/***
 * MAP DATA LAYER FUNCTIONS
 */

static int mapIntrCmp (const void * a, const void * b);
static int mapExtrCmp (const void * k, const void * d);

static struct mapData * mapDataCreate ()
{
	struct mapData
		* md = xph_alloc (sizeof (struct mapData));
	md->entries = dynarr_create (2, sizeof (struct mapDataEntry *));
	return md;
}

static struct mapData * mapDataCopy (const struct mapData * md)
{
	int
		i = 0;
	struct mapData
		* cpy = mapDataCreate ();
	struct mapDataEntry
		* base,
		* cpyEntry;
	while ((base = *(struct mapDataEntry **)dynarr_at (md->entries, i)) != NULL)
	{
		cpyEntry = xph_alloc (sizeof (struct mapDataEntry));
		memcpy (cpyEntry, base, sizeof (struct mapDataEntry));
		dynarr_push (cpy->entries, cpyEntry);
		i++;
	}
	return cpy;
}

static void mapDataDestroy (struct mapData * md)
{
	dynarr_wipe (md->entries, xph_free);
	dynarr_destroy (md->entries);
	xph_free (md);
}

void mapDataSet (SUBHEX at, char * type, signed int amount)
{
	struct mapDataEntry
		* match;
	if (subhexSpanLevel (at) == 0)
	{
		ERROR ("Can't set map data (\"%s\", %d) of specific tile %p", type, amount, at);
		return;
	}
	match = *(struct mapDataEntry **)dynarr_search (at->sub.mapInfo->entries, mapExtrCmp, type);
	if (match == NULL)
	{
		mapDataAdd (at, type, amount);
		return;
	}
	match->value = amount;
}

signed int mapDataAdd (SUBHEX at, char * type, signed int amount)
{
	struct mapDataEntry
		* match;
	if (subhexSpanLevel (at) == 0)
	{
		ERROR ("Can't set map data (\"%s\", %d) of specific tile %p", type, amount, at);
		return 0;
	}
	match = *(struct mapDataEntry **)dynarr_search (at->sub.mapInfo->entries, mapExtrCmp, type);
	if (match == NULL)
	{
		match = xph_alloc (sizeof (struct mapDataEntry));
		strncpy (match->type, type, 32);
		match->value = 0;
		dynarr_push (at->sub.mapInfo->entries, match);
		dynarrSortFinal (at->sub.mapInfo->entries, mapIntrCmp, 1);
	}
	match->value += amount;
	return match->value;
}

signed int mapDataGet (SUBHEX at, char * type)
{
	struct mapDataEntry
		* match;
	if (at == NULL || subhexSpanLevel (at) == 0)
	{
		INFO ("Can't get map data (\"%s\") of specific tile/non-existant subhex %p", type, at);
		return 0;
	}
	match = *(struct mapDataEntry **)dynarr_search (at->sub.mapInfo->entries, mapExtrCmp, type);
	if (match == NULL)
	{
		INFO ("No such map data (\"%s\") exists on subhex %p", type, at);
		return 0;
	}
	return match->value;
}

static int mapIntrCmp (const void * a, const void * b)
{
	//DEBUG ("%s: \"%s\" vs. \"%s\"", __FUNCTION__, (*(struct mapDataEntry **)a)->type, (*(struct mapDataEntry **)b)->type);
	return strcmp ((*(struct mapDataEntry **)a)->type, (*(struct mapDataEntry **)b)->type);
}

static int mapExtrCmp (const void * k, const void * d)
{
	//DEBUG ("%s: \"%s\" vs. \"%s\"", __FUNCTION__, *(char **)k, (*(struct mapDataEntry **)d)->type);
	return strcmp (*(char **)k, (*(struct mapDataEntry **)d)->type);
}


void subhexAddArch (SUBHEX at, Entity arch)
{
	if (!at || subhexSpanLevel (at) < 1)
		return;
	dynarr_push (at->sub.arches, arch);
}

void subhexRemoveArch (SUBHEX at, Entity arch)
{
	if (!at || subhexSpanLevel (at) < 1)
		return;
	dynarr_remove_condense (at->sub.arches, arch);
}

const Dynarr subhexGetArches (const SUBHEX at)
{
	if (!at || subhexSpanLevel (at) < 1)
		return NULL;
	return at->sub.arches;
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
		ERROR ("Can't get pole name for %p: no poles!", subhex);
		return '!';
	}
	else if (subhex == NULL)
	{
		ERROR ("Can't get pole name for %p: NULL subhex", subhex);
		return '?';
	}
	sh = subhex;
	parent = subhexParent (subhex);
	while (parent != NULL)
	{
		sh = parent;
		parent = subhexParent (parent);
	}
	/* DEBUG ("got pole value %p from %p", sh, subhex); */
	i = 0;
	while (i < PoleCount)
	{
		if (Poles[i] == sh)
			return PoleNames[i];
		i++;
	}
	ERROR ("Can't get pole name for %p: ???", subhex);
	return '?';
}

SUBHEX subhexData (const SUBHEX subhex, signed int x, signed int y)
{
	unsigned int
		r, k, i,
		offset = 0;
	if (!subhex || subhex->type != HS_SUB)
		return NULL;
	if (subhex->sub.data == NULL)
		return NULL;
	hex_xy2rki (x, y, &r, &k, &i);
	offset = hex_linearCoord (r, k, i);
	if (offset >= hx (MapRadius + 1))
	{
		WARNING ("Can't get offset %d from subhex; maximum offset is %d", offset, hx (MapRadius + 1));
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
		return false;
	if (xp != NULL)
		*xp = (subhex->type == HS_SUB)
			? subhex->sub.x
			: subhex->hex.x;
	if (yp != NULL)
		*yp = (subhex->type == HS_SUB)
			? subhex->sub.y
			: subhex->hex.y;
	return true;
}

bool subhexPartlyLoaded (const SUBHEX subhex)
{
	SUBHEX
		val = subhex;
	if (subhex == NULL)
		return true;
	if (subhexSpanLevel (val) == 0)
	{
		val = subhexParent (val);
	}
	// TODO: blah blah recurse, but before adding that check make 'imprintable' and 'loaded' mean something for platters w/ span > 1 since right now they're never set except at 1 and pole level
	return !(val->sub.imprintable && val->sub.loaded);
	
}

/* these functions don't really make sense; they could check map data for higher-span subhexes but, just... the way they traverse down to the hex level to get the 'real' height doesn't make a whole lot of sense given the context of the world. or something. */
unsigned int subhexGetRawHeight (const SUBHEX subhex)
{
	SUBHEX
		hex = subhex;
	HEXSTEP
		step;
	while (subhexSpanLevel (hex) != 0)
	{
		hex = subhexData (hex, 0, 0);
		if (hex == NULL)
			return 0.0;
	}
	step = *(HEXSTEP *)dynarr_front (hex->hex.steps);
	return step->height;
}

float subhexGetHeight (const SUBHEX subhex)
{
	SUBHEX
		hex = subhex;
	HEXSTEP
		step;
	while (subhexSpanLevel (hex) != 0)
	{
		hex = subhexData (hex, 0, 0);
		if (hex == NULL)
			return 0.0;
	}
	step = *(HEXSTEP *)dynarr_front (hex->hex.steps);
	return step->height * HEX_SIZE_4;
}

bool hexColor (const HEX hex, unsigned char * rgb)
{
	rgb[0] = 64 +
		!!(hex->light & 0x01) * 127 +
		!!(hex->light & 0x02) * 64;
	rgb[1] = 64 +
		!!(hex->light & 0x04) * 127 +
		!!(hex->light & 0x08) * 64;
	rgb[2] = 64 +
		!!(hex->light & 0x10) * 127 +
		!!(hex->light & 0x20) * 64;
	return true;
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
	depth = MapSpan - subhexSpanLevel (subhex);
	whx->spanDepth = depth;
	if (depth < 0)
	{
		ERROR ("Cannot create worldhex: invalid subhex; has span of %d when map limit is %d; can't create worldhex and also other things are likely going to break soon.", subhexSpanLevel (subhex), MapSpan);
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
 * MAP LOADING FUNCTIONS
 */


static Dynarr
	RenderCache = NULL;
SUBHEX
	RenderOrigin = NULL;
static unsigned char
	AbsoluteViewLimit = 12;

Dynarr
	loadedPlatters = NULL,
	toLoad = NULL;

/*
static void initMapLoad ()
{
	if (loadedPlatters != NULL)
	{
		ERROR ("Can't (re?)load map storage; loaded platter list is already assigned (%p) and not NULL", loadedPlatters);
		return;
	}
	if (toLoad != NULL)
	{
		ERROR ("Can't (re?)load map storage; platter to-load is already assigned (%p) and not NULL", toLoad);
		return;
	}
	loadedPlatters = dynarr_create (12, sizeof (SUBHEX));
	toLoad = dynarr_create (12, sizeof (SUBHEX));
}
*/

static void mapQueueLoadAround (SUBHEX origin)
{
/*
	if (loadedPlatters == NULL)
		initMapLoad ();
*/
	mapForceGrowAtLevelForDistance (origin, 1, 5);
}

/* i feel like this function is ignoring the fact that subhexes will
 * frequently be high-span (not just span-1 at all times) and i /also/ feel
 * like that will doubtless lead to problems when you accidentally unload a
 * pole or something like that
 *  - xph 2011 08 01
 */
static signed int mapUnloadDistant ()
{
/*
	int
		x, y,
		distance = 0,
		i = 0;
	SUBHEX
		subhex = NULL;
	if (loadedPlatters == NULL)
		initMapLoad ();
	while ((subhex = *(SUBHEX *)dynarr_at (toLoad, i++)) != NULL)
	{
		mapStepDistanceBetween (RenderOrigin, subhex, 1, &x, &y);
		distance = hexMagnitude (x, y);
		if (distance > (AbsoluteViewLimit * 2))
		{
			// unload subhex i guess
		}
	}
*/
	return -1;
}

/***
 * RENDERING FUNCTIONS
 */


void worldSetRenderCacheCentre (SUBHEX origin)
{
	if (origin == RenderOrigin)
	{
		INFO ("%s called with repeat origin (%p). already set; ignoring", __FUNCTION__, origin);
		return;
	}
	RenderOrigin = origin;
	//assert (subhexSpanLevel (origin) == 1);
	if (subhexSpanLevel (origin) == 0)
	{
		WARNING ("The rendering origin is a specific tile; using its parent instead");
		origin = subhexParent (origin);
	}
	else if (subhexSpanLevel (origin) != 1)
		ERROR ("The rendering origin isn't at maximum resolution; this may be a problem");
	if (RenderCache != NULL)
	{
		dynarr_wipe (RenderCache, (void (*)(void *))mapRelativeDestroy);
		dynarr_destroy (RenderCache);
	}

	/* the instant map expansion rule:
	 * this ought to add all the affected subhexes to a list to iterate
	 * though, but for now an instant load will suffice.
	 *  - xph 2011 08 01
	 */

	mapQueueLoadAround (origin);

	DEBUG ("remaking render cache");
	RenderCache = mapAdjacentSubhexes (origin, AbsoluteViewLimit);
	DEBUG ("set render cache to %p", RenderCache);

	mapUnloadDistant ();

	/* i really don't know if this is the right place for the imprinting code
	 * -- it obviously has /something/ to do with rendering, but when we're
	 * actually loading and saving platters instead of re-imprinting every
	 * time the platter becomes visible i suspect this will pose a problem.
	 * maybe???
	 *   - xph 2011 07 28
	 */
	dynarr_map (RenderCache, mapCheckLoadStatusAndImprint);
}

static void mapCheckLoadStatusAndImprint (void * rel_v)
{
	SUBHEX
		target = mapRelativeTarget (rel_v),
		adjacent = NULL;
	int
		i = 0,
		loadCount = 0;
	/* higher span platters can still be imprinted and 'loaded' in the sense that their data values can be used to, like, the raycasting bg or something. ultimately they shouldn't be ignored, but for the time being they are */
	if (subhexSpanLevel (target) != 1)
		return;
	if (target->sub.loaded)
		return;
	if (!target->sub.imprintable)
	{
		while (i < 6)
		{
			adjacent = mapHexAtCoordinateAuto (target, 0, XY[i][X], XY[i][Y]);
			if (adjacent != NULL)
				loadCount++;
			i++;
		}
		if (loadCount == 6)
			target->sub.imprintable = true;
		else
			return;
	}
	if (!target->sub.loaded)
	{
		/* loaded needs to be set before the imprinting because the imprinting code checks to see if the hexes it's getting (i.e., nearby hexes it's using to get information about the world) are part of a loaded platter and discards them if they're not */
		target->sub.loaded = true;
		worldImprint (target);
		mapBakeHexes (target);
		i = 0;
		while (i < 6)
		{
			adjacent = mapHexAtCoordinateAuto (target, 0, XY[i][X], XY[i][Y]);
			if (adjacent != NULL && subhexSpanLevel (adjacent) == subhexSpanLevel (target) && adjacent->sub.loaded == true)
			{
				/* FIXME: only some of these need to be updated (based on the i value) but I have no clue which and I'm too lazy to figure it out right now */
				mapBakeEdgeHexes (adjacent, 0);
				mapBakeEdgeHexes (adjacent, 1);
				mapBakeEdgeHexes (adjacent, 2);
				mapBakeEdgeHexes (adjacent, 3);
				mapBakeEdgeHexes (adjacent, 4);
				mapBakeEdgeHexes (adjacent, 5);
			}
			i++;
		}
	}
}

void mapBakeEdgeHexes (SUBHEX subhex, unsigned int dir)
{
	unsigned int
		r = MapRadius,
		i = 0,
		offset;
	if (subhexSpanLevel (subhex) != 1)
		return;
	offset = hex_linearCoord (r, dir, i);
	while (i < MapRadius)
	{
		mapBakeHex (&subhex->sub.data[offset]->hex);
		i++;
		offset++;
	}
}

void mapBakeHexes (SUBHEX subhex)
{
	int
		offset = 0;
	if (subhexSpanLevel (subhex) != 1)
		return;
	while (offset < hx (MapRadius + 1))
	{
		mapBakeHex (&subhex->sub.data[offset]->hex);
		offset++;
	}
}

static void mapBakeHex (HEX hex)
{
	HEXSTEP
		step = NULL,
		adjacent = NULL;
	unsigned int
		dir = 0,
		i = 0,
		adjacentIndex = 0;
	while (dir < 6)
	{
		if (hex->adjacent[dir] == NULL)
			hex->adjacent[dir] = (HEX)mapHexAtCoordinateAuto ((SUBHEX)hex, 0, XY[(dir + 1) % 6][X], XY[(dir + 1) % 6][Y]);
		dir++;
	}
	while ((step = *(HEXSTEP *)dynarr_at (hex->steps, i++)) != NULL)
	{
		dir = 0;
		while (dir < 6)
		{
			if (hex->adjacent[dir] == NULL)
			{
				dir++;
				continue;
			}
			adjacentIndex = dynarr_size (hex->adjacent[dir]->steps);
			while (1)
			{
				adjacentIndex--;
				adjacent = *(HEXSTEP *)dynarr_at (hex->adjacent[dir]->steps, adjacentIndex);
				if (adjacent == NULL || adjacent->height <= step->height)
				{
					step->adjacentHigherIndex[dir] = adjacentIndex++;
					break;
				}
			}
			dir++;
		}
	}
}

void mapDraw (const float const * matrix)
{
	VECTOR3
		centreOffset,
		norm;
	RELATIVEHEX
		rel;
	SUBHEX
		sub;
	unsigned int
		tier1Detail = hx (AbsoluteViewLimit + 1),/*
		tier2Detail = hx (AbsoluteViewLimit / 2),
		tier3Detail = hx (AbsoluteViewLimit),*/
		i = 0;
	float
		facing = matrixHeading (matrix),
		platterAngle = 0,
		diff = 0;
	if (RenderCache == NULL)
	{
		ERROR ("Cannot draw map: Render cache is uninitialized.");
		return;
	}
	glBindTexture (GL_TEXTURE_2D, 0);
	while (i < tier1Detail)
	{
		rel = *(RELATIVEHEX *)dynarr_at (RenderCache, i);
		if (!isPerfectFidelity (rel))
		{
			i++;
			continue;
		}
		centreOffset = mapRelativeDistance (rel);
		sub = mapRelativeTarget (rel);
		if (i > 6)
		{
			norm = vectorNormalize (&centreOffset);
			platterAngle = atan2 (norm.z, norm.x);
			diff = facing - platterAngle;
			if (diff > M_PI)
				diff = (M_PI * -2) + diff;
			if (diff < -M_PI)
				diff = (M_PI * 2) + diff;
			//printf ("render at %.2f, %.2f has angle %f (vs. facing of %f; diff %f)\n", norm.z, norm.x, platterAngle, facing, diff);
			if (fabs (diff - M_PI_2) >= M_PI_2)
			{
				i++;
				continue;
			}
		}

		DEBUG ("trying to render the subhex %d-th in the cache (relativehex val: %p)", i, rel);
		DEBUG (" - centre offset: %f %f %f", centreOffset.x, centreOffset.y, centreOffset.z);
		//DEBUG (" - target: %p", sub);
		i++;
		if (sub == NULL || subhexSpanLevel (sub) != 1 || sub->sub.loaded == false)
		{
			//DEBUG ("skipping value; subhex is NULL (%p) or span level isn't 1 (%d)", sub, sub == NULL ? -1 : subhexSpanLevel (sub));
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
		max = hx (MapRadius + 1);

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
	HEXSTEP
		step = NULL,
		higher = NULL,
		lower = NULL,
		adjacentStep = NULL;
	unsigned char
		rgb[3];
	unsigned int
		corners[6] = {0, 0, 0, 0, 0, 0},
		columnIndex = 0,
		adjacentColumnIndex = 0,
		high[2] = {0, 0};
	signed int
		i, j;

	higher = NULL;
	columnIndex = dynarr_size (hex->steps) - 1;
	step = *(HEXSTEP *)dynarr_at (hex->steps, columnIndex);
	corners[0] = FULLHEIGHT (step, 0);
	corners[1] = FULLHEIGHT (step, 1);
	corners[2] = FULLHEIGHT (step, 2);
	corners[3] = FULLHEIGHT (step, 3);
	corners[4] = FULLHEIGHT (step, 4);
	corners[5] = FULLHEIGHT (step, 5);
	while (step != NULL)
	{
		//printf ("on column %p, #%d; %p\n", hex, columnIndex, step);
		matspecColor (step->material, &rgb[0], &rgb[1], &rgb[2], NULL);
		if (stepParam (step, "visible") != false)
		{
			if (hex->light)
				glColor3ub (rgb[0], rgb[1], rgb[2]);
			else
				glColor3ub (rgb[0] - (rgb[0] >> 4), rgb[1] - (rgb[1] >> 4), rgb[2] - (rgb[2] >> 4));
			if (stepParam (higher, "opaque") == false)
			{
				glBegin (GL_TRIANGLE_FAN);
				glVertex3f (totalOffset.x, step->height * HEX_SIZE_4, totalOffset.z);
				glVertex3f (totalOffset.x + H[0][0], corners[0] * HEX_SIZE_4, totalOffset.z + H[0][1]);
				glVertex3f (totalOffset.x + H[5][0], corners[5] * HEX_SIZE_4, totalOffset.z + H[5][1]);
				glVertex3f (totalOffset.x + H[4][0], corners[4] * HEX_SIZE_4, totalOffset.z + H[4][1]);
				glVertex3f (totalOffset.x + H[3][0], corners[3] * HEX_SIZE_4, totalOffset.z + H[3][1]);
				glVertex3f (totalOffset.x + H[2][0], corners[2] * HEX_SIZE_4, totalOffset.z + H[2][1]);
				glVertex3f (totalOffset.x + H[1][0], corners[1] * HEX_SIZE_4, totalOffset.z + H[1][1]);
				glVertex3f (totalOffset.x + H[0][0], corners[0] * HEX_SIZE_4, totalOffset.z + H[0][1]);
				glEnd ();
			}
		}

		i = 0;
		j = 1;
		lower = *(HEXSTEP *)dynarr_at (hex->steps, --columnIndex);

		if (stepParam (step, "visible") != false)
		{
			if (hex->light)
				glColor3ub (
					rgb[0] - (rgb[0] >> 2),
					rgb[1] - (rgb[1] >> 2),
					rgb[2] - (rgb[2] >> 2)
				);
			else
				glColor3ub (
					rgb[0] - ((rgb[0] >> 2) + (rgb[0] >> 4)),
					rgb[1] - ((rgb[1] >> 2) + (rgb[1] >> 4)),
					rgb[2] - ((rgb[2] >> 2) + (rgb[2] >> 4))
				);

			while (i < 6)
			{
				if (hex->adjacent[i] == NULL || subhexPartlyLoaded ((SUBHEX)hex->adjacent[i]))
				{
					i++;
					j = (i + 1) % 6;
					continue;
				}
				adjacentColumnIndex = dynarr_size (hex->adjacent[i]->steps) - 1;
				high[0] = corners[i];
				high[1] = corners[j];
				while ((adjacentStep = *(HEXSTEP *)dynarr_at (hex->adjacent[i]->steps, adjacentColumnIndex)) != NULL)
				{
					if (adjacentStep->height >= step->height)
					{
						adjacentColumnIndex--;
						continue;
					}
					glBegin (GL_TRIANGLE_STRIP);
					glVertex3f (totalOffset.x + H[i][X], high[0] * HEX_SIZE_4, totalOffset.z + H[i][Y]);
					glVertex3f (totalOffset.x + H[j][X], high[1] * HEX_SIZE_4, totalOffset.z + H[j][Y]);
					if (lower != NULL && adjacentStep->height < lower->height)
					{
						high[0] = FULLHEIGHT (lower, i % 6);
						high[1] = FULLHEIGHT (lower, (i + 1) % 6);
					}
					else
					{
						high[0] = FULLHEIGHT (adjacentStep, (i + 4) % 6);
						high[1] = FULLHEIGHT (adjacentStep, (i + 3) % 6);
					}
					glVertex3f (totalOffset.x + H[i][X], high[0] * HEX_SIZE_4, totalOffset.z + H[i][Y]);
					glVertex3f (totalOffset.x + H[j][X], high[1] * HEX_SIZE_4, totalOffset.z + H[j][Y]);
					glEnd ();
					adjacentColumnIndex--;
				}
				i++;
				j = (i + 1) % 6;
			}

		}

		if (lower == NULL)
			break;
		corners[0] = FULLHEIGHT (lower, 0);
		corners[1] = FULLHEIGHT (lower, 1);
		corners[2] = FULLHEIGHT (lower, 2);
		corners[3] = FULLHEIGHT (lower, 3);
		corners[4] = FULLHEIGHT (lower, 4);
		corners[5] = FULLHEIGHT (lower, 5);
		if (stepParam (lower, "visible") == false)
		{
			if (hex->light)
				glColor3ub (
					rgb[0] - (rgb[0] >> 1),
					rgb[1] - (rgb[1] >> 1),
					rgb[2] - (rgb[2] >> 1)
				);
			else
				glColor3ub (
					rgb[0] - ((rgb[0] >> 1) + (rgb[0] >> 4)),
					rgb[1] - ((rgb[1] >> 1) + (rgb[1] >> 4)),
					rgb[2] - ((rgb[2] >> 1) + (rgb[2] >> 4))
				);
			glBegin (GL_TRIANGLE_FAN);
			glVertex3f (totalOffset.x, lower->height * HEX_SIZE_4, totalOffset.z);
			glVertex3f (totalOffset.x + H[0][X], corners[0] * HEX_SIZE_4, totalOffset.z + H[0][Y]);
			glVertex3f (totalOffset.x + H[1][X], corners[1] * HEX_SIZE_4, totalOffset.z + H[1][Y]);
			glVertex3f (totalOffset.x + H[2][X], corners[2] * HEX_SIZE_4, totalOffset.z + H[2][Y]);
			glVertex3f (totalOffset.x + H[3][X], corners[3] * HEX_SIZE_4, totalOffset.z + H[3][Y]);
			glVertex3f (totalOffset.x + H[4][X], corners[4] * HEX_SIZE_4, totalOffset.z + H[4][Y]);
			glVertex3f (totalOffset.x + H[5][X], corners[5] * HEX_SIZE_4, totalOffset.z + H[5][Y]);
			glVertex3f (totalOffset.x + H[0][X], corners[0] * HEX_SIZE_4, totalOffset.z + H[0][Y]);
			glEnd ();
		}

		higher = step;
		step = lower;
	}
	//FUNCCLOSE ();
}

TEXTURE mapGenerateMapTexture (SUBHEX centre, float facing, unsigned char span)
{
	TEXTURE
		texture;
	VECTOR3
		mapCoords,
		rot;
	SUBHEX
		centreLevel = centre,
		hex;
	unsigned int
		r = 0, k = 0, i = 0;
	signed int
		localX = 0, localY = 0,
		x, y,
		targetSpan = 0,
		* scale;
	unsigned char
		color[3] = {0xff, 0xff, 0xff};

	textureSetBackgroundColor (0x00, 0x00, 0x00, 0x7f);
	texture = textureGenBlank (256, 256, 4);

	subhexLocalCoordinates (centre, &localX, &localY);

	while (targetSpan < (span + 1))
	{
		centreLevel = subhexParent (centreLevel);
		targetSpan++;
	}
	if (centreLevel == NULL)
	{
		ERROR ("Can't construct map texture for span level %d", span);
		return NULL;
	}

	while (r < 16)
	{
		hex_rki2xy (r, k, i, &x, &y);
		//printf ("{%d %d %d} %d, %d\n", r, k, i, x, y);
		hex = mapHexAtCoordinateAuto (centreLevel, -1, localX + x, localY + y);
		//printf ("  got subhex %p\n", hex);
		if (hex == NULL)
		{
			color[0] = 0x00;
			color[1] = 0x00;
			color[2] = 0x00;
		}
		else if (hex->type == HS_HEX)
		{
			hexColor (&hex->hex, color);
		}
		else
		{
			color[0] = mapDataGet (hex, "height") / (float)(1 << 9) * 255;
			color[1] = mapDataGet (hex, "height") / (float)(1 << 9) * 255;
			color[2] = mapDataGet (hex, "height") / (float)(1 << 9) * 255;
		}

		scale = mapSpanCentres (span);
		//printf ("  got %p with %d\n", scale, span);

		mapCoords = hex_xyCoord2Space
		(
			x * scale[0] + y * scale[2],
			x * scale[1] + y * scale[3]
		);
		mapCoords = vectorDivideByScalar (&mapCoords, 52 * hexMagnitude (scale[0], scale[1]));

		rot = mapCoords;
		mapCoords.x = rot.x * cos (-facing) - rot.z * sin (-facing);
		mapCoords.z = rot.x * sin (-facing) + rot.z * cos (-facing);

		mapCoords = vectorMultiplyByScalar (&mapCoords, 14);
		textureSetColor (color[0], color[1], color[2], 0xcf);
		textureDrawHex (texture, vectorCreate (128.0 + mapCoords.x, 128.0 + mapCoords.z, 0.0), 6, -facing);
		//printf ("{%d %d %d} %d, %d (real offset: %d, %d) at %2.2f, %2.2f (span %d) -- %2.2x%2.2x%2.2x / %p\n", r, k, i, x, y, localX + x, localY + y, mapCoords.x, mapCoords.z, span, color[0], color[1], color[2], hex);
		
		hex_nextValidCoord (&r, &k, &i);
	}
	textureBind (texture);

	return texture;
}
