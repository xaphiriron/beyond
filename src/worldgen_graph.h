#ifndef XPH_WORLDGEN_GRAPH
#define XPH_WORLDGEN_GRAPH

#include "xph_memory.h"
#include "dynarr.h"

#include "world_position.h"

typedef struct xgraph * GRAPH;

typedef struct xregion * REGION;
typedef struct xedge * EDGE;
typedef struct xvertex * VERTEX;

enum graph_inits {
	GRAPH_POLE_R = 0x01,
	GRAPH_POLE_G = 0x02,
	GRAPH_POLE_B = 0x04,
};

/***
 * region graph functions
 */

GRAPH worldgenCreateBlankRegionGraph ();

void graphWorldBase (GRAPH g, enum graph_inits seedPoints);
/*
void graphAddConnectedVertices (GRAPH g, int c, ...);	// expecting [position]
void graphCalcRegionEdges (GRAPH g);
*/
void graphDestroy (GRAPH g);

Dynarr graphGetRawEdges (GRAPH g);
Dynarr graphGetRawVertices (GRAPH g);
Dynarr graphGetRawRegions (GRAPH g);
VERTEX graphGetVertex (GRAPH g, int i);
EDGE graphGetEdge (GRAPH g, int i);
REGION graphGetRegion (GRAPH g, int i);

const WORLDHEX vertexPosition (const VERTEX v);

int graphRegionCount (const GRAPH g);
int graphVertexCount (const GRAPH g);
int graphEdgeCount (const GRAPH g);

bool graphHasOutside (const GRAPH g);

#endif /* XPH_WORLDGEN_GRAPH */