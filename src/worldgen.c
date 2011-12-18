#include "worldgen.h"

#include "bit.h"
#include "system.h"

#include "hex_utility.h"
#include "map.h"
#include "map_internal.h"

#include <time.h>

#include "component_position.h"

enum worldMaterialList
{
	MATERIAL_AIR,
	MATERIAL_GROUND,
};

static Dynarr
	worldMaterials = NULL;

// this is very much a makeshift way of addressing arches; it's to be replaced with a general arch list or something??
static Entity
	base;

static Dynarr worldArchOrderFor (SUBHEX platter);

void worldInit ()
{
	static unsigned long
		seed = 0;
	hexPos
		pos;
	FUNCOPEN ();
	
	mapSetSpanAndRadius (4, 8);
	mapGeneratePoles ();

	worldMaterials = dynarr_create (3, sizeof (MATSPEC));
	dynarr_assign (worldMaterials, MATERIAL_AIR, makeMaterial (MAT_TRANSPARENT));
	dynarr_assign (worldMaterials, MATERIAL_GROUND, makeMaterial (MAT_OPAQUE));

	seed = time (NULL);
	INFO ("%s: using seed of \'%ld\'", __FUNCTION__, seed);
	loadSetText ("Initializing...");

	base = entity_create ();
	component_instantiate ("position", base);
	pos = map_randomPos ();
	map_posSwitchFocus (pos, 1);
	position_set (base, pos);
	component_instantiate ("worldArch", base);

	FUNCCLOSE ();
}

void worldFinalize ()
{
	hexPos
		centre;
	FUNCOPEN ();

	centre = position_get (base);
	printf ("loading around %p\n", centre);
	mapLoadAround (centre);
	printf ("...\n");

	map_posSwitchFocus (centre, 1);
	printf ("creating player\n");
	systemCreatePlayer (base);

	systemClearStates();
	systemPushState (STATE_FREEVIEW);
	FUNCCLOSE ();
}

void worldGenerate (TIMER timer)
{

	// pick unexpanded arch at the highest level; expand it; repeat until there are no more arches on that level; repeat from top
	entity_message (base, NULL, "archExpand", NULL);

	loadSetGoal (1);
	loadSetLoaded (1);
}

/***
 * IMPRINTING FUNCTIONS
 */

void worldImprint (SUBHEX at)
{
	int
		i = 0,
		j = 0,
		max = fx (mapGetRadius ());
	Entity
		arch;
	Dynarr
		arches;
	hexPos
		archFocus;

	if (subhexSpanLevel (at) != 1)
		return;

	while (i < max)
	{
		hexSetBase (&at->sub.data[i]->hex, 0, *(MATSPEC *)dynarr_at (worldMaterials, MATERIAL_GROUND));
		i++;
	}

	arches = worldArchOrderFor (at);
	i = 0;
	while ((arch = *(Entity *)dynarr_at (arches, i++)))
	{
		printf ("imprinting arch %p on subhex %p\n", arch, at);
		archFocus = position_get (arch);
		
		j = 0;
		while (j < max)
		{
			if (mapDistanceFrom (archFocus, at->sub.data[j]) < 5)
			{
				hexSetBase (&at->sub.data[j]->hex, 18 - mapDistanceFrom (archFocus, at->sub.data[j]) * 3, *(MATSPEC *)dynarr_at (worldMaterials, MATERIAL_GROUND));
			}
			j++;
		}
	}
	dynarr_destroy (arches);
}


static Dynarr worldArchOrderFor (SUBHEX platter)
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
		nearbyPlatters = map_posAround (current, 1);
		i = 0;
		while ((pos = *(hexPos *)dynarr_at (nearbyPlatters, i++)))
		{
			platter = map_posFocusedPlatter (pos);
			if (!platter)
				continue;
			platterArches = subhexGetArches (platter);
			j = 0;
			printf ("platter %p has %d arch(es)\n", platter, dynarr_size (platterArches));
			while ((arch = *(Entity *)dynarr_at (platterArches, j++)))
			{
				printf ("adding arch %p to list\n", arch);
				dynarr_push (r, arch);
			}
		}
		dynarr_map (nearbyPlatters, (void (*)(void *))map_freePos);
		dynarr_destroy (nearbyPlatters);
	}
	return r;
}
