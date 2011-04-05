#include "worldgen.h"

struct worldgenFeature // FEATURE
{
	WORLDSHAPE
		* shape;
	WORLDEFFECT
		* effect;
};

struct worldgenPattern // PATTERN
{
	// ???
};

struct worldTile // WORLDTILE
{
	unsigned int
		pr, pi;
	signed int
		gx, gy;
	unsigned char
		bits;	// the pole bits and pk-val
};

struct groundList // GROUNDLIST
{
	int
		i;
	worldPosition
		** grounds;
};

union worldShapes // WORLDSHAPE
{
	enum worldShapeTypes type;
};

union worldEffects // WORLDEFFECT
{
	enum worldEffectTypes type;
};

void worldgenAbsHocNihilo ()
{
	// instantiate pattern #1
	// register loading function
	system_registerTimedFunction (worldgenExpandUniversalPatternGraph, 0xff);
}

void worldgenExpandUniversalPatternGraph (TIMER t)
{
	DEBUG ("in %s", __FUNCTION__);
	while (!outOfTime (t))
	{
	// bredth-first traversal of pattern #1; when the first unexpanded pattern is found, call worldgenExpandPatternGraph (p, 1) on it; keep going until the depth threshhold is reached.
		break;
	}
	system_removeTimedFunction (worldgenExpandUniversalPatternGraph);
	groundWorld_placePlayer ();
	system_setState (STATE_FIRSTPERSONVIEW);
}

void worldgenExpandPatternGraph (PATTERN p, unsigned int depth)
{

}