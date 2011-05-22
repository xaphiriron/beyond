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

	// register loading function
	system_registerTimedFunction (worldgenExpandWorldGraph, 0xff);
	FUNCCLOSE ();
}

void worldgenExpandWorldGraph (TIMER t)
{
	FUNCOPEN ();

/*
	while ()
	{
		if (outOfTime (t))
		{
			return;
		}
	}
*/

	system_removeTimedFunction (worldgenExpandWorldGraph);

	systemPlacePlayerAt (mapPole ('r'));

	systemClearStates();
	systemPushState (STATE_FIRSTPERSONVIEW);
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