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
	
	mapSetSpanAndRadius (3, 3);
	mapGeneratePoles (POLE_TRI);

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
		i = 0,
		heightval = 0;

	FUNCOPEN ();

	// all this is just worldgen scratchpad stuff
	loadSetGoal (655);
	switch (genstate)
	{
		case GEN_INIT:
			pole = mapPole ('r');
			heightval = rand () & ((1 << 9) - 1);
			mapDataAdd (pole, "height", heightval);

			pole = mapPole ('g');
			heightval = rand () & ((1 << 9) - 1);
			mapDataAdd (pole, "height", heightval);

			pole = mapPole ('b');
			heightval = rand () & ((1 << 9) - 1);
			mapDataAdd (pole, "height", heightval);

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
				heightval = rand () & ((1 << 8) - 1);
				mapDataAdd (base, "height", heightval);
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
				heightval = rand () & ((1 << 8) - 1);
				mapDataAdd (base, "height", heightval);
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
				heightval = rand () & ((1 << 8) - 1);
				mapDataAdd (base, "height", heightval);
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

			worldgenCreateArch (NULL, base);

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
	arch->size = (rand () & 0x03) + 3;

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
		hex = NULL;
	if (subhexSpanLevel (at) != 1)
		return;

	hex = mapHexAtCoordinateAuto (at, -1, 0, 0);
	hex->hex.centre = height;

	subhexLocalCoordinates (at, &x, &y);
	while (dir < 6)
	{
		adjCoords[dir] = hex_xyCoord2Space (centres[dir * 2], centres[dir * 2 + 1]);
		adjHeight[dir] = mapDataGet (mapHexAtCoordinateAuto (subhexParent (at), -1, x + XY[dir][X], y + XY[dir][Y]), "height");
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
				hex->hex.centre = baryInterpolate (&p, &adjCoords[dir], &adjCoords[lastDir], height, adjHeight[dir], adjHeight[lastDir]);
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
		height = hex->hex.centre;

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
			if (r < arch->size)
				hex->hex.centre = height;
			else
			{
				if (abs (i - r/2) < 1)
					hex->hex.centre = height + arch->size / 2;
				else
					hex->hex.centre = height + arch->size * 2;
			}

			hex_nextValidCoord (&r, &k, &i);
		}
	}
}