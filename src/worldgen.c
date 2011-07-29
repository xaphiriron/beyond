#include "worldgen.h"

#include "bit.h"
#include "system.h"

#include "map.h"

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
	memset (arch, '\0', sizeof (struct worldgenArch));

	arch->centre = base;

	// do the pattern setup here!!!

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