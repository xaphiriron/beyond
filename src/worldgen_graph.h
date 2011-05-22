#ifndef XPH_WORLDGEN_GRAPH
#define XPH_WORLDGEN_GRAPH

#include "xph_memory.h"
#include "dynarr.h"

#include "map.h"

typedef struct xgraph * GRAPH;

typedef struct xedge * EDGE;
typedef struct xvertex * VERTEX;

#include "worldgen.h"

enum graph_inits {
	GRAPH_POLE_R = 0x01,
	GRAPH_POLE_G = 0x02,
	GRAPH_POLE_B = 0x04,
};

/***
 * region graph functions
 */

GRAPH worldgenCreateBlankGraph ();

void graphWorldBase (GRAPH g, enum graph_inits seedPoints);
/*
void graphAddConnectedVertices (GRAPH g, int c, ...);	// expecting [position]
void graphCalcRegionEdges (GRAPH g);
*/
void graphDestroy (GRAPH g);

Dynarr graphGetRawVertices (GRAPH g);
Dynarr graphGetRawEdges (GRAPH g);
VERTEX graphGetVertex (const GRAPH g, int i);
EDGE graphGetEdge (GRAPH g, int i);

int graphVertexCount (const GRAPH g);
int graphEdgeCount (const GRAPH g);
bool graphHasOutside (const GRAPH g);


void graphSetVertexArch (VERTEX v, ARCH a);

const Dynarr vertexEdges (const VERTEX v);
const WORLDHEX vertexPosition (const VERTEX v);
const ARCH vertexArch (const VERTEX v);

#endif /* XPH_WORLDGEN_GRAPH */