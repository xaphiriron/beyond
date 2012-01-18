#include "worldgen.h"

#include "bit.h"
#include "system.h"

#include "hex_utility.h"
#include "map.h"
#include "map_internal.h"

#include <time.h>

#include "component_position.h"
#include "comp_arch.h"

// this is very much a makeshift way of addressing arches; it's to be replaced with a general arch list or something??
static Entity
	base;

static Dynarr archOrderFor (SUBHEX platter);

void worldInit ()
{
	static unsigned long
		seed = 0;
	hexPos
		pos;
	FUNCOPEN ();
	
	mapSetSpanAndRadius (4, 8);
	mapGeneratePoles ();

	materialsGenerate ();

	seed = time (NULL);
	INFO ("%s: using seed of \'%ld\'", __FUNCTION__, seed);
	loadSetText ("Initializing...");

	base = entity_create ();
	component_instantiate ("position", base);
	pos = map_randomPos ();
	map_posSwitchFocus (pos, 1);
	position_set (base, pos);
	component_instantiate ("arch", base);

	entity_message (base, NULL, "setArchPattern", patternGet (1));

	FUNCCLOSE ();
}

void worldFinalize ()
{
	hexPos
		centre;
	Entity
		plant;
	SUBHEX
		basePlatter,
		baseHex;
	HEXSTEP
		baseStep;

	FUNCOPEN ();

	centre = position_get (base);
	printf ("loading around %p\n", centre);
	mapLoadAround (centre);
	printf ("...\n");

	map_posSwitchFocus (centre, 1);
	printf ("creating player\n");
	systemCreatePlayer (base);

	basePlatter = hexPos_platter (centre, 1);
	baseHex = subhexData (basePlatter, 0, 0);
	baseStep = hexGroundStepNear (&baseHex->hex, 0);
	plant = entity_create ();
	component_instantiate ("position", plant);
	component_instantiate ("plant", plant);
	entity_refresh (plant);
	position_placeOnHexStep (plant, &baseHex->hex, baseStep);
	//position_set (plant, map_copy (position_get (base)));

//	Entity
//		npc;
// 	npc = entity_create ();
// 	component_instantiate ("position", npc);
// 	component_instantiate ("body", npc);
// 	position_copy (npc, player);
// 	entity_refresh (npc);


	systemClearStates();
	systemPushState (STATE_FREEVIEW);
	FUNCCLOSE ();
}

void worldGenerate (TIMER timer)
{
	static int
		delay = 0;

	int
		i = 0,
		max = fx (MapRadius),
		x, y;

	SUBHEX
		pole[3],
		active;
	pole[0] = mapPole ('r');
	pole[1] = mapPole ('g');
	pole[2] = mapPole ('b');
	mapForceSubdivide (pole[0]);
	mapForceSubdivide (pole[1]);
	mapForceSubdivide (pole[2]);

	i = 0;
	while (i < max)
	{
		hex_unlineate (i, &x, &y);
		active = subhexData (pole[0], x, y);
		mapDataSet (active, "tempAvg", 2048 - ((MapRadius + 1) - hexMagnitude (x, y)) * 2048);
		mapDataSet (active, "tempVar", ((MapRadius + 1) - hexMagnitude (x, y)) * 1024);
		active = subhexData (pole[1], x, y);
		mapDataSet (active, "tempAvg", 2048 - ((MapRadius + 1) - hexMagnitude (x, y)) * 2048);
		mapDataSet (active, "tempVar", ((MapRadius + 1) - hexMagnitude (x, y)) * 1024);
		active = subhexData (pole[2], x, y);
		mapDataSet (active, "tempAvg", 2048 - ((MapRadius + 1) - hexMagnitude (x, y)) * 2048);
		mapDataSet (active, "tempVar", ((MapRadius + 1) - hexMagnitude (x, y)) * 1024);
		i++;
	}

	// pick unexpanded arch at the highest level; expand it; repeat until there are no more arches on that level; repeat from top
	entity_message (base, NULL, "archExpand", NULL);

	loadSetGoal (65536);
	while (!outOfTime (timer) && delay < 65536)
	{
		delay++;
		loadSetLoaded (delay);
	}
	printf ("ran out of time at %d\n", i);
}

/***
 * IMPRINTING FUNCTIONS
 */

void worldImprint (SUBHEX at)
{
	int
		i = 0;
	Entity
		arch;
	Dynarr
		arches;

	if (subhexSpanLevel (at) != 1)
		return;

	arches = archOrderFor (at);
	i = 0;
	while ((arch = *(Entity *)dynarr_at (arches, i++)))
	{
		arch_imprint (arch, at);
	}
	dynarr_destroy (arches);
}


static Dynarr archOrderFor (SUBHEX platter)
{
	Dynarr
		r = dynarr_create (2, sizeof (Entity)),
		nearbyPlatters,
		platterArches;
	static SUBHEX
		* platterList = NULL;
	int
		i = 0,
		j = 0,
		span;
	SUBHEX
		current;
	Entity
		arch;
	hexPos
		pos;
	if (!platterList)
		platterList = xph_alloc (sizeof (SUBHEX *) * (mapGetSpan () + 1));
	// note: if we ever start loading multiple worlds in one game execution frame (as in, loading up a world and then discarding it and loading up a different one) AND we allow for worlds with different span sizes then this becomes buggy broken code since mapGetSpan will change its value between the inital alloc and the memset
	memset (platterList, 0, sizeof (SUBHEX *) * (mapGetSpan () + 1));

	// traverse up the platter hierarchy to pole level
	span = subhexSpanLevel (platter);
	platterList[span] = platter;
	current = platter;
	while (span < mapGetSpan ())
	{
		assert (current != NULL);
		current = subhexParent (current);
		span++;
		platterList[span] = current;
	}

	// from pole down, get all arches and add them to the dynarr
	span = mapGetSpan () + 1;
	while (span > 1)
	{
		current = platterList[--span];
		nearbyPlatters = map_posAround (current, mapGetRadius () - 1);
		i = 0;
		while ((pos = *(hexPos *)dynarr_at (nearbyPlatters, i++)))
		{
			platter = map_posFocusedPlatter (pos);
			if (!platter)
				continue;
			platterArches = subhexGetArches (platter);
			j = 0;
			while ((arch = *(Entity *)dynarr_at (platterArches, j++)))
			{
				dynarr_push (r, arch);
			}
		}
		dynarr_map (nearbyPlatters, (void (*)(void *))map_freePos);
		dynarr_destroy (nearbyPlatters);
	}
	return r;
}
