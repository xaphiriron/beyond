#ifndef XPH_WORLDGEN_H
#define XPH_WORLDGEN_H

#include "bit.h"
#include "entity.h"
#include "world_position.h"
#include "system.h"

typedef struct worldgenTemplate * TEMPLATE;
typedef struct worldgenFeature * FEATURE;
typedef struct worldgenPattern * PATTERN;

typedef struct worldHex * WORLDHEX;
typedef struct groundList * GROUNDLIST;

typedef union worldShapes * WORLDSHAPE;
typedef union worldEffects * WORLDEFFECT;

struct affectedHexes
{
	int
		count;
	unsigned int
		* r,
		* k,
		* i;
};

enum worldShapeTypes
{
	WGS_HEX,				// unsigned int radius
};

enum worldEffectTypes
{
	WGE_ELEVATION,			// signed int change
};

// START EVERYTHING
void worldgenAbsHocNihilo ();
void worldgenExpandWorldPatternGraph ();
void worldgenExpandPatternGraph (PATTERN p, unsigned int depth);


bool worldgenUnexpandedPatternsAt (const worldPosition wp);
Dynarr worldgenGetUnimprintedPatternsAt (const worldPosition wp);
void worldgenMarkPatternImprinted (PATTERN p, const worldPosition wp);
void worldgenImprintGround (TIMER t, Component c);
bool worldgenIsGroundFullyLoaded (const worldPosition wp);

TEMPLATE templateCreate ();
TEMPLATE templateFromSpecification (const char * spec);

void templateSetShape (TEMPLATE template, WORLDSHAPE shape);
void templateAddFeature (TEMPLATE template, const char * region, WORLDEFFECT effect);

PATTERN templateInstantiateNear (const TEMPLATE template, const WORLDHEX whx);



WORLDSHAPE worldgenShapeCreate (enum worldShapeTypes shape, ...);
WORLDEFFECT worldgenEffectCreate (enum worldEffectTypes effect, ...);

void worldgenShapeDestroy (WORLDSHAPE shape);
void worldgenEffectDestroy (WORLDEFFECT effect);


WORLDHEX worldhex (const worldPosition, unsigned int r, unsigned int k, unsigned int i);

GROUNDLIST worldgenCalculateShapeFootprint (const WORLDSHAPE shape, const WORLDHEX centre);
struct affectedHexes * worldgenAffectedHexes (const PATTERN p, const worldPosition wp);

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