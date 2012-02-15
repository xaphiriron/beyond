/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#ifndef XPH_SHAPES_H
#define XPH_SHAPES_H

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>

#include "vector.h"
#include "xph_memory.h"

#include <SDL/SDL_opengl.h>

typedef struct obj_shape * SHAPE;
typedef struct lines_3d * LINES3;

void shape_setColor (SHAPE s, unsigned char r, unsigned char g, unsigned char b, unsigned char a);
void shape_setScale (SHAPE s, unsigned short sc);
void shape_setNormal (SHAPE s, const VECTOR3 * n);

SHAPE shape_makeBlank ();
SHAPE shape_makeFilledPoly (unsigned char sides);
SHAPE shape_makeHollowPoly (unsigned char sides, unsigned char thickness);
SHAPE shape_makeCube (unsigned char size);
SHAPE shape_makeCross ();

void shape_destroy (SHAPE s);

void shape_draw (const SHAPE s, const VECTOR3 * offset);

#endif /* XPH_SHAPES_H */