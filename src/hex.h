#ifndef XPH_HEX_H
#define XPH_HEX_H

#include <assert.h>
#include <float.h>

#include "xph_memory.h"

#include "vector.h"
#include "dynarr.h"

extern const int H[6][2];
extern const char XY[6][2];

typedef struct hex * Hex;

struct hex {
  VECTOR3
    topNormal,
    baseNormal;
  float
    top, topA, topB,
    base, baseA, baseB;
  Dynarr entitiesOccupying;
  short
    r, k, i,	// polar coordinates from center of ground map
    x, y;	// cartesian coordinates from center (w/ non-orthographic axes)
};


enum hex_sides {
  HEX_TOP = 1,
  HEX_BASE
};

int hex (int n);
int hex_linearCoord (short r, short k, short i);
void hex_rki2xy (short r, short k, short i, short * xp, short * yp);
void hex_xy2rki (short x, short y, short * rp, short * kp, short * ip);
bool hex_wellformedRKI (short r, short k, short i);

VECTOR3 hex_linearTileDistance (int length, int dir);
VECTOR3 hex_coordOffset (short r, short k, short i);
bool hex_coordinateAtSpace (const VECTOR3 * space, int * xp, int * yp);

struct hex * hex_create (short r, short k, short i, float height);
void hex_destroy (struct hex * h);

void hex_setSlope (struct hex * h, enum hex_sides side, float a, float b, float c);

#endif /* XPH_HEX_H */