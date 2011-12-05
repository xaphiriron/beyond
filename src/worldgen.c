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

void worldgenAbsHocNihilo ()
{
	static unsigned long
		seed = 0;
	FUNCOPEN ();
	
	mapSetSpanAndRadius (4, 8);
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

void worldgenFinalizeCreation ()
{
	int
		i = 1,
		span = mapGetSpan ();
	hexPos
		centre;
	SUBHEX
		platter;
	FUNCOPEN ();

	position_alignToLevel (base, 3);
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

void worldgenExpandWorldGraph (TIMER timer)
{

	// pick unfinished arch at the highest level; expand its graph; repeat until there are no more arches on that level; imprint all arches; repeat from top

	loadSetGoal (1);
	loadSetLoaded (1);
}

/***
 * IMPRINTING FUNCTIONS
 */

void worldgenImprintMapData (SUBHEX at)
{
}

void worldgenImprintAllArches (SUBHEX at)
{
}