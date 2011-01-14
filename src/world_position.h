#ifndef XPH_WORLD_POSITION
#define XPH_WORLD_POSITION

#include <string.h>
#include <stdio.h>

#include "hex.h"

typedef struct world_position * worldPosition;

worldPosition wp_fromRelativeOffset (const worldPosition pos, unsigned int poleRadius, unsigned int r, unsigned int k, unsigned int i);
worldPosition * wp_adjacent (const worldPosition pos, unsigned int poleRadius);

worldPosition wp_create (char pole, unsigned int r, unsigned int k, unsigned int i);
worldPosition wp_createEmpty ();
worldPosition wp_duplicate (const worldPosition wp);
void wp_destroy (worldPosition wp);
void wp_destroyAdjacent (worldPosition * adj);

unsigned int wp_distance (const worldPosition a, const worldPosition b);

int wp_compare (const worldPosition a, const worldPosition b);
void wp_print (const worldPosition pos);

unsigned char wp_getPole (const worldPosition pos);
bool wp_getCoords (const worldPosition pos, unsigned int * rp, unsigned int * kp, unsigned int * ip);

#endif