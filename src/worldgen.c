#include "worldgen.h"

#include "bit.h"
#include "system.h"

#include "hex_utility.h"
#include "map.h"
#include "map_internal.h"

GRAPH
	WorldGraph = NULL;

struct worldgenPattern
{
	
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
		height;
	signed int
		x, y;

};

void worldgenAbsHocNihilo ()
{
	FUNCOPEN ();
/*
	PATTERN
		firstPattern = patternCreate ();	// from patterns file
*/
	
	mapSetSpanAndRadius (7, 8);
	mapGeneratePoles (POLE_TRI);

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

void worldgenExpandWorldGraph (TIMER t)
{
	static SUBHEX
		pole = NULL,
		base = NULL;
	SUBHEX
		active = NULL;
	RELATIVEHEX
		rel = NULL;
	FUNCOPEN ();

	loadSetGoal (2);

	if (!pole)
		pole = mapPole ('r');

	if (!base)
	{
		mapForceGrowAtLevelForDistance (pole, 1, 3);
		rel = mapRelativeSubhexWithCoordinateOffset (pole, -6, 0, 0);
		base = mapRelativeTarget (rel);
		mapRelativeDestroy (rel);
	}

	loadSetLoaded (1);

	active = base;

	while (1)
	{
		// place a single arch on the pole and break
		worldgenCreateArch (NULL, active);
		break;
		if (outOfTime (t))
			return;
	}

	loadSetLoaded (2);

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

	memset (arch, '\0', sizeof (struct worldgenArch));

	arch->centre = base;

	// do the pattern setup here!!!
	r = rand () % MapRadius;
	k = rand () % 6;
	i = rand () % MapRadius - 1;
	hex_rki2xy (r, k, i, &arch->x, &arch->y);
	arch->height = (rand () & 0x0f) + 5;

	mapArchSet (base, arch);
	return arch;
}

void worldgenExpandArchGraph (ARCH p, unsigned int depth)
{
}

/*
void worldgenBuildArch (GRAPH g, VERTEX v, PATTERN p)
{
	FUNCOPEN ();
	if (vertexArch (v) != NULL)
	{
		ERROR ("Cannot build arch at %s (on vertex %p, in graph %p); Arch already present.", worldhexPrint (vertexPosition (v)), v, g);
		return;
	}
	graphSetVertexArch (v, (ARCH)-1);
	FUNCCLOSE ();
}


const GRAPH worldgenWorldGraph ()
{
	return WorldGraph;
}
*/

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

void worldgenImprintAllArches (SUBHEX at)
{
	ARCH
		arch = NULL;
	SUBHEX
		hex = NULL,
		closer = NULL;
	int
		offset = 0;
	signed int
		hx, hy,
		cx, cy,
		height;
	unsigned int
		r, k, i;
	if (subhexSpanLevel (at) < 1)
	{
		WARNING ("Can't imprint platter (%p) with span level %d.", at, subhexSpanLevel (at));
		return;
	}
	if (at->sub.imprinted == TRUE)
		return;
	while ((arch = mapArchGet (at, offset++)) != NULL)
	{
		hex = mapHexAtCoordinateAuto (at, arch->x, arch->y);
		if (hex->hex.centre < arch->height)
			hex->hex.centre = arch->height;
		r = 1;
		k = i = 0;
		while (r * 2 < arch->height)
		{
			hex_rki2xy (r, k, i, &hx, &hy);
			hx += arch->x;
			hy += arch->y;
			hex = mapHexAtCoordinateAuto (at, hx, hy);
			if (hex == NULL)
			{
				hex_nextValidCoord (&r, &k, &i);
				continue;
			}
			hex_stepLineToOrigin (hx - arch->x,hy - arch->y, 1, &cx, &cy);
			closer = mapHexAtCoordinateAuto (at, arch->x + cx, arch->y + cy);
			DEBUG ("from %d, %d got 'closer' hex %d, %d", hx, hy, arch->x + cx, arch->y + cy);
			if (closer == NULL)
			{
				// ?!?
				hex_nextValidCoord (&r, &k, &i);
				continue;
			}
			if (i == 0)
			{
				SETCORNER (hex->hex.corners, (k + 2) % 6, 1);
				SETCORNER (hex->hex.corners, (k + 3) % 6, 1);
				SETCORNER (hex->hex.corners, (k + 5) % 6, -1);
				SETCORNER (hex->hex.corners, k, -1);
			}
			else
			{
				SETCORNER (hex->hex.corners, (k + 2) % 6, 1);
				SETCORNER (hex->hex.corners, (k + 3) % 6, 2);
				SETCORNER (hex->hex.corners, (k + 4) % 6, 1);
				SETCORNER (hex->hex.corners, k, -1);
			}
			height = 2;
			if (height <= closer->hex.centre && hex->hex.centre < closer->hex.centre - height)
				hex->hex.centre = closer->hex.centre - height;
			hex_nextValidCoord (&r, &k, &i);
		}
	}
	at->sub.imprinted = TRUE;
}