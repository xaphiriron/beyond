#include "worldgen.h"

#include "bit.h"
#include "system.h"

#include "xph_path.h"

#include "hex_utility.h"
#include "map.h"
#include "map_internal.h"

#include <time.h>

#include "component_position.h"
#include "comp_arch.h"
#include "comp_optlayout.h"
#include "comp_gui.h"

#include "comp_chaser.h"

// this is very much a makeshift way of addressing arches; it's to be replaced with a general arch list or something??
static Entity
	base;

static Dynarr archOrderFor (SUBHEX platter);

static unsigned char
	worldSpan = 0;
static unsigned long
	worldSeed = 0;

void worldConfig (Entity confirm)
{
	char
		patternPath[PATH_MAX];
	Dynarr
		openFrames;
	Entity
		options = gui_getFrame (confirm);
	assert (entity_getAs (options, "optlayout") != NULL);

	worldSpan = optlayout_optionNumValue (options, "World Size");
	if (strcmp (optlayout_optionStrValue (options, "Seed"), "") == 0)
		worldSeed = time (NULL);
	else if (optlayout_optionIsNumeric (options, "Seed"))
		worldSeed = optlayout_optionNumValue (options, "Seed");
	else
		worldSeed = hash (optlayout_optionStrValue (options, "Seed"));

	INFO ("%s: using seed of \'%ld\'", __FUNCTION__, worldSeed);
	printf ("%s: using seed of \'%ld\'\n", __FUNCTION__, worldSeed);

	entity_destroy (options);

	strcpy (patternPath, "../");
	strncat (patternPath, optlayout_optionStrValue (options, "Pattern Data"), PATH_MAX - 4);
	patternLoadDefinitions (absolutePath (patternPath));


	openFrames = entity_getWith (1, "gui");
	dynarr_map (openFrames, (void (*)(void *))entity_destroy);
	dynarr_destroy (openFrames);

	printf ("TRIGGERING WORLDGEN:\n");
	systemLoad (worldInit, worldGenerate, worldFinalize);
}

void worldInit ()
{
	hexPos
		pos;
	FUNCOPEN ();
	
	mapSetSpanAndRadius (worldSpan, 4);
	mapGeneratePoles ();

	materialsGenerate ();

	srand (worldSeed);

	base = entity_create ();
	component_instantiate ("position", base);
	component_instantiate ("arch", base);
	entity_refresh (base);
	pos = map_randomPos ();
	map_posSwitchFocus (pos, 1);
	position_set (base, pos);
	entity_message (base, NULL, "setArchPattern", patternGet (1));

	FUNCCLOSE ();
}

void worldFinalize ()
{
	hexPos
		centre/*,
		randPos*/;
	Entity
//		npc1,
//		npc2,
//		npc3,
//		plant,
		player;
	SUBHEX
		basePlatter,
		baseHex;
	HEXSTEP
		baseStep;

	FUNCOPEN ();

	// FIXME: platters are currently imprinted based on the rendering cache, which isn't set until the player exists. this results in the player being placed on an empty pre-imprinting platter. this is dumb. - xph 2012 02 03
	centre = position_get (base);
	map_posSwitchFocus (centre, 1);
	printf ("loading the platter the player will be standing on (%p)\n", centre);
	mapLoad_load (centre);

	printf ("creating player\n");
	systemCreatePlayer (base);
	player = entity_getByName ("PLAYER");

	basePlatter = hexPos_platter (centre, 1);
	baseHex = subhexData (basePlatter, 0, 0);
	baseStep = hexGroundStepNear (&baseHex->hex, 0);
	//plant = entity_create ();
	//component_instantiate ("position", plant);
	//component_instantiate ("plant", plant);
	//entity_refresh (plant);
	//position_placeOnHexStep (plant, &baseHex->hex, baseStep);
	//position_set (plant, map_copy (position_get (base)));

// 	npc1 = entity_create ();
// 	component_instantiate ("position", npc1);
// 	component_instantiate ("body", npc1);
// 	component_instantiate ("chaser", npc1);
// 	component_instantiate ("walking", npc1);
// 	entity_refresh (npc1);
// 
// 	npc2 = entity_create ();
// 	component_instantiate ("position", npc2);
// 	component_instantiate ("body", npc2);
// 	component_instantiate ("chaser", npc2);
// 	component_instantiate ("walking", npc2);
// 	entity_refresh (npc2);
// 
// 	npc3 = entity_create ();
// 	component_instantiate ("position", npc3);
// 	component_instantiate ("body", npc3);
// 	component_instantiate ("chaser", npc3);
// 	component_instantiate ("walking", npc3);
// 	entity_refresh (npc3);
// 
// 	printf ("randpos\n");
// 	randPos = map_randomPositionNear (centre, 0);
// 	printf ("basehex\n");
// 	baseHex = hexPos_platter (randPos, 0);
// 	printf ("place (%p)\n", baseHex);
// 	position_placeOnHexStep (npc1, &baseHex->hex, hexGroundStepNear (&baseHex->hex, 0));
// 	printf ("freepos\n");
// 	map_freePos (randPos);
// 
// 	randPos = map_randomPositionNear (centre, 0);
// 	baseHex = hexPos_platter (randPos, 0);
// 	position_placeOnHexStep (npc2, &baseHex->hex, hexGroundStepNear (&baseHex->hex, 0));
// 	map_freePos (randPos);
// 
// 	randPos = map_randomPositionNear (centre, 0);
// 	baseHex = hexPos_platter (randPos, 0);
// 	position_placeOnHexStep (npc3, &baseHex->hex, hexGroundStepNear (&baseHex->hex, 0));
// 	map_freePos (randPos);
// 
// 	chaser_target (npc1, player, 0.2);
// 	chaser_target (npc2, player, 0.2);
// 	chaser_target (npc3, player, 0.2);
// 	chaser_target (npc1, npc2, 1.0);
// 	chaser_target (npc1, npc3, -1.0);
// 	chaser_target (npc2, npc3, 1.0);
// 	chaser_target (npc2, npc1, -1.0);
// 	chaser_target (npc3, npc1, 1.0);
// 	chaser_target (npc3, npc2, -1.0);
// 
// 	printf ("made entity #%d, #%d, #%d; npcs\n", entity_GUID (npc1), entity_GUID (npc2), entity_GUID (npc3));
	

	systemClearStates();
	systemPushState (STATE_FREEVIEW);

	FUNCCLOSE ();
}

void worldGenerate (TIMER * timer)
{
	static enum
	{
		SET_HEIGHT_BASE,
		SUBDIVIDE_POLE_R,
		SUBDIVIDE_POLE_G,
		SUBDIVIDE_POLE_B,
		SET_TEMP_POLE_R,
		SET_TEMP_POLE_G,
		SET_TEMP_POLE_B,
		SET_HEIGHT_POLE_R,
		SET_HEIGHT_POLE_G,
		SET_HEIGHT_POLE_B,

		SEALEVEL_CALC,
		SET_LAND_SEA_POLE_R,
		SET_LAND_SEA_POLE_G,
		SET_LAND_SEA_POLE_B,

		EXPAND_ARCH,
		WORLDGEN_DONE,
	} stage = SET_HEIGHT_BASE;
	static SUBHEX
		pole[3] = {NULL, NULL, NULL};
	SUBHEX
		active;

	int
		i = 0,
		max = fx (MapRadius),
		x, y,
		val;
	static int
		maxHeight = 0,
		minHeight = INT_MAX,
		seaLevel = 0;

	if (pole[0] == NULL)
	{
		pole[0] = mapPole ('r');
		pole[1] = mapPole ('g');
		pole[2] = mapPole ('b');
	}

	loadSetGoal (20);

	// FIXME: it would not surprise me if this crashed worlds with too-small span limits
	while (!outOfTime (timer) && stage < WORLDGEN_DONE)
	{
		switch (stage)
		{
			case SET_HEIGHT_BASE:
				val = rand () & ((1 << 11) - 1);
				if (maxHeight < val)
					maxHeight = val;
				if (minHeight > val)
					minHeight = val;
				mapDataSet (pole[0], "height", val);
				val = rand () & ((1 << 11) - 1);
				if (maxHeight < val)
					maxHeight = val;
				if (minHeight > val)
					minHeight = val;
				mapDataSet (pole[1], "height", val);
				val = rand () & ((1 << 11) - 1);
				if (maxHeight < val)
					maxHeight = val;
				if (minHeight > val)
					minHeight = val;
				mapDataSet (pole[2], "height", val);
				loadSetLoaded (1);
				break;
			case SUBDIVIDE_POLE_R:
				mapForceSubdivide (pole[0]);
				loadSetLoaded (2);
				break;
			case SUBDIVIDE_POLE_G:
				mapForceSubdivide (pole[1]);
				loadSetLoaded (3);
				break;
			case SUBDIVIDE_POLE_B:
				mapForceSubdivide (pole[2]);
				loadSetLoaded (4);
				break;
			case SET_TEMP_POLE_R:
				i = 0;
				while (i < max)
				{
					hex_unlineate (i, &x, &y);
					active = subhexData (pole[0], x, y);
					mapDataSet (active, "tempAvg", 2048 - ((MapRadius + 1) - hexMagnitude (x, y)) * 2048);
					mapDataSet (active, "tempVar", ((MapRadius + 1) - hexMagnitude (x, y)) * 1024);
					i++;
				}
				loadSetLoaded (5);
				break;
			case SET_TEMP_POLE_G:
				i = 0;
				while (i < max)
				{
					hex_unlineate (i, &x, &y);
					active = subhexData (pole[1], x, y);
					mapDataSet (active, "tempAvg", 2048 - ((MapRadius + 1) - hexMagnitude (x, y)) * 2048);
					mapDataSet (active, "tempVar", ((MapRadius + 1) - hexMagnitude (x, y)) * 1024);
					i++;
				}
				loadSetLoaded (6);
				break;
			case SET_TEMP_POLE_B:
				i = 0;
				while (i < max)
				{
					hex_unlineate (i, &x, &y);
					active = subhexData (pole[2], x, y);
					mapDataSet (active, "tempAvg", 2048 - ((MapRadius + 1) - hexMagnitude (x, y)) * 2048);
					mapDataSet (active, "tempVar", ((MapRadius + 1) - hexMagnitude (x, y)) * 1024);
					i++;
				}
				loadSetLoaded (7);
				break;
			case SET_HEIGHT_POLE_R:
				i = 0;
				while (i < max)
				{
					hex_unlineate (i, &x, &y);
					active = subhexData (pole[0], x, y);
					mapDataAdd (active, "height", rand () & ((1 << 10) - 1));
					val = mapDataGet (active, "height");
					if (maxHeight < val)
						maxHeight = val;
					if (minHeight > val)
						minHeight = val;
					i++;
				}
				loadSetLoaded (8);
				break;
			case SET_HEIGHT_POLE_G:
				i = 0;
				while (i < max)
				{
					hex_unlineate (i, &x, &y);
					active = subhexData (pole[1], x, y);
					mapDataAdd (active, "height", rand () & ((1 << 10) - 1));
					val = mapDataGet (active, "height");
					if (maxHeight < val)
						maxHeight = val;
					if (minHeight > val)
						minHeight = val;
					i++;
				}
				loadSetLoaded (9);
				break;
			case SET_HEIGHT_POLE_B:
				i = 0;
				while (i < max)
				{
					hex_unlineate (i, &x, &y);
					active = subhexData (pole[2], x, y);
					mapDataAdd (active, "height", rand () & ((1 << 10) - 1));
					val = mapDataGet (active, "height");
					if (maxHeight < val)
						maxHeight = val;
					if (minHeight > val)
						minHeight = val;
					i++;
				}
				loadSetLoaded (10);
				break;

			case SEALEVEL_CALC:
				seaLevel = minHeight + (maxHeight / 2);
				break;
			case SET_LAND_SEA_POLE_R:
				i = 0;
				while (i < max)
				{
					hex_unlineate (i, &x, &y);
					active = subhexData (pole[0], x, y);
					mapDataSet (active, "seaLevel", seaLevel);
					val = mapDataGet (active, "height");
					if (val <= seaLevel)
						mapDataSet (active, "sea", 4);
					else
						mapDataSet (active, "land", 4);
					i++;
				}
				break;
			case SET_LAND_SEA_POLE_G:
				i = 0;
				while (i < max)
				{
					hex_unlineate (i, &x, &y);
					active = subhexData (pole[1], x, y);
					mapDataSet (active, "seaLevel", seaLevel);
					val = mapDataGet (active, "height");
					if (val <= seaLevel)
						mapDataSet (active, "sea", 4);
					else
						mapDataSet (active, "land", 4);
					i++;
				}
				break;
			case SET_LAND_SEA_POLE_B:
				i = 0;
				while (i < max)
				{
					hex_unlineate (i, &x, &y);
					active = subhexData (pole[2], x, y);
					mapDataSet (active, "seaLevel", seaLevel);
					val = mapDataGet (active, "height");
					if (val <= seaLevel)
						mapDataSet (active, "sea", 4);
					else
						mapDataSet (active, "land", 4);
					i++;
				}
				break;


			case EXPAND_ARCH:
				//entity_message (base, NULL, "archExpand", NULL);
				loadSetLoaded (19);
				break;
			case WORLDGEN_DONE:
				
				loadSetGoal (20);
				loadSetLoaded (20);
				break;
		}
		stage++;
	}

	if (stage >= WORLDGEN_DONE)
	{
		loadSetGoal (20);
		loadSetLoaded (20);
		return;
	}
	else
		printf ("ran out of time on stage %d\n", stage);
}

/***
 * IMPRINTING FUNCTIONS
 */

void worldImprint (SUBHEX at)
{
	int
		i = 0,
		hexCount = fx (MapRadius),
		x, y;
	Entity
		arch;
	Dynarr
		arches;
	SUBHEX
		hex;

	signed int
		height,
		seaDiff,
		seaLevel,
		randVar;
	unsigned int
		matType;

	if (subhexSpanLevel (at) != 1)
		return;

	while (i < hexCount)
	{
		hex_unlineate (i, &x, &y);
		hex = subhexData (at, x, y);
		height = mapDataBaryInterpolate (at, x, y, "height");
		seaLevel = mapDataGet (at, "seaLevel");
		randVar = (rand () & 7) - 3;
		seaDiff = (height + randVar) - seaLevel;
		if (seaDiff < -8)
			matType = MAT_SILT;
		else if (seaDiff < 8)
			matType = MAT_SAND;
		else if (seaDiff < 10)
			matType = MAT_DIRT;
		else if (seaDiff < 52)
			matType = MAT_GRASS;
		else if (seaDiff < 72)
			matType = MAT_STONE;
		else
			matType = MAT_SNOW;

		if (height < seaLevel)
		{
			hexSetBase (&hex->hex, height, material (matType));
			hexCreateStep (&hex->hex, seaLevel, material (MAT_WATER));
		}
		else
		{
			hexSetBase (&hex->hex, height, material (matType));
		}

		int
			j,
			adjHeight[2],
			cornerHeight;
		HEXSTEP
			baseStep;
		baseStep = *(HEXSTEP *)dynarr_at (hex->hex.steps, 0);
		j = 0;
		while (j < 6)
		{
			adjHeight[0] = mapDataBaryInterpolate (at, x + XY[j][X], y + XY[j][Y], "height");
			adjHeight[1] = mapDataBaryInterpolate (at, x + XY[(j + 1) % 6][X], y + XY[(j + 1) % 6][Y], "height");

			cornerHeight = (height + adjHeight[0] + adjHeight[1]) / 3;
			//SETCORNER (baseStep->corners, j, (cornerHeight - height));
			if (cornerHeight < height)
				SETCORNER (baseStep->corners, j, (cornerHeight - height) + 1);
			else
				SETCORNER (baseStep->corners, j, (cornerHeight - height));
			
			j++;
		}

		i++;
	}

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
