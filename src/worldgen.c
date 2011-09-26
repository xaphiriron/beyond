#include "worldgen.h"

#include "bit.h"
#include "system.h"

#include "hex_utility.h"
#include "map.h"
#include "map_internal.h"

#include <time.h>

GRAPH
	WorldGraph = NULL;

#define PATTERN_NAME_LENGTH	64

struct worldgenPattern
{
	unsigned int
		id;
	char
		name[PATTERN_NAME_LENGTH];
	
};



enum worldMaterialList
{
	MATERIAL_AIR,
	MATERIAL_GROUND,
};
static Dynarr
	worldMaterials = NULL;

/*
struct worldgenArch // ARCH
{
	SUBHEX
		centred;
	


	PATTERN
		pattern;
	bool
		expanded;
	GRAPH
		subpatterns;	// ???
	// ???
};
*/

struct worldgenArch // ARCH
{
	SUBHEX
		centre;

	VECTOR3
		position;
	unsigned int
		size;
	signed int
		x, y;

};

void worldgenAbsHocNihilo ()
{
	static unsigned long
		seed = 0;
	FUNCOPEN ();
/*
	PATTERN
		firstPattern = patternCreate ();	// from patterns file
*/
	
	mapSetSpanAndRadius (4, 8);
	mapGeneratePoles (POLE_TRI);

	worldMaterials = dynarr_create (3, sizeof (MATSPEC));
	dynarr_assign (worldMaterials, MATERIAL_AIR, makeMaterial (MAT_TRANSPARENT));
	dynarr_assign (worldMaterials, MATERIAL_GROUND, makeMaterial (MAT_OPAQUE));

	seed = time (NULL);
	INFO ("%s: using seed of \'%ld\'", __FUNCTION__, seed);
	loadSetText ("Initializing...");

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
	static enum {
		GEN_INIT,
		GEN_REDPOLE,
		GEN_GREENPOLE,
		GEN_BLUEPOLE,
		GEN_FINAL,
		GEN_DONE
	} genstate = GEN_INIT;
	SUBHEX
		pole = NULL,
		base = NULL;
	signed int
		x = 0,
		y = 0;
	unsigned int
		r = 0,
		k = 0,
		i = 0;

	FUNCOPEN ();

	// all this is just worldgen scratchpad stuff
	loadSetGoal (655);
	switch (genstate)
	{
		case GEN_INIT:
			pole = mapPole ('r');
			mapDataAdd (pole, "height", rand () & ((1 << 12) - 1));

			pole = mapPole ('g');
			mapDataAdd (pole, "height", rand () & ((1 << 12) - 1));

			pole = mapPole ('b');
			mapDataAdd (pole, "height", rand () & ((1 << 12) - 1));

			mapForceSubdivide (mapPole ('r'));
			mapForceSubdivide (mapPole ('g'));
			mapForceSubdivide (mapPole ('b'));

			genstate = GEN_REDPOLE;
			loadSetLoaded (3);
			loadSetText ("Filling red pole w/ noise...");
			if (outOfTime (timer))
				return;

		case GEN_REDPOLE:
			pole = mapPole ('r');
			r = k = i = 0;
			while (r <= MapRadius)
			{
				hex_rki2xy (r, k, i, &x, &y);
				base = subhexData (pole, x, y);
				mapDataAdd (base, "height", rand () & ((1 << 11) - 1));
				hex_nextValidCoord (&r, &k, &i);
			}
			genstate = GEN_GREENPOLE;
			loadSetLoaded (220);
			loadSetText ("Filling green pole w/ noise...");
			if (outOfTime (timer))
				return;

		case GEN_GREENPOLE:
			pole = mapPole ('g');
			r = k = i = 0;
			while (r <= MapRadius)
			{
				hex_rki2xy (r, k, i, &x, &y);
				base = subhexData (pole, x, y);
				mapDataAdd (base, "height", rand () & ((1 << 11) - 1));
				hex_nextValidCoord (&r, &k, &i);
			}
			genstate = GEN_BLUEPOLE;
			loadSetLoaded (437);
			loadSetText ("Filling blue pole w/ noise...");
			if (outOfTime (timer))
				return;

		case GEN_BLUEPOLE:
			pole = mapPole ('b');
			r = k = i = 0;
			while (r <= MapRadius)
			{
				hex_rki2xy (r, k, i, &x, &y);
				base = subhexData (pole, x, y);
				mapDataAdd (base, "height", rand () & ((1 << 11) - 1));
				hex_nextValidCoord (&r, &k, &i);
			}
			genstate = GEN_FINAL;
			loadSetLoaded (654);
			loadSetText ("Finalizing...");
			if (outOfTime (timer))
				return;

		case GEN_FINAL:
			pole = mapPole ('g');
			mapForceGrowAtLevelForDistance (pole, 1, 3);
			base = mapHexAtCoordinateAuto (pole, -MapSpan + 1, 0, 0);

			//worldgenCreateArch (NULL, base);

			loadSetLoaded (655);
			genstate = GEN_DONE;
			break;

		case GEN_DONE:
		default:
			loadSetGoal (655);
			loadSetLoaded (655);
			break;
		}

	FUNCCLOSE ();
}

ARCH worldgenBuildArch (ARCH parent, unsigned int subid, PATTERN pattern)
{
	return NULL;
}

ARCH worldgenCreateArch (PATTERN pattern, SUBHEX base)
{
	ARCH
		arch = xph_alloc (sizeof (struct worldgenArch));
	unsigned int
		r, k, i;

	memset (arch, 0, sizeof (struct worldgenArch));

	arch->centre = base;

	// do the pattern setup here!!!
	r = rand () % MapRadius;
	k = rand () % 6;
	i = rand () % MapRadius - 1;
	hex_rki2xy (r, k, i, &arch->x, &arch->y);
	arch->size = (rand () & 0x03) + 6;

	mapArchSet (base, arch);

	i = 0;
	while (i < 6)
	{
		mapArchSet (mapHexAtCoordinateAuto (base, 0, XY[i][X], XY[i][Y]), arch);
		i++;
	}

	return arch;
}

void worldgenExpandArchGraph (ARCH p, unsigned int depth)
{
}

/***
 * PATTERN FUNCTIONS
 */

PATTERN patternCreate ()
{
	PATTERN
		p = xph_alloc (sizeof (struct worldgenPattern));
	return p;
}

/***
 * IMPRINTING FUNCTIONS
 */

void worldgenImprintMapData (SUBHEX at)
{
	unsigned int
		height = mapDataGet (at, "height"),
		localHeight = 0,
		adjHeight[6],
		r = 0, k = 0, i = 0;
	signed int
		* centres = mapSpanCentres (1),
		dir = 0,
		lastDir = 5,
		x, y;
	VECTOR3
		p,
		adjCoords[6];
	SUBHEX
		hex = NULL,
		adj = NULL;
	HEXSTEP
		base;
	if (subhexSpanLevel (at) != 1)
		return;

	hex = mapHexAtCoordinateAuto (at, -1, 0, 0);
	base = hexSetBase ((HEX)hex, height, *(MATSPEC *)dynarr_at (worldMaterials, MATERIAL_GROUND));

	subhexLocalCoordinates (at, &x, &y);
	while (dir < 6)
	{
		adjCoords[dir] = hex_xyCoord2Space (centres[dir * 2], centres[dir * 2 + 1]);
		adj = mapHexAtCoordinateAuto (at, 0, XY[dir][X], XY[dir][Y]);
		adjHeight[dir] = mapDataGet (adj, "height");
		dir++;
	}

	dir = 0;
	lastDir = 5;
	while (dir < 6)
	{
		r = 1;
		k = dir;
		i = 0;
		while (r <= MapRadius)
		{
			hex_rki2xy (r, k, i, &x, &y);
			hex = mapHexAtCoordinateAuto (at, -1, x, y);
			if (hex != NULL)
			{
				p = hex_coord2space (r, k, i);

				localHeight = baryInterpolate (&p, &adjCoords[dir], &adjCoords[lastDir], height, adjHeight[dir], adjHeight[lastDir]);
				hexSetBase ((HEX)hex, localHeight, *(MATSPEC *)dynarr_at (worldMaterials, MATERIAL_GROUND));

			}

			i++;
			if (k == dir && i >= (r/2.0))
			{
				r++;
				k = lastDir;
				i = ceil (r / 2.0);
			}
			else if (i == r)
			{
				k = dir;
				i = 0;
			}

		}
		lastDir = dir;
		dir++;
	}
}

void worldgenImprintAllArches (SUBHEX at)
{
	ARCH
		arch = NULL;
	SUBHEX
		hex = NULL;
	HEXSTEP
		base;
	int
		offset = 0;
	signed int
		hx = 0,
		hy = 0,
		height;
	unsigned int
		r = 0, k = 0, i = 0;
	if (subhexSpanLevel (at) < 1)
	{
		WARNING ("Can't imprint platter (%p) with span level %d.", at, subhexSpanLevel (at));
		return;
	}
	while ((arch = mapArchGet (at, offset++)) != NULL)
	{
		hex = mapHexAtCoordinateAuto (arch->centre, -1, arch->x, arch->y);
		/* FIXME: if the platter the base is on isn't loaded then this height value will be all wrong or maybe even potentially NULL, and that would lead to crashes or improperly-imprinted hexes */
		base = *(HEXSTEP *)dynarr_front (hex->hex.steps);
		height = base->height + 32;

		while (r <= arch->size)
		{
			hex_rki2xy (r, k, i, &hx, &hy);
			hx += arch->x;
			hy += arch->y;
			
			hex = mapHexAtCoordinateAuto (arch->centre, -1, hx, hy);
			if (hex == NULL)
			{
				hex_nextValidCoord (&r, &k, &i);
				continue;
			}
			else if (subhexParent (hex) != at)
			{
				/* WARNING: this else if will break if we ever apply patterns to span-2 or higher platters (since hex is a hex and at would be span-2+, so there's no possibility of it being the direct parent even if it is the grandparent) */
				hex_nextValidCoord (&r, &k, &i);
				continue;
			}
			if (r == 0)
				hexSetBase ((HEX)hex, height, *(MATSPEC *)dynarr_at (worldMaterials, MATERIAL_GROUND));
			else if (r < arch->size)
			{
				hexCreateStep ((HEX)hex, height - (((arch->size + 1) - r) * 2), *(MATSPEC *)dynarr_at (worldMaterials, MATERIAL_AIR));
				hexCreateStep ((HEX)hex, height, *(MATSPEC *)dynarr_at (worldMaterials, MATERIAL_GROUND));
			}
			else
			{
				hexCreateStep ((HEX)hex, height - (((arch->size + 1) - r) * 2), *(MATSPEC *)dynarr_at (worldMaterials, MATERIAL_AIR));
				if (abs (i - r/2) < 1)
					hexCreateStep ((HEX)hex, height + arch->size / 2, *(MATSPEC *)dynarr_at (worldMaterials, MATERIAL_GROUND));
				else
					hexCreateStep ((HEX)hex, height + arch->size * 2, *(MATSPEC *)dynarr_at (worldMaterials, MATERIAL_GROUND));
			}
			hex_nextValidCoord (&r, &k, &i);
		}
	}
}