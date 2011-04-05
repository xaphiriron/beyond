#ifndef XPH_WORLDGEN_H
#define XPH_WORLDGEN_H

#include "entity.h"
#include "world_position.h"
#include "system.h"

typedef struct worldgenFeature * FEATURE;
typedef struct worldgenPattern * PATTERN;

typedef struct worldTile * WORLDTILE;
typedef struct groundList * GROUNDLIST;

typedef union worldShapes WORLDSHAPE;
typedef union worldEffects WORLDEFFECT;

enum worldShapeTypes
{
	WG_STRAIGHTLINE,	// WORLDTILE * s, WORLDTILE * f, VECTOR3 * d
	WG_CIRCLE,			// WORLDTILE * c, float r
	WG_ELIPSOID,		// WORLDTILE * a, WORLDTILE * b, ???
	WG_BEZIERCURVE,		// ???
};

enum worldEffectTypes
{
	WG_HEIGHTSHARP,			// unsigned int height
	WG_HEIGHTSLOPE,			// unsigned int height, unsigned int changePerTile
};

// DO EVERYTHING
void worldgenAbsHocNihilo ();
void worldgenExpandUniversalPatternGraph ();
void worldgenExpandPatternGraph (PATTERN p, unsigned int depth);

WORLDSHAPE worldgenShapeCreate (enum worldShapeTypes shape, ...);
WORLDEFFECT worldgenEffectCreate (enum worldEffectTypes effect, ...);

void worldgenShapeDestroy (WORLDSHAPE shape);
void worldgenEffectDestroy (WORLDEFFECT effect);

GROUNDLIST worldgenCalculateShapeFootprint (const WORLDSHAPE shape);
void worldgenImprint (Entity ground, WORLDSHAPE shape, WORLDEFFECT effect);

GROUNDLIST groundlistUnion (const GROUNDLIST a, const GROUNDLIST b);
GROUNDLIST groundlistIntersection (const GROUNDLIST a, const GROUNDLIST b);


/*
LINE COORDS:
two wp/ground points and a direction
(a direction is needed since the maps wrap around and there are a lot of viable lines between two points)

a global tile reference would be six values:
the pole (three possible values; 2 bits)
the pole r-value (max is GroundWorld->poleRadius)
the pole k-value (six possible values; 3 bits)
the pole i-value (max is GroundWorld->poleRadius - 1)
this defines a ground, so:
the tile's x-value within the ground (max is GroundWorld->groundRadius/-GroundWorld->groundRadius)
the tile's y-value within the ground (...)
... which could be condensed to five if the pole's x,y coord was used instead of polar coordinates

typedef struct worldTile * WORLDTILE;

struct worldTile
{
unsigned int
	pr, pi;
signed int
	gx, gy;
unsigned char
	bits;	// the pole and k-val
};

so:

typedef struct worldShape * WORLDSHAPE;
struct worldShape
{
};
// ^ except probably it should be a union like the sdl event union

WORLDSHAPE worldgenShapeCreate (enum worldShapes shape, ...);

WORLDSHAPE worldgenShapeCreate (WG_LINE, worldTileA, worldTileB, vector);

and then...

GROUNDLIST worldgenCalculateShapeFootprint (const WORLDSHAPE shape);

AND THEN...

GROUNDLIST tilelistUnion (const GROUNDLIST a, const GROUNDLIST b);
void worldgenImprint ({ground?}, WORLDSHAPE, WORLDEFFECT);

update = tilelistUnion (generated, visible);
foreach (update as g)
{
	worldgenImprint (g, shape, effect);
}
*/

#endif /* XPH_WORLDGEN_H */