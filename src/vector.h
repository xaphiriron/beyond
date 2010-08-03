#ifndef VECTOR_H
#define VECTOR_H

#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include "bool.h"
#include "fcmp.h"

typedef struct vector {
  float x, y, z;
} VECTOR3;

struct vector vectorCreate (float x, float y, float z);
float vectorMagnitude (const struct vector * v);
float vectorDistance (const struct vector * a, const struct vector * b);
struct vector vectorNormalize (const struct vector * v);
struct vector vectorAdd (const struct vector * a, const struct vector * b);
struct vector vectorSubtract (const struct vector * a, const struct vector * b);
/* expects vector *, not vector like all the other functions */
struct vector vectorAverage (int i, ...);
struct vector vectorMultiplyByScalar (const struct vector * v, float m);
/* expects a 4x4 matrix in row-major order */
struct vector vectorMultiplyByMatrix(const struct vector * v, float * m);
struct vector vectorCross (const struct vector * a, const struct vector * b);
float vectorDot (const struct vector * a, const struct vector * b);

bool vector_cmp (const struct vector * a, const struct vector * b);
#endif /* VECTOR_H */
