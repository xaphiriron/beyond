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

void worldgenAbsHocNihilo ()
{
	static unsigned long
		seed = 0;
	Entity
		base;
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
	position_alignToLevel (base, 1);
	component_instantiate ("worldArch", base);

	FUNCCLOSE ();
}

void worldgenFinalizeCreation ()
{
	FUNCOPEN ();

	systemCreatePlayer ();

	systemClearStates();
	systemPushState (STATE_FREEVIEW);
	FUNCCLOSE ();
}

void worldgenExpandWorldGraph (TIMER timer)
{
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