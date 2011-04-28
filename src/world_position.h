#ifndef XPH_WORLD_POSITION_H
#define XPH_WORLD_POSITION_H

#include <string.h>
#include <stdio.h>

#include "bit.h"
#include "hex.h"

#include "xph_log.h"

typedef struct world_position * worldPosition;
typedef struct worldHex * WORLDHEX;


worldPosition wp_fromRelativeOffset (const worldPosition pos, unsigned int poleRadius, unsigned int r, unsigned int k, unsigned int i);
worldPosition * wp_adjacent (const worldPosition pos, unsigned int poleRadius);
worldPosition * wp_adjacentSweep (const worldPosition pos, unsigned int poleRadius, unsigned int radius);

worldPosition wp_create (char pole, unsigned int r, unsigned int k, unsigned int i);
worldPosition wp_createEmpty ();
worldPosition wp_duplicate (const worldPosition wp);
void wp_destroy (worldPosition wp);
void wp_destroyAdjacent (worldPosition * adj);

/* the xp/yp values set are in ground coordinate steps, not hex coordinates
 */
unsigned int wp_pos2xy (const worldPosition a, const worldPosition b, signed int * xp, signed int * yp);
unsigned int wp_distance (const worldPosition a, const worldPosition b, unsigned int poleRadius);

int wp_compare (const worldPosition a, const worldPosition b);
const char * wp_print (const worldPosition pos);

unsigned char wp_getPole (const worldPosition pos);
bool wp_getCoords (const worldPosition pos, unsigned int * rp, unsigned int * kp, unsigned int * ip);

/***
 * WORLDHEX FUNCTIONS
 */

WORLDHEX worldhexFromPosition (const worldPosition, unsigned int r, unsigned int k, unsigned int i);
WORLDHEX worldhex (unsigned char pole, unsigned int pr, unsigned int pk, unsigned int pi, unsigned int gr, unsigned int gk, unsigned int gi);
WORLDHEX worldhexDuplicate (const WORLDHEX whx);
void worldhexDestroy (WORLDHEX whx);
const worldPosition worldhexAsWorldPosition (const WORLDHEX whx);
bool worldhexGroundOffset (const WORLDHEX whx, signed int * x, signed int * y);
const char * whxPrint (const WORLDHEX whx);

signed char whxPoleDifference (const WORLDHEX a, const WORLDHEX b);
VECTOR3 whxDistanceVector (const WORLDHEX a, const WORLDHEX b, unsigned char dir);

#endif