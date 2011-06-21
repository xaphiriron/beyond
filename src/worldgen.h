#ifndef XPH_WORLDGEN_H
#define XPH_WORLDGEN_H

#include "bool.h"
#include "timer.h"

typedef struct worldgenPattern * PATTERN;
typedef struct worldgenArch * ARCH;

#include "worldgen_graph.h"




typedef struct worldgenFeature * FEATURE;
typedef struct groundList * GROUNDLIST;

typedef union worldShapes * WORLDSHAPE;
typedef union worldEffects * WORLDEFFECT;
typedef union worldRegion * WORLDREGION;

struct affectedHexes
{
	int
		count;
	unsigned int
		* r,
		* k,
		* i;
};

// START EVERYTHING
void worldgenAbsHocNihilo ();
void worldgenFinalizeCreation ();
void worldgenExpandWorldGraph (TIMER t);
void worldgenExpandArchGraph (ARCH p, unsigned int depth);

void worldgenBuildArch (GRAPH g, VERTEX v, PATTERN p);

const GRAPH worldgenWorldGraph ();


PATTERN patternCreate ();

#endif /* XPH_WORLDGEN_H */