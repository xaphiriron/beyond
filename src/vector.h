/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#ifndef VECTOR_H
#define VECTOR_H

#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include "fcmp.h"

typedef struct vector {
  float x, y, z;
} VECTOR3;

struct vector vectorCreate (float x, float y, float z);
float vectorMagnitude (const struct vector * v);
float vectorDistance (const struct vector * a, const struct vector * b);
float vectorAngleDifference (const struct vector * a, const struct vector * b);
struct vector vectorNormalize (const struct vector * v);
struct vector vectorAdd (const struct vector * a, const struct vector * b);
struct vector vectorSubtract (const struct vector * a, const struct vector * b);
/* expects vector *, not vector like all the other functions */
struct vector vectorAverage (int i, ...);
struct vector vectorMultiplyByScalar (const struct vector * v, float m);
VECTOR3 vectorDivideByScalar (const struct vector * v, float d);
/* expects a 4x4 matrix in row-major order */
struct vector vectorMultiplyByMatrix (const struct vector * v, float * m);
struct vector vectorCross (const struct vector * a, const struct vector * b);
float vectorDot (const struct vector * a, const struct vector * b);

bool vector_cmp (const struct vector * a, const struct vector * b);
#endif /* VECTOR_H */
