/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#ifndef XPH_MATSPEC_H
#define XPH_MATSPEC_H

#include <stdbool.h>
enum materials
{
	MAT_AIR,
	MAT_STONE,
	MAT_DIRT,
	MAT_GRASS,
	MAT_SAND,
	MAT_SNOW,
	MAT_SILT,
	MAT_WATER,
};

typedef struct material_specification * MATSPEC;

void materialsGenerate ();

MATSPEC material (enum materials);

void matspecColor (const MATSPEC mat, unsigned char * r, unsigned char * g, unsigned char * b, unsigned char * a);

unsigned char materialParam (const MATSPEC mat, const char * param);
#define matParam(x, y)	materialParam(x, y)

#endif /* XPH_MATSPEC_H */