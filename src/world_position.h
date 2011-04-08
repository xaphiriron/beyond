#ifndef XPH_WORLD_POSITION_H
#define XPH_WORLD_POSITION_H

#include <string.h>
#include <stdio.h>

#include "bit.h"
#include "hex.h"

typedef struct world_position * worldPosition;

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
unsigned int wp_pos2xy (const worldPosition a, const worldPosition b, unsigned int poleRadius, signed int * xp, signed int * yp);
unsigned int wp_distance (const worldPosition a, const worldPosition b, unsigned int poleRadius);

int wp_compare (const worldPosition a, const worldPosition b);
const char * wp_print (const worldPosition pos);

unsigned char wp_getPole (const worldPosition pos);
bool wp_getCoords (const worldPosition pos, unsigned int * rp, unsigned int * kp, unsigned int * ip);

#endif