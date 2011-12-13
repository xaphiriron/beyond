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
	map_posSwitchFocus (pos, mapGetSpan () - 1);
	position_set (base, pos);
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


	//position_alignToLevel (base, mapGetSpan () - 1);
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
		max = fx (mapGetRadius ());
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

	int
		x, y;
	subhexLocalCoordinates (at, &x, &y);
	printf ("got %d applicable arch%s for subhex %p (at: %d, %d)\n", dynarr_size (arches), dynarr_size (arches) == 1 ? "" : "es", at, x, y);
	while ((arch = *(Entity *)dynarr_at (arches, i++)))
	{
		printf ("imprinting arch %p on subhex %p\n", arch, at);
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
		//printf ("getting positions arond subhex %p\n", current);
		nearbyPlatters = map_posAround (current, 1);
		i = 0;
		while ((pos = *(hexPos *)dynarr_at (nearbyPlatters, i++)))
		{
			platter = map_posFocusedPlatter (pos);
			if (!platter)
				continue;
			platterArches = subhexGetArches (platter);
			//printf (" - hit platter %p (which has %d arch%s)\n", platter, dynarr_size (platterArches), dynarr_size (platterArches) == 1 ? "" : "es");
			j = 0;
			while ((arch = *(Entity *)dynarr_at (platterArches, j++)))
			{
				dynarr_push (r, arch);
			}
		}
		//printf ("destroying platter list [1]\n");
		dynarr_map (nearbyPlatters, (void (*)(void *))map_freePos);
		//printf ("destroying platter list [2]\n");
		dynarr_destroy (nearbyPlatters);
	}
	//printf ("done generating arch order\n");

	return r;
}
