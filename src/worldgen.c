#include "worldgen.h"

#include "bit.h"
#include "system.h"

#include "map.h"

GRAPH
	WorldGraph = NULL;

struct worldgenPattern
{
	
};

struct worldgenArch // ARCH
{
	PATTERN
		pattern;
	bool
		expanded;
	GRAPH
		subpatterns;	// ???
	// ???
};

void worldgenAbsHocNihilo ()
{
	FUNCOPEN ();
	PATTERN
		firstPattern = patternCreate ();	// from patterns file
	
	mapSetSpanAndRadius (7, 8);
	mapGeneratePoles (POLE_TRI);
	WorldGraph = worldgenCreateBlankGraph ();
	graphWorldBase (WorldGraph, GRAPH_POLE_R | GRAPH_POLE_G | GRAPH_POLE_B);

	worldgenBuildArch (WorldGraph, graphGetVertex (WorldGraph, 1), firstPattern);

	FUNCCLOSE ();
}

void worldgenFinalizeCreation ()
{
	SUBHEX
		pole;

	FUNCOPEN ();

	pole = mapPole ('r');
	mapForceGrowAtLevelForDistance (pole, 1, 3);

	systemCreatePlayer ();

	systemClearStates();
	systemPushState (STATE_FREEVIEW);
	FUNCCLOSE ();
}

void worldgenExpandWorldGraph (TIMER t)
{
	FUNCOPEN ();

	loadSetGoal (1);

	while (0)
	{
		if (outOfTime (t))
		{
			return;
		}
	}

	loadSetLoaded (1);

	FUNCCLOSE ();
}

void worldgenExpandArchGraph (ARCH p, unsigned int depth)
{
}

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

/***
 * PATTERN FUNCTIONS
 */

PATTERN patternCreate ()
{
	PATTERN
		p = xph_alloc (sizeof (struct worldgenPattern));
	return p;
}