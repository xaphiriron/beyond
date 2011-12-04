#include "worldgen_graph.h"

struct xgraph
{
	Dynarr
		vertices,
		edges;
};

struct xedge
{
	signed int
		vertices[2];
	unsigned char
		dir;	// from vertices[0] to vertices[1]; 0-5 if the edge crosses a pole boundary; 8 otherwise
};

struct xvertex
{
	WORLDHEX
		position;
	Dynarr
		edges;		// size_t values that are offsets of xgraph->edges
	ARCH
		arch;
};

static void graphAddVertex (GRAPH g, WORLDHEX pos);
static void graphConnectVertices (GRAPH g, int a, int b, unsigned char dir);

/*
static signed int edgeVertexConnection (const EDGE e, unsigned int vc);
static float angleBetweenEdges (const GRAPH g, const signed int baseVertex, const EDGE a, const EDGE b);
*/

/***
 * region graph functions
 */


GRAPH worldgenCreateBlankGraph ()
{
	GRAPH
		graph = xph_alloc (sizeof (struct xgraph));
	//memset (graph, '\0', sizeof (struct xgraph));
	graph->vertices = dynarr_create (2, sizeof (struct xvertex *));
	graph->edges = dynarr_create (2, sizeof (struct xedge *));
	return graph;
}

void graphWorldBase (GRAPH g, enum graph_inits seedPoints)
{
	WORLDHEX
		whx;
	int
		i, j, k;
	signed int
		* poleDirs;
	struct xvertex
		* v,
		* w;
/*
	if (seedPoints == GRAPH_POLE_R || seedPoints == GRAPH_POLE_G || seedPoints == GRAPH_POLE_B)
	{
		ERROR ("Can't generate base map with only one seed point (got %s)", seedPoints == GRAPH_POLE_R ? "GRAPH_POLE_R" : seedPoints == GRAPH_POLE_G ? "GRAPH_POLE_G" : "GRAPH_POLE_B");
		return;
	}
*/
	if (seedPoints & GRAPH_POLE_R)
	{
		whx = subhexGeneratePosition (mapPole ('r'));
		graphAddVertex (g, whx);
		worldhexDestroy (whx);
	}
	if (seedPoints & GRAPH_POLE_G)
	{
		whx = subhexGeneratePosition (mapPole ('g'));
		graphAddVertex (g, whx);
		worldhexDestroy (whx);
	}
	if (seedPoints & GRAPH_POLE_B)
	{
		whx = subhexGeneratePosition (mapPole ('b'));
		graphAddVertex (g, whx);
		worldhexDestroy (whx);
	}
	i = graphVertexCount (g);
	while (--i >= 0)
	{
		v = graphGetVertex (g, i);
		j = i;
		while (--j >= 0)
		{
			w = graphGetVertex (g, j);
			poleDirs = mapPoleConnections (worldhexPole (vertexPosition (v)), worldhexPole (vertexPosition (w)));
			//poleDirs = mapPoleConnections (subhexPoleName (worldhexSubhex (vertexPosition (v))), subhexPoleName (worldhexSubhex (vertexPosition (w))));
			k = 0;
			while (poleDirs[k] != -1)
			{
				graphConnectVertices (g, i, j, poleDirs[k]);
				k++;
			}
			xph_free (poleDirs);
		}
	}
}

/***
 * internal graph alteration functions
 */

static void graphAddVertex (GRAPH g, WORLDHEX pos)
{
	VERTEX
		v = xph_alloc (sizeof (struct xvertex));
	v->position = worldhexDuplicate (pos);
	v->edges = dynarr_create (2, sizeof (size_t));
	v->arch = NULL;
	dynarr_push (g->vertices, v);
}

static void graphConnectVertices (GRAPH g, int a, int b, unsigned char dir)
{
	EDGE
		e = xph_alloc (sizeof (struct xedge));
	VERTEX
		av = *(VERTEX *)dynarr_at (g->vertices, a),
		bv = *(VERTEX *)dynarr_at (g->vertices, b);
	size_t
		eo;
	if (av == NULL || bv == NULL)
	{
		ERROR ("edge creation not valid: one or both values (%d/%p, %d/%p) is NULL; unable to connect vertices", a, av, b, bv);
		xph_free (e);
		return;
	}
	if (a < b)
	{
		e->vertices[0] = a;
		e->vertices[1] = b;
		e->dir = dir;
	}
	else
	{
		e->vertices[0] = b;
		e->vertices[1] = a;
		if (dir > 5)
			e->dir = dir;
		else
			e->dir = (dir + 3) % 6;
	}
	//e->regions[0] = -1;
	//e->regions[1] = -1;
	
	eo = dynarr_push (g->edges, e);
	dynarr_push (av->edges, eo);
	dynarr_push (bv->edges, eo);
	
}

/*
static void graphGenerateRegions (GRAPH g)
{
	VERTEX
		initialVertex,
		v, w;
	EDGE
		initialEdge,
		e, f;
	REGION
		r;
	float
		rad,
		min;
	size_t
		ev;
	signed int
		i,
		vertexBaseIndex = 0,
		vertexIndex = 0,
		edgeIndex = 0,
		edgeCheckIndex = 0,
		edgeMatchIndex = 0,
		regionIndex = 0;

	int
		o;

	while ((v = graphGetVertex (g, vertexBaseIndex)) != NULL)
	{
		//INFO ("got vertex %d/%p", vertexBaseIndex, v);
		while (dynarr_size (v->edges) == 0)
		{
			WARNING ("Vertex (%d/%p) has no edges; the graph isn't well-formed.", vertexBaseIndex, v);
			vertexBaseIndex++;
			continue;
		}
		edgeIndex = dynarr_size (v->edges);
		while (edgeIndex > 0)
		{
			edgeIndex--;
			e = graphGetEdge (g, *(unsigned int *)dynarr_at (v->edges, edgeIndex));
			DEBUG ("got edge %d/%p, %d-th on vertex %d/%p", *(unsigned int *)dynarr_at (v->edges, edgeIndex), e, edgeIndex, vertexBaseIndex, v);
			ev = edgeVertexConnection (e, vertexBaseIndex);
			if (e->regions[ev] != -1)
			{
				INFO ("edge %d has its region already set to %d", *(unsigned int *)dynarr_at (v->edges, edgeIndex), e->regions[ev]);
				continue;
			}
			regionIndex = dynarr_size (g->regions);
			r = xph_alloc (sizeof (struct xregion));
			r->vert = dynarr_create (2, sizeof (size_t));
			r->edge = dynarr_create (2, sizeof (size_t));
			DEBUG ("Found edge (%d/%p, %d<>%d(%d)) with unset region; generating new region area (%d)", edgeIndex, e, e->vertices[0], e->vertices[1], e->dir, regionIndex);

			 * between the awkwardness of this initialization vs. the equally
			 * long variable updates at the end of this loop and the necessary
			 * use of a do-while to keep the loop from exiting on the first
			 * pass, I'm /certain/ there's a way to rewrite this to be less,
			 * uh. awkward.
			 *   -xph 2011-04-20
			 *
			w = graphGetVertex (g, e->vertices[ev ^ 1]);
			initialVertex = v;
			initialEdge = e;
			vertexIndex = vertexBaseIndex;
			dynarr_push (r->vert, vertexIndex);
			dynarr_push (r->edge, edgeIndex);
			e->regions[ev] = regionIndex;
			INFO ("Initial vertex: %d; initial edge %d/%d<>%d(%d)", vertexIndex, edgeIndex, e->vertices[0], e->vertices[1], e->dir);
			 * a circuit has been completed iff we've arrived at the same edge
			 * with the same vertex as the base -- in the largest scope it's
			 * possible to traverse the same vertex multiple times or traverse
			 * an edge twice in different directions without closing the
			 * circuit. this is the downside of allowing multigraphs under
			 * certain conditions (the condition: each edge between the same
			 * two vertices must 1, cross poles and 2, not cross the same pole
			 * edge as any other edge between the two applicable vertices)
			 *   -xph 2011-04-20
			 *
			do
			{
				// iterate over all of w's edges and compare their angle to e
				min = M_PI * 2;
				i = dynarr_size (w->edges);
				edgeMatchIndex = -1;
				while (i > 0)
				{
					i--;
					edgeCheckIndex = *(unsigned int *)dynarr_at (w->edges, i);
					f = graphGetEdge (g, edgeCheckIndex);
					//DEBUG ("comparing edge %d/%p", edgeCheckIndex, f);
					rad = angleBetweenEdges (g, e->vertices[ev ^ 1], e, f);
					if (rad < min)
					{
						min = rad;
						edgeMatchIndex = edgeCheckIndex;
					}
				}
				if (edgeMatchIndex == -1)
				{
					ERROR ("No match found", NULL);
				}
				dynarr_push (r->edge, edgeMatchIndex);
				dynarr_push (r->vert, e->vertices[ev ^ 1]);	// w's index

				// these next lines are /highly/ order-dependant, since we need to update all these values and the only way to calculate them is to use the values of various variables from the prior loop pass. so don't move them around. if you don't understand what's going on, meditate on the distinction between a pointer to an vertex struct and that vertex's index offset in g->vertices, and how absolutely aside from g stores the latter and not the former.
				vertexIndex = e->vertices [ev ^ 1];
				v = w;
				e = graphGetEdge (g, edgeMatchIndex);
				ev = edgeVertexConnection (e, vertexIndex);
				w = graphGetVertex (g, e->vertices[ev ^ 1]);
				INFO ("Adding edge %d<>%d(%d) to region border (and its vertices too)", e->vertices[0], e->vertices[1], e->dir);

				e->regions[ev] = regionIndex;
				INFO ("looking for initial vertex: %d; initial edge %d/%d<>%d(%d)", vertexBaseIndex, edgeIndex, initialEdge->vertices[0], initialEdge->vertices[1], initialEdge->dir);
				INFO ("(have %d; %d/%d<>%d(%d))", vertexIndex, edgeMatchIndex, e->vertices[0], e->vertices[1], e->dir);
			}
			while (v != initialVertex || e != initialEdge);

			dynarr_push (g->regions, r);
			INFO ("finalizing region %d/%p and adding to graph", regionIndex, r);
			o = 0;
			INFO ("region info:", NULL);
			while (o < dynarr_size (r->vert))
			{
				INFO ("from vertex %d along edge %d...", *(signed int *)dynarr_at (r->vert, o), *(signed int *)dynarr_at (r->edge, o));
				o++;
			}
			r = NULL;
			edgeIndex--;
		}
		vertexBaseIndex++;
	}
}
*/

/*
static signed int edgeVertexConnection (const EDGE e, unsigned int vc)
{
	if (e->vertices[0] == vc)
		return 0;
	else if (e->vertices[1] == vc)
		return 1;
	ERROR ("No connection between edge %d<>%d(%d) and vertex %d", e->vertices[0], e->vertices[1], e->dir, vc);
	exit (2);
	return -1;
}

static float angleBetweenEdges (const GRAPH g, const signed int baseVertex, const EDGE a, const EDGE b)
{
	VECTOR3
		aVector,
		bVector,
		cross;
	VERTEX
		aVertex,
		bVertex,
		origin;
	float
		rad;
	size_t
		av, bv;
	signed int
		aRelativeDir,
		bRelativeDir;
	//DEBUG ("%s (%p, %p, %p)...", __FUNCTION__, g, a, b);
	if (a == b)
	{
		ERROR ("Edges compared are the same (%p vs. %p)", a, b);
		return M_PI * 2;
	}

	if (a->vertices[0] == baseVertex)
	{
		av = 0;
		aRelativeDir = a->dir;
	}
	else if (a->vertices[1] == baseVertex)
	{
		av = 1;
		aRelativeDir = a->dir < 6 ? (a->dir + 3) % 6 : 8;
	}
	else
	{
		ERROR ("Unable to measure angle: edge %d<>%d doesn't connect to vertex %d", a->vertices[0], a->vertices[1], baseVertex);
		return M_PI * 2;
	}

	if (b->vertices[0] == baseVertex)
	{
		bv = 0;
		bRelativeDir = b->dir;
	}
	else if (b->vertices[1] == baseVertex)
	{
		bv = 1;
		bRelativeDir = b->dir < 6 ? (b->dir + 3) % 6 : 8;
	}
	else
	{
		ERROR ("Unable to measure angle: edge %d<>%d doesn't connect to vertex %d", b->vertices[0], b->vertices[1], baseVertex);
		return M_PI * 2;
	}

	aVertex = *(VERTEX *)dynarr_at (g->vertices, a->vertices[av ^ 1]);
	bVertex = *(VERTEX *)dynarr_at (g->vertices, b->vertices[bv ^ 1]);
	origin = *(VERTEX *)dynarr_at (g->vertices, a->vertices[av]);
	// convert differing endpoint positions into vectors; calc angle between them
	INFO ("edge %d<>%d(%d) and %d<>%d(%d); endpoints %d %d and connected at %d/%d:", a->vertices[0], a->vertices[1], a->dir, b->vertices[0], b->vertices[1], b->dir, a->vertices[av ^ 1], b->vertices[bv ^ 1], a->vertices[av], b->vertices[bv]);
	INFO ("dir/relative: %d/%d (a) %d/%d (b)", a->dir, aRelativeDir, b->dir, bRelativeDir);
	aVector = whxDistanceVector (origin->position, aVertex->position, aRelativeDir);
	bVector = whxDistanceVector (origin->position, bVertex->position, bRelativeDir);
	rad = vectorAngleDifference (&aVector, &bVector);
	cross = vectorCross (&aVector, &bVector);
	if (cross.y < 0)
		rad *= -1;
	//DEBUG ("...%s", __FUNCTION__);
	INFO ("  angle: %f", rad);
	return rad;
}
*/

void graphDestroy (GRAPH g)
{
/*
	dynarr_destroy (g->vertices);
	dynarr_destroy (g->edges);
*/
	xph_free (g);
}

Dynarr graphGetRawEdges (GRAPH g)
{
	return g->edges;
}

Dynarr graphGetRawVertices (GRAPH g)
{
	return g->vertices;
}


VERTEX graphGetVertex (const GRAPH g, int i)
{
	return *(VERTEX *)dynarr_at (g->vertices, i);
}

EDGE graphGetEdge (GRAPH g, int i)
{
	return *(EDGE *)dynarr_at (g->edges, i);
}

int graphVertexCount (const GRAPH g)
{
	return dynarr_size (g->vertices);
}

int graphEdgeCount (const GRAPH g)
{
	return dynarr_size (g->edges);
}

bool graphHasOutside (const GRAPH g)
{
	return false;
}


/***
 * VERTEX FUNCTIONS
 */

void graphSetVertexArch (VERTEX v, ARCH a)
{
	v->arch = a;
}

const Dynarr vertexEdges (const VERTEX v)
{
	return v->edges;
}

const WORLDHEX vertexPosition (const VERTEX v)
{
	return v->position;
}

const ARCH vertexArch (const VERTEX v)
{
	return v->arch;
}