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


static hexPos map_blankPos ();
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
		centre = subhex;
	unsigned char
		currentSpan;
	int
		i = 0;
	hexPos
		pos;
	Dynarr
		around;

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

	i = 0;
	around = map_posAround (centre, distance);
	while ((pos = *(hexPos *)dynarr_at (around, i++)))
	{
		hexPos_forceLoadTo (pos, 1);
	}
	dynarr_wipe (around, (void (*)(void *))map_freePos);
	dynarr_destroy (around);
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
	if (!platter)
		return;

	while (subhexSpanLevel (platter) > pos->focus)
	{
		platter = map_posBestMatchPlatter (pos);
		assert (i >= 0);
		mapForceGrowChildAt (platter, pos->x[i], pos->y[i]);
		i--;
	}
}


bool hexPos_forceLoadTo (hexPos pos, unsigned char span)
{
	int
		activeSpan = MapSpan;
	if (span < pos->focus)
	{
		WARNING ("Loading position to span %d when it only has focus to %d", span, pos->focus);
	}
	while (pos->platter[activeSpan])
		activeSpan--;
	if (activeSpan == MapSpan)
	{
		ERROR ("Cannot load position; given position has no existant platters");
		return false;
	}
	
	while (activeSpan >= span)
	{
		pos->platter[activeSpan] = mapSubdivCreate (pos->platter[activeSpan + 1], pos->x[activeSpan + 1], pos->y[activeSpan + 1]);
		activeSpan--;
	}

	return true;
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

	hexSetBase (&sh->hex, 0, material (MAT_AIR));

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
			map_posRecalcPlatters (pos);
			if (map_posBestMatchPlatter (pos) == sh)
			{
				subhexRemoveArch (parent, arch);
				subhexAddArch (sh, arch);
				// since we just removed the latest entry in the list we're going through, repeat this index over.
				o--;
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
			ERROR ("INVALID BASE HIGHT FOR HEX COLUMN %p; base is %u while next-highest is %u; destroying old data until the column is valid.", hex, base->height, next->height);
			while (1)
			{
				next = *(HEXSTEP *)dynarr_at (hex->steps, 1);
				if (next != NULL && base->height > next->height)
				{
					dynarr_remove_condense (hex->steps, next);
					xph_free (next);
				}
				else
					break;
			}
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

HEXSTEP hexGroundStepNear (const HEX hex, unsigned int height)
{
	HEXSTEP
		step,
		nextStep;
	int
		i = 0;
	assert (hex != NULL);
	nextStep = *(HEXSTEP *)dynarr_at (hex->steps, 0);
	while (i < dynarr_size (hex->steps))
	{
		step = nextStep;
		nextStep = *(HEXSTEP *)dynarr_at (hex->steps, ++i);
		if (step->height <= height && (!nextStep || (nextStep->height >= height && matParam (nextStep->material, "transparent"))))
			return step;
		else if (step->height >= height && (!nextStep || matParam (nextStep->material, "transparent")))
			return step;
	}
	char
		heightstr[16],
		errorbuffer[128];
	errorbuffer[0] = 0;
	i = 0;
	while ((step = *(HEXSTEP *)dynarr_at (hex->steps, i++)))
	{
		snprintf (heightstr, 16, "%d", step->height);
		if (i != 1)
			strncat (errorbuffer, ", ", 128 - (strlen (errorbuffer) + 1));
		strncat (errorbuffer, heightstr, 128 - (strlen (errorbuffer) + 1));
	}
	ERROR ("Couldn't get valid ground step for target %d w/ steps %s", height, errorbuffer);
	return NULL;
}

void stepShiftHeight (HEX hex, HEXSTEP step, signed int shift)
{
	int
		stepIndex = in_dynarr (hex->steps, step);
	unsigned int
		newHeight;
	HEXSTEP
		overlapped;
	assert (hex);
	assert (step);
	assert (stepIndex != -1);

	newHeight = step->height + shift;
	if (shift < 0 && newHeight > step->height) // underflow
		newHeight = 0;
	else if (shift > 0 && newHeight < step->height) // overflow
		newHeight = INT_MAX;
	
	if (newHeight == step->height)
		return;

	if (newHeight > step->height)
	{
		step->height = newHeight;
		while ((overlapped = *(HEXSTEP *)dynarr_at (hex->steps, ++stepIndex)))
		{
			if (overlapped->height <= step->height)
			{
				dynarr_unset (hex->steps, stepIndex);
				xph_free (overlapped);
			}
		}
		dynarr_condense (hex->steps);
	}
	else
	{
		step->height = newHeight;
		while ((overlapped = *(HEXSTEP *)dynarr_at (hex->steps, --stepIndex)))
		{
			if (overlapped->height >= step->height)
			{
				dynarr_unset (hex->steps, stepIndex);
				xph_free (overlapped);
			}
		}
		dynarr_condense (hex->steps);
	}
}

void stepTransmute (HEX hex, HEXSTEP step, MATSPEC material, int penetration)
{
	int
		stepIndex = in_dynarr (hex->steps, step);
	HEXSTEP
		newStep;
	assert (hex);
	assert (step);
	assert (stepIndex != -1);
	assert (penetration > 0);
	
	newStep = xph_alloc (sizeof (struct hexStep));
	newStep->height = step->height;
	newStep->material = material;

	if (step->height - penetration > step->height)
		step->height = 0;
	else
		step->height = step->height - penetration;

	// TODO: error handling if there's a step between the old step and the new step (probably just shift the step down until it no longer overlaps but if there's /another/ step overlap when that happens things get complex and blah blah blah let's not think about it right now - xph 2012 01 07

	dynarr_push (hex->steps, newStep);
	dynarr_sort (hex->steps, hexStepSort);
}

/***
 * LOCATIONS
 */

static hexPos map_blankPos ()
{
	hexPos
		position = xph_alloc (sizeof (struct xph_world_hex_position));
	int
		spanLevels = mapGetSpan () + 1;
	// span level x goes from individual hex at 0 to pole at MapSpan so MapSpan + 1 slots are needed (x[0] and y[0] are useless and always 0, but platter[0] is the individual hex if it's loaded)
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
		displacement,
		x, y,
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

	printf ("position: '%c'\n", 
		pole == 0
			? 'r'
			: pole == 1
			? 'g'
			: 'b');

	// FIXME: this should be span >= 0 since it should generate positions down to hex fidelity by default HOWEVER before that change is made it should be ensured that all the hexPos functions actually work with max fidelity positions (they should but it hasn't been tested since this function doesn't really generate max fidelity positions that are near platter borders) - xph 2011 12 15
	while (span > 0)
	{
		displacement = rand () % fx (MapRadius);
		hex_unlineate (displacement, &x, &y);

		printf (" %d: %d, %d\n", span, x, y);
		position->x[span] = x;
		position->y[span] = y;
		span--;
	}

	position->focus = 0;
	position->from = vectorCreate (0.0, 0.0, 0.0);

	map_posRecalcPlatters (position);

	return position;
}

hexPos map_randomPositionNear (const hexPos base, int range)
{
	hexPos
		pos;
	int
		spanLevels,
		focus,
		oldFocus,
		displacement,
		x = 0,
		y = 0,
		higherX,
		higherY,
		xRemainder,
		yRemainder;

	if (base->focus == MapSpan && range != 0)
	{
		// come on why would you even do this
		return map_randomPos ();
	}
	pos = map_blankPos ();

	spanLevels = MapSpan + 1;
	memcpy (pos->x, base->x, sizeof (int) * spanLevels);
	memcpy (pos->y, base->y, sizeof (int) * spanLevels);
	memcpy (pos->platter, base->platter, sizeof (SUBHEX) * spanLevels);
	focus = base->focus;

	if (range > 0)
	{
		displacement = rand () % fx (range);
		hex_unlineate (displacement, &x, &y);

		pos->x[focus + 1] += x;
		pos->y[focus + 1] += y;
		// check for overflow -- this code is copied from map_from, and maybe it should be turned into its own function if this kind of thing is going to happen a lot - xph 2011 12 24
		oldFocus = focus;
		focus++;
		while (hexMagnitude (pos->x[focus], pos->y[focus]) > MapRadius)
		{
			assert (focus <= MapSpan);
			mapScaleCoordinates
			(
				1,
				pos->x[focus], pos->y[focus],
				&higherX, &higherY,
				&xRemainder, &yRemainder
			);
			pos->x[focus] = xRemainder;
			pos->y[focus] = yRemainder;
			focus++;
			if (focus > MapSpan)
			{
				pos->platter[MapSpan] = mapPole (mapPoleTraversal (subhexPoleName (pos->platter[MapSpan]), higherX, higherY));
				break;
			}
			pos->x[focus] += higherX;
			pos->y[focus] += higherY;
		}
		focus = oldFocus;
	}

	while (focus > 0)
	{
		displacement = rand () % fx (MapRadius);
		hex_unlineate (displacement, &x, &y);

		pos->x[focus] = x;
		pos->y[focus] = y;
		focus--;
	}

	pos->focus = 0;
	pos->from = vectorCreate (0.0, 0.0, 0.0);

	assert (pos->platter[MapSpan] != NULL);
	assert (subhexParent (pos->platter[MapSpan]) == NULL);
	assert (subhexSpanLevel (pos->platter[MapSpan]) == MapSpan);

	map_posRecalcPlatters (pos);

	return pos;
}

hexPos map_copy (const hexPos original)
{
	hexPos
		pos;
	int
		spanLevels = MapSpan + 1;
	if (!original)
		return NULL;
	pos = map_blankPos ();
	memcpy (pos->x, original->x, sizeof (int) * spanLevels);
	memcpy (pos->y, original->y, sizeof (int) * spanLevels);
	memcpy (pos->platter, original->platter, sizeof (SUBHEX) * spanLevels);
	pos->focus = original->focus;
	return pos;
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
		pos->x[span] = x;
		pos->y[span] = y;
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
	int
		higherX = 0,
		higherY = 0,
		xRemainder = 0,
		yRemainder = 0;

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
	if (span == MapSpan)
	{
		// i'm unsure if this actually works right -- shouldn't there be some sort of calculation with the lower-span coordinates to update them? or something? something in some way similar to the hexMagnitude update block below? - xph 2011 12 15
		scratch->platter[MapSpan] = mapPole (mapPoleTraversal (subhexPoleName (scratch->platter[MapSpan]), x, y));
		map_posRecalcPlatters (scratch);
		return scratch;
	}

	span++;

	scratch->x[span] += x;
	scratch->y[span] += y;

	while (hexMagnitude (scratch->x[span], scratch->y[span]) > MapRadius)
	{
		assert (span <= MapSpan);
		mapScaleCoordinates
		(
			1,
			scratch->x[span], scratch->y[span],
			&higherX, &higherY,
			&xRemainder, &yRemainder
		);
		scratch->x[span] = xRemainder;
		scratch->y[span] = yRemainder;
		span++;
		if (span > MapSpan)
		{
			scratch->platter[MapSpan] = mapPole (mapPoleTraversal (subhexPoleName (scratch->platter[MapSpan]), higherX, higherY));
			break;
		}
		scratch->x[span] += higherX;
		scratch->y[span] += higherY;
	}

	map_posRecalcPlatters (scratch);
	return scratch;
}

void map_freePos (hexPos pos)
{
	assert (pos != NULL);
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
		assert (span > 0);
		pos->platter[span - 1] = pos->platter[span]
			? subhexData (pos->platter[span], pos->x[span], pos->y[span])
			: NULL;
		span--;
	}
}


SUBHEX hexPos_platter (const hexPos pos, unsigned char focus)
{
	assert (pos != NULL);
	if (focus < pos->focus)
		WARNING ("Getting the %d-th platter of a postion only focused to %d; (result: %p)", focus, pos->focus, pos->platter[focus]);
	return pos->platter[focus];
}

unsigned char hexPos_focus (const hexPos pos)
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
	int
		x, y,
		i = 0;
	while (i < fx (distance))
	{
		hex_unlineate (i, &x, &y);
		dynarr_push (arr, map_from (subhex, 0, x, y));
		i++;
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

	v2c (position, &x, &y);
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


static signed int
 Connections [][4] =
	{{ 0,  2,  4, -1},
	{ 1,  3,  5, -1},
	{-1, -1, -1, -1}};
const signed int * mapPoleConnections (const char a, const char b)
{
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
				return Connections[0];
			else
				return Connections[1];
			break;
		default:
			break;
	}
	return Connections[2];
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
	RELATIVEHEX
		rel;
	v2c (offset, &x, &y);
	if (x == 0 && y == 0)
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
	// hex_space2coord translates into individual hex coordinates, i.e., span-0 platters.
	span = subhexSpanLevel (subhex);
	DEBUG ("%s: converted %.2f, %.2f, %.2f to coordinates %d, %d at span level 0 (%d relative to the span %d of the subhex %p)", __FUNCTION__, offset->x, offset->y, offset->z, x, y, -span, span, subhex);
	return mapRelativeSubhexWithCoordinateOffset (subhex, -span, x, y);
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

SUBHEX mapHexAtCoordinateAuto (const SUBHEX subhex, const signed short relativeSpan, const signed int x, const signed int y)
{
	hexPos
		at = map_from (subhex, relativeSpan, x, y);
	SUBHEX
		target = map_posFocusedPlatter (at);
	map_freePos (at);
	return target;
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


static Dynarr
	centreCache =  NULL;
signed int * const mapSpanCentres (const unsigned char targetSpan)
{
	signed int
		* r,
		* fill,
		* prev;
	int
		span = 0,
		val = 0,
		nval;

	if (centreCache != NULL)
	{
		r = *(signed int **)dynarr_at (centreCache, targetSpan);
		return r;
	}
	centreCache = dynarr_create (MapSpan + 1, sizeof (signed int **));
	prev = NULL;
	fill = xph_alloc (sizeof (signed int) * 12);
	while (val < 6)
	{
		fill[val * 2] = XY[val][X];
		fill[val * 2 + 1] = XY[val][Y];
		val++;
	}
	dynarr_assign (centreCache, span, fill);
	span++;

	while (span <= MapSpan)
	{
		prev = fill;
		fill = xph_alloc (sizeof (signed int) * 12);
		val = 0;
		while (val < 6)
		{
			nval = val == 5 ? 0 : val + 1;
			fill[val * 2] =
				prev[val * 2] * (MapRadius + 1) +
				prev[nval * 2] * MapRadius;
			fill[val * 2 + 1] =
				prev[val * 2 + 1] * (MapRadius + 1) +
				prev[nval * 2 + 1] * MapRadius;
			val++;
		}
		dynarr_assign (centreCache, span, fill);
		span++;
	}
	r = *(signed int **)dynarr_at (centreCache, targetSpan);
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


void subhexResetLoadStateForNewArch (SUBHEX at)
{
	Dynarr
		around;
	hexPos
		pos;
	int
		i = 0;
	SUBHEX
		active;
	if (!at)
		return;
	around = map_posAround (at, 1);
	while ((pos = *(hexPos *)dynarr_at (around, i++)))
	{
		active = map_posFocusedPlatter (pos);
		if (!active || subhexSpanLevel (active) == 0)
			continue;
		if (active->sub.loaded == true)
		{
			printf ("re-imprinting subhex %p\n", active);
			worldImprint (active);
		}
	}
	dynarr_map (around, (void (*)(void *))map_freePos);
	dynarr_destroy (around);
}

/***
 * COLLISION
 */

static VECTOR3 line_planeIntersection (const VECTOR3 * const lineOrigin, const VECTOR3 * const line, const VECTOR3 * const plane, float dist);

static VECTOR3 line_planeIntersection (const VECTOR3 * const lineOrigin, const VECTOR3 * const line, const VECTOR3 * const plane, float dist)
{
	float
		denom = plane->x * line->x + plane->y * line->y + plane->z * line->z,
		t;
	VECTOR3
		r = vectorCreate (0, 0, 0);
	if (fabs (denom) < FLT_EPSILON)
		return r;	// line is parallel to plane
	t = -(plane->x * lineOrigin->x + plane->y * lineOrigin->y + plane->z * lineOrigin->z + dist) / denom;
	r.x = lineOrigin->x + line->x * t;
	r.y = lineOrigin->y + line->y * t;
	r.z = lineOrigin->z + line->z * t;
	return r;
}


// FIXME: this assumes we're operating on a span 0 platter (i.e., an individual hex) but there aren't any code checks to ensure that
union collide_marker map_lineCollide (const SUBHEX base, const VECTOR3 * local, const VECTOR3 * ray)
{
	union collide_marker
		mark;
	int
		x, y,
		distance = 0,
		i = 0,
		begin,
		side,
		collidingEdge = -1;
	float
		stepHeight,
		rayAngle;
	SUBHEX
		active;
	VECTOR3
		hexCentre,
		hexNormal,
		intersection = vectorCreate (0, 0, 0);
	HEXSTEP
		step;

	mark.type = HIT_NOTHING;

	rayAngle = atan2 (ray->z, ray->x);
	v2c (local, &x, &y);
	active = mapHexAtCoordinateAuto (base, -1, x, y);

	while (distance < 24)
	{
		hexCentre = renderOriginDistance (active);

		i = 0;
		while ((step = *(struct hexStep **)dynarr_at (active->hex.steps, i++)))
		{
			if (matParam (step->material, "transparent"))
				continue;

			if (collidingEdge != -1)
			{
				//intersection = line_planeIntersection (local, ray, ???, ???);
				
			}

			// FIXME: this is a collision against the hex assuming its surface is flat. if the surface isn't flat there will be some inaccuracies
			stepHeight = step->height * HEX_SIZE_4;
			hexNormal = vectorCreate (0, 1, 0);
			intersection = line_planeIntersection (local, ray, &hexNormal, -stepHeight);
			intersection = vectorSubtract (&intersection, &hexCentre);

			if (pointInHex (&intersection))
			{
				mark.type = HIT_SURFACE;
				mark.hex.hex = &active->hex;
				mark.hex.step = step;
				return mark;
			}
		}

		i = 0;
		while (turns (hexCentre.x + H[i][X], hexCentre.z + H[i][Y], local->x, local->z, local->x + ray->x, local->z + ray->z) != LEFT)
		{
			i++;
			if (i > 6)
			{
				ERROR ("collision failed: got hex that didn't intersect with line at all");
				return mark;
			}
		}
		begin = i;
		i = 1;
		while (i < 6)
		{
			side = turns (hexCentre.x + H[(begin + i) % 6][X], hexCentre.z + H[(begin + i) % 6][Y], local->x, local->z, local->x + ray->x, local->z + ray->z);
			if (side == RIGHT)
			{
				// collidingEdge is used in the next go-round to see if there's a join collision
				collidingEdge = (begin + i) % 6;
				active = mapHexAtCoordinateAuto (active, 0, XY[collidingEdge][X], XY[collidingEdge][Y]);
				assert (active != NULL);
				break;
			}
			i++;
		}
		distance++;
	}

	return mark;
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
	assert (subhex != NULL);
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

VECTOR3 mapDistanceBetween (const SUBHEX a, const SUBHEX b)
{
	hexPos
		aPos = map_at (a),
		bPos = map_at (b);
	VECTOR3
		r = mapDistance (aPos, bPos);
	map_freePos (aPos);
	map_freePos (bPos);
	return r;
}

VECTOR3 mapDistance (const hexPos a, const hexPos b)
{
	hexPos
		difference = map_blankPos ();
	int
		focus = a->focus < b->focus
			? a->focus
			: b->focus,
		span;
	VECTOR3
		r = vectorCreate (0.0, 0.0, 0.0);

	// FIXME: i don't actually know if this will work right in all cases. notably: on crossing poles (definitely won't); on maps where there are so many span levels or there's such a large distance that a top-level difference isn't representable as an int when it's scaled to be in span 0 coordinates - xph 2011 12 24
	// using the raw coordinates, get the net distance
	span = MapSpan;
	while (span > focus)
	{
		// this may or may not deal properly with pole crossings. if it does work, it definitely doesn't generate the shortest possible distance, which is what we want.
		if (span == MapSpan && a->platter[span] != b->platter[span])
		{
			const int
				* connections;
			int
				diffs[3][2],
				diffMag[3],
				closest = -1;
			connections = mapPoleConnections (subhexPoleName (b->platter[span]), subhexPoleName (a->platter[span]));
			difference->x[span] = (a->x[span] - b->x[span]);
			difference->y[span] = (a->y[span] - b->y[span]);

			mapScaleCoordinates (-1, XY[connections[0]][X], XY[connections[0]][Y], &diffs[0][0], &diffs[0][1], NULL, NULL);
			diffs[0][0] += difference->x[span];
			diffs[0][1] += difference->y[span];
			diffMag[0] = hexMagnitude (diffs[0][0], diffs[0][1]);

			mapScaleCoordinates (-1, XY[connections[1]][X], XY[connections[1]][Y], &diffs[1][0], &diffs[1][1], NULL, NULL);
			diffs[1][0] += difference->x[span];
			diffs[1][1] += difference->y[span];
			diffMag[1] = hexMagnitude (diffs[1][0], diffs[1][1]);

			mapScaleCoordinates (-1, XY[connections[2]][X], XY[connections[2]][Y], &diffs[2][0], &diffs[2][1], NULL, NULL);
			diffs[2][0] += difference->x[span];
			diffs[2][1] += difference->y[span];
			diffMag[2] = hexMagnitude (diffs[2][0], diffs[2][1]);

			if (diffMag[0] < diffMag[1])
			{
				if (diffMag[0] < diffMag[2])
					closest = 0;
				else
					closest = 2;
			}
			else
			{
				if (diffMag[1] < diffMag[2])
					closest = 1;
				else
					closest = 2;
			}
			difference->x[span] = diffs[closest][0];
			difference->y[span] = diffs[closest][1];
		}
		else
		{
			// add real coordinate distance to the difference on span level [span]
			difference->x[span] += (a->x[span] - b->x[span]);
			difference->y[span] += (a->y[span] - b->y[span]);
		}
		// scale difference up for $span - 1 to carry it over
		mapScaleCoordinates
		(
			-1,
			difference->x[span],
			difference->y[span],
			&difference->x[span - 1],
			&difference->y[span - 1],
			NULL, NULL
		);
		span--;
	}

	// i don't remember why focus + 1 is the correct value but it probably has something to do with how hexPos coordinates are stored on the level above them
	r = mapDistanceFromSubhexCentre (focus, difference->x[focus + 1], difference->y[focus + 1]);

	map_freePos (difference);

	return r;
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

static VECTOR3
	* Distance = NULL,
	* VertexJitter = NULL;

static int green (int n);
//static int red (int n);
static unsigned int vertex (int x, int y, int v);

void worldSetRenderCacheCentre (SUBHEX origin)
{
	int
		i = 0;
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
		dynarr_wipe (RenderCache, (void (*)(void *))map_freePos);
		dynarr_destroy (RenderCache);
	}

	/* the instant map expansion rule:
	 * this ought to add all the affected subhexes to a list to iterate
	 * though, but for now an instant load will suffice.
	 *  - xph 2011 08 01
	 */

	mapQueueLoadAround (origin);

	RenderCache = map_posAround (origin, AbsoluteViewLimit);

	/* this is painfully clever and yet somehow still weirdly constructed. so, as it turns out, no matter what the origin is or what other platters are being drawn, the actual offsets from the origin will be constant, because the map is laid out on a regular hexagonal grid. therefore, it's possible to calculate these platter offsets once and then use them forever afterward. this doesn't need real subhexes or even real hexposes to calculate the offsets, it's just i had already written the mapDistance function and there are all these hexPos values on hand already, so might as well put it here. it'd be more reasonable for this to be placed in some worldgen pre-render system, once i get to making the world map rendering code all systemic. - xph 2011 12 24*/
	if (Distance == NULL)
	{
		hexPos
			originPos,
			cacheEntry;
		float
			rad;
		originPos = map_at (RenderOrigin);
		Distance = xph_alloc (sizeof (VECTOR3) * fx (AbsoluteViewLimit));
		while ((cacheEntry = *(hexPos *)dynarr_at (RenderCache, i)))
		{
			Distance[i] = mapDistance (cacheEntry, originPos);
			i++;
		}
		map_freePos (originPos);
		VertexJitter = xph_alloc (sizeof (VECTOR3) * green (MapRadius + 1));
		i = 0;
		while (i < green (MapRadius + 1))
		{
			rad = ((float)rand () / RAND_MAX) * M_PI;
			VertexJitter[i].x = sin (rad) * 8.0;
			VertexJitter[i].z = cos (rad) * 8.0;
			//printf ("%d: %f, %f\n", i, VertexJitter[i].x, VertexJitter[i].z);
			i++;
		}
	}

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

VECTOR3 renderOriginDistance (const SUBHEX hex)
{
	return mapDistanceBetween (hex, RenderOrigin);
}

static void mapCheckLoadStatusAndImprint (void * rel_v)
{
	SUBHEX
		target,
		adjacent = NULL;
	int
		i = 0,
		loadCount = 0;
	hexPos
		pos;
	assert (rel_v != NULL);
	pos = rel_v;
	target = map_posFocusedPlatter (pos);
	/* higher span platters can still be imprinted and 'loaded' in the sense that their data values can be used to, like, the raycasting bg or something. ultimately they shouldn't be ignored, but for the time being they are */
	if (!target || subhexSpanLevel (target) != 1)
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

static int
	__e = 0;
#define EDGE_HIGHER(dir,edge,higher)		(__e = (dir), FULLHEIGHT (higher, (__e + 4) % 6) > FULLHEIGHT (edge, __e) || FULLHEIGHT (higher, (__e + 3) % 6) > FULLHEIGHT (edge, (__e + 1) % 6))

static void mapBakeHex (HEX hex)
{
	HEX
		adjHex = NULL;
	HEXSTEP
		lastStep = NULL,
		step = NULL,
		nextStep = NULL,
		adjStep = NULL,
		lastAdjStep = NULL;
	unsigned int
		dir = 0,
		i, j,
		high[2] = {0, 0},
		low[2] = {0, 0},
		* join;
	bool
		lastStepTransparent = true,
		lastAdjStepTransparent = true,
		validJoin = false;
	while (dir < 6)
	{
		if (hex->adjacent[dir] == NULL)
			hex->adjacent[dir] = (HEX)mapHexAtCoordinateAuto ((SUBHEX)hex, 0, XY[(dir + 1) % 6][X], XY[(dir + 1) % 6][Y]);
		dir++;
	}

	i = dynarr_size (hex->steps);
	while ((step = *(HEXSTEP *)dynarr_at (hex->steps, --i)) != NULL)
	{
		nextStep = *(HEXSTEP *)dynarr_at (hex->steps, i - 1);

		step->info.jit[0] = &VertexJitter [vertex (hex->x, hex->y, 1)];
		step->info.jit[1] = &VertexJitter [vertex (hex->x, hex->y, 2)];
		step->info.jit[2] = &VertexJitter [vertex (hex->x, hex->y, 3)];
		step->info.jit[3] = &VertexJitter [vertex (hex->x, hex->y, 4)];
		step->info.jit[4] = &VertexJitter [vertex (hex->x, hex->y, 5)];
		step->info.jit[5] = &VertexJitter [vertex (hex->x, hex->y, 0)];

		if (!lastStepTransparent && matParam (step->material, "transparent"))
			lastStep->info.undersideVisible = true;
		if (lastStepTransparent && !matParam (step->material, "transparent"))
			step->info.surfaceVisible = true;

		if (matParam (step->material, "transparent"))
		{
			lastStepTransparent = true;
			lastStep = step;
			continue;
		}

		dir = 0;
		while (dir < 6)
		{
			// do visible join calc, which has a theoretical upper limit somewhere around MAX_INT / 2 quads but in practice will usually be around one (each dynarr entry is four unsigned ints that make up a join quad)
			if (step->info.visibleJoin[dir])
				dynarr_wipe (step->info.visibleJoin[dir], xph_free);
			else
				step->info.visibleJoin[dir] = dynarr_create (2, sizeof (unsigned int *));
			adjHex = hex->adjacent[dir];
			lastAdjStep = NULL;
			lastAdjStepTransparent = true;
			j = dynarr_size (adjHex->steps);
			while ((adjStep = *(HEXSTEP *)dynarr_at (adjHex->steps, --j)))
			{
				validJoin = false;
				// FIXME: i think all these uses of EDGE_HIGHER might be wrong since it's very much a directional test but i don't remember which vertices are being checked -- it should be step dir & dir + 1 vs adjacent dir + 4 & and + 3 but i haven't checked because i'm dumb - xph 2012 01 11
				if (EDGE_HIGHER (dir, step, adjStep))
				{
					lastAdjStepTransparent = matParam (adjStep->material, "transparent");
					lastAdjStep = adjStep;
					continue;
				}

				if (lastAdjStep && !lastAdjStepTransparent && FULLHEIGHT (lastAdjStep, (dir + 4) % 6) > FULLHEIGHT (step, dir))
					high[0] = FULLHEIGHT (lastAdjStep, (dir + 4) % 6);
				else
					high[0] = FULLHEIGHT (step, dir);


				if (lastAdjStep && !lastAdjStepTransparent && FULLHEIGHT (lastAdjStep, (dir + 3) % 6) > FULLHEIGHT (step, (dir + 1) % 6))
					high[1] = FULLHEIGHT (lastAdjStep, (dir + 3) % 6);
				else
					high[1] = FULLHEIGHT (step, (dir + 1) % 6);


				if (lastAdjStepTransparent && nextStep && !EDGE_HIGHER (dir, nextStep, adjStep))
				{
					low[0] = FULLHEIGHT (nextStep, dir);
					low[1] = FULLHEIGHT (nextStep, (dir + 1) % 6);
					validJoin = true;
				}
				else if (lastAdjStepTransparent && !EDGE_HIGHER (dir, step, adjStep))
				{
					low[0] = FULLHEIGHT (adjStep, (dir + 4) % 6);
					low[1] = FULLHEIGHT (adjStep, (dir + 3) % 6);
					validJoin = true;
				}

				if (validJoin && (high[0] != low[0] || high[1] != low[1]))
				{
					join = xph_alloc (sizeof (unsigned int) * 4);
					join[0] = high[0];
					join[1] = high[1];
					join[2] = low[0];
					join[3] = low[1];
					dynarr_push (step->info.visibleJoin[dir], join);
				}

				lastAdjStepTransparent = matParam (adjStep->material, "transparent");
				lastAdjStep = adjStep;
			}
			dir++;
		}

		lastStepTransparent = false;
		lastStep = step;
	}
}

void mapDraw (const float const * matrix)
{
	VECTOR3
		centreOffset,
		norm;
	hexPos
		pos;
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
		pos = *(hexPos *)dynarr_at (RenderCache, i);
		sub = map_posFocusedPlatter (pos);
		if (!sub)
		{
			i++;
			continue;
		}
		centreOffset = Distance[i];
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

		DEBUG ("trying to render the subhex %d-th in the cache (hexPos val: %p)", i, pos);
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

void drawHexSurface (const struct hexColumn * const hex, const HEXSTEP step, const VECTOR3 * const render, enum map_draw_types style)
{
	unsigned int
		corner[6];
	unsigned char
		rgba[4];
	
	corner[0] = FULLHEIGHT (step, 0);
	corner[1] = FULLHEIGHT (step, 1);
	corner[2] = FULLHEIGHT (step, 2);
	corner[3] = FULLHEIGHT (step, 3);
	corner[4] = FULLHEIGHT (step, 4);
	corner[5] = FULLHEIGHT (step, 5);

	switch (style)
	{
		case DRAW_HIGHLIGHT:
			glDisable (GL_DEPTH_TEST);
			glColor4ub (0x00, 0x99, 0xff, 0x7f);
			break;
		case DRAW_NORMAL:
		default:
			matspecColor (step->material, &rgba[0], &rgba[1], &rgba[2], &rgba[3]);
			if (hex->light)
				glColor4ub (rgba[0], rgba[1], rgba[2], rgba[3]);
			else
				glColor4ub (rgba[0] - (rgba[0] >> 4), rgba[1] - (rgba[1] >> 4), rgba[2] - (rgba[2] >> 4), rgba[3]);
			break;
	}

	glBegin (GL_TRIANGLE_FAN);
	glVertex3f (render->x, step->height * HEX_SIZE_4, render->z);
	glVertex3f (render->x + H[0][0] + step->info.jit[0]->x, corner[0] * HEX_SIZE_4, render->z + H[0][1] + step->info.jit[0]->z);
	glVertex3f (render->x + H[5][0] + step->info.jit[5]->x, corner[5] * HEX_SIZE_4, render->z + H[5][1] + step->info.jit[5]->z);
	glVertex3f (render->x + H[4][0] + step->info.jit[4]->x, corner[4] * HEX_SIZE_4, render->z + H[4][1] + step->info.jit[4]->z);
	glVertex3f (render->x + H[3][0] + step->info.jit[3]->x, corner[3] * HEX_SIZE_4, render->z + H[3][1] + step->info.jit[3]->z);
	glVertex3f (render->x + H[2][0] + step->info.jit[2]->x, corner[2] * HEX_SIZE_4, render->z + H[2][1] + step->info.jit[2]->z);
	glVertex3f (render->x + H[1][0] + step->info.jit[1]->x, corner[1] * HEX_SIZE_4, render->z + H[1][1] + step->info.jit[1]->z);
	glVertex3f (render->x + H[0][0] + step->info.jit[0]->x, corner[0] * HEX_SIZE_4, render->z + H[0][1] + step->info.jit[0]->z);
	glEnd ();

	if (style == DRAW_HIGHLIGHT)
	{
		glEnable (GL_DEPTH_TEST);
	}
}

void drawHexUnderside (const struct hexColumn * const hex, const HEXSTEP step, MATSPEC material, const VECTOR3 * const render, enum map_draw_types style)
{
	unsigned int
		corner[6];
	unsigned char
		rgba[4];

	corner[0] = FULLHEIGHT (step, 0);
	corner[1] = FULLHEIGHT (step, 1);
	corner[2] = FULLHEIGHT (step, 2);
	corner[3] = FULLHEIGHT (step, 3);
	corner[4] = FULLHEIGHT (step, 4);
	corner[5] = FULLHEIGHT (step, 5);

	switch (style)
	{
		case DRAW_HIGHLIGHT:
			glDisable (GL_DEPTH_TEST);
			glColor4ub (0x00, 0x99, 0xff, 0x7f);
			break;
		case DRAW_NORMAL:
		default:
			matspecColor (material, &rgba[0], &rgba[1], &rgba[2], &rgba[3]);
			if (hex->light)
				glColor4ub (rgba[0], rgba[1], rgba[2], rgba[3]);
			else
				glColor4ub (rgba[0] - (rgba[0] >> 4), rgba[1] - (rgba[1] >> 4), rgba[2] - (rgba[2] >> 4), rgba[3]);
			break;
	}

	glBegin (GL_TRIANGLE_FAN);
	glVertex3f (render->x, step->height * HEX_SIZE_4, render->z);
	glVertex3f (render->x + H[0][0] + step->info.jit[0]->x, corner[0] * HEX_SIZE_4, render->z + H[0][1] + step->info.jit[0]->z);
	glVertex3f (render->x + H[1][0] + step->info.jit[1]->x, corner[1] * HEX_SIZE_4, render->z + H[1][1] + step->info.jit[1]->z);
	glVertex3f (render->x + H[2][0] + step->info.jit[2]->x, corner[2] * HEX_SIZE_4, render->z + H[2][1] + step->info.jit[2]->z);
	glVertex3f (render->x + H[3][0] + step->info.jit[3]->x, corner[3] * HEX_SIZE_4, render->z + H[3][1] + step->info.jit[3]->z);
	glVertex3f (render->x + H[4][0] + step->info.jit[4]->x, corner[4] * HEX_SIZE_4, render->z + H[4][1] + step->info.jit[4]->z);
	glVertex3f (render->x + H[5][0] + step->info.jit[5]->x, corner[5] * HEX_SIZE_4, render->z + H[5][1] + step->info.jit[5]->z);
	glVertex3f (render->x + H[0][0] + step->info.jit[0]->x, corner[0] * HEX_SIZE_4, render->z + H[0][1] + step->info.jit[0]->z);
	glEnd ();

	if (style == DRAW_HIGHLIGHT)
	{
		glEnable (GL_DEPTH_TEST);
	}
}

void drawHexEdge (const struct hexColumn * const hex, const HEXSTEP step, unsigned int high1, unsigned int high2, unsigned int low1, unsigned int low2, int direction, const VECTOR3 * const render, enum map_draw_types style)
{
	int
		nextdir = direction == 5 ? 0 : direction + 1;
	unsigned char
		rgba[4];
	VECTOR3
		jit[2];

	jit[0] = *step->info.jit[direction];
	jit[1] = *step->info.jit[nextdir];

	switch (style)
	{
		case DRAW_HIGHLIGHT:
			glColor4ub (0x00, 0x99, 0xff, 0x7f);
			break;
		case DRAW_NORMAL:
		default:
			matspecColor (step->material, &rgba[0], &rgba[1], &rgba[2], &rgba[3]);
			if (hex->light)
				glColor4ub (
					rgba[0] - (rgba[0] >> 2),
					rgba[1] - (rgba[1] >> 2),
					rgba[2] - (rgba[2] >> 2),
					rgba[3]
				);
			else
				glColor4ub (
					rgba[0] - ((rgba[0] >> 2) + (rgba[0] >> 4)),
					rgba[1] - ((rgba[1] >> 2) + (rgba[1] >> 4)),
					rgba[2] - ((rgba[2] >> 2) + (rgba[2] >> 4)),
					rgba[3]
				);
			break;
	}
	glBegin (GL_TRIANGLE_STRIP);
	glVertex3f
	(
		render->x + H[direction][X] + jit[0].x,
		high1 * HEX_SIZE_4,
		render->z + H[direction][Y] + jit[0].z
	);
	glVertex3f
	(
		render->x + H[nextdir][X] + jit[1].x,
		high2 * HEX_SIZE_4,
		render->z + H[nextdir][Y] + jit[1].z
	);
	glVertex3f
	(
		render->x + H[direction][X] + jit[0].x,
		low1 * HEX_SIZE_4,
		render->z + H[direction][Y] + jit[0].z
	);
	glVertex3f
	(
		render->x + H[nextdir][X] + jit[1].x,
		low2 * HEX_SIZE_4,
		render->z + H[nextdir][Y] + jit[1].z
	);
	glEnd ();
}




void hexDraw (const HEX hex, const VECTOR3 centreOffset)
{
	VECTOR3
		hexOffset = hex_xyCoord2Space (hex->x, hex->y),
		totalOffset = vectorAdd (&centreOffset, &hexOffset);
	HEXSTEP
		step;
	int
		column = dynarr_size (hex->steps),
		dir,
		joins;
	unsigned int
		* joinHeight;

	while ((step = *(HEXSTEP *)dynarr_at (hex->steps, --column)))
	{
		if (step->info.undersideVisible)
			drawHexUnderside (hex, *(HEXSTEP *)dynarr_at (hex->steps, column - 1), step->material, &totalOffset, DRAW_NORMAL);
		if (step->info.surfaceVisible)
			drawHexSurface (hex, step, &totalOffset, DRAW_NORMAL);

		if (!matParam (step->material, "transparent"))
		{
			dir = 0;
			while (dir < 6)
			{
				joins = 0;
				while ((joinHeight = *(unsigned int **)dynarr_at (step->info.visibleJoin[dir], joins++)))
				{
					drawHexEdge (hex, step, joinHeight[0], joinHeight[1], joinHeight[2], joinHeight[3], dir, &totalOffset, DRAW_NORMAL);
				}
				dir++;
			}
		}
		
	}
}


static int green (int n)
{
	return 6 * (n * n);
}

// static int red (int n)
// {
// 	return green (n - 1) + 3 * (2 * n - 1);
// }

static unsigned int vertex (int x, int y, int v)
{
	unsigned int
		r, k, i,
		vertex;
	int
		diff;
	if (x == 0 && y == 0)
		return v;
	if (hexMagnitude (x, y) > MapRadius)
		return 0;
	hex_xy2rki (x, y, &r, &k, &i);
	diff = (v - (signed int)k > -2)
		? v - (signed int)k
		: 6 + (v - (signed int)k);
	if (i == 0 && k == 0 && v == 5)
		vertex = green (r + 1) - 1;
	else if (i == r - 1 && k == 5 && v == 2)
		vertex = green (r - 1);
	else if (diff == 3 || diff == 4 || (i != 0 && (diff == 5 || diff == -1)))
	{
		diff = (diff == -1) ? 5 : diff;
		vertex = green (r - 1) + k * (2 * r - 1) + (i - 1) * 2 + 3 + (3 - diff);
	}
	else
		vertex = green (r) + k * (2 * (r + 1) - 1) + i * 2 + diff;
	if (vertex >= green (MapRadius))
	{
		/* there has got to be a better way to do the border math than this. :( - xph 2012 12 27
		 * okay the idea here is it's possible to have three rows of border jitter values, except that the k == [0, 2, 4], i == 0, diff == 0 vertices all have to match since that's the vertex shared between three different platters. in practice this doesn't matter at all, since even reusing the same exact jitter value for the entire border isn't really that noticable. - xph 2012 12 27 (slightly later)
		printf ("original: %d ..", vertex);
		if (diff == 0 && k == 5 && i == 0)
			vertex = green (MapRadius) + (2 * r - 1);
		else if (k == 2 || (k == 5 && !(i == 0 && diff == -1)) || (k == 4 && i == 0 && diff == 0))
			vertex = green (MapRadius);
		else
		{
			vertex = green (MapRadius) + (k - 2) * (2 * r - 1) - (i * 2 + diff);
		}
		printf (" %d (g: %d)\n", vertex, green (MapRadius));
		*/
		vertex = green (MapRadius);
	}
	assert (vertex < green (MapRadius + 2));
	return vertex;
}

/***
 * ENTITY/SYSTEM/COMPONENT CODE HERE
 */

void mapLoad_system (Dynarr entities)
{
	if (!worldExists ())
		return;
	
}
