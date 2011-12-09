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

static Entity
	base;

static Dynarr worldArchOrderFor (SUBHEX platter);

void worldInit ()
{
	static unsigned long
		seed = 0;
	FUNCOPEN ();
	
	mapSetSpanAndRadius (2, 8);
	mapGeneratePoles (POLE_TRI);

	worldMaterials = dynarr_create (3, sizeof (MATSPEC));
	dynarr_assign (worldMaterials, MATERIAL_AIR, makeMaterial (MAT_TRANSPARENT));
	dynarr_assign (worldMaterials, MATERIAL_GROUND, makeMaterial (MAT_OPAQUE));

	seed = time (NULL);
	INFO ("%s: using seed of \'%ld\'", __FUNCTION__, seed);
	loadSetText ("Initializing...");

	base = entity_create ();
	component_instantiate ("position", base);
	position_set (base, position_random ());
	position_alignToLevel (base, mapGetSpan () - 1);
	component_instantiate ("worldArch", base);

	FUNCCLOSE ();
}

void worldFinalize ()
{
	int
		i = 1,
		span = mapGetSpan ();
	hexPos
		centre;
	SUBHEX
		platter;
	FUNCOPEN ();


	position_alignToLevel (base, mapGetSpan () - 1);
	centre = position_get (base);
	platter = centre->platter[0];
	while (i <= span)
	{
		mapForceGrowChildAt (platter, centre->x[i], centre->y[i]);
		platter = mapHexAtCoordinateAuto (platter, -1, centre->x[i], centre->y[i]);
		centre->platter[i] = platter;
		//printf ("generated platter %p at level %d (%d) on %d,%d\n", platter, span - i, i, centre->x[i], centre->y[i]);
		i++;
	}

	systemCreatePlayer (base);

	systemClearStates();
	systemPushState (STATE_FREEVIEW);
	FUNCCLOSE ();
}

void worldGenerate (TIMER timer)
{

	// pick unexpanded arch at the highest level; expand it; repeat until there are no more arches on that level; repeat from top

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
		max = hx (mapGetRadius () + 1);
	Entity
		arch;
	Dynarr
		arches;

	if (subhexSpanLevel (at) != 1)
		return;

	while (i < max)
	{
		hexSetBase ((HEX)at->sub.data[i], 0, *(MATSPEC *)dynarr_at (worldMaterials, MATERIAL_GROUND));
		i++;
	}

	arches = worldArchOrderFor (at);
	i = 0;
	while ((arch = *(Entity *)dynarr_at (arches, i++)))
	{
	}
	dynarr_destroy (arches);

}


static Dynarr worldArchOrderFor (SUBHEX platter)
{
	Dynarr
		r = dynarr_create (2, sizeof (Entity));
	return r;
}