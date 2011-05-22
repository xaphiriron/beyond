#include "vector.h"

struct vector vectorCreate (float x, float y, float z) {
  struct vector r;
  r.x = x;
  r.y = y;
  r.z = z;
  return r;
}

float vectorMagnitude (const struct vector * v) {
  return sqrt (v->x * v->x + v->y * v->y + v->z * v->z);
}

float vectorDistance (const struct vector * a, const struct vector * b) {
  float
    x = a->x - b->x,
    y = a->y - b->y,
    z = a->z - b->z;
  return sqrt (x * x + y * y + z * z);
}

float vectorAngleDifference (const struct vector * a, const struct vector * b) {
  float
    theta = acos ((a->x * b->x + a->y * b->y + a->z * b->z) / sqrt ((a->x * a->x + a->y * a->y + a->z * a->z) * (b->x * b->x + b->y * b->y + b->z * b->z)));
  return theta;
}

/*
import math
def angle (a, b):
	theta = math.acos ((a[0] * b[0] + a[1] * b[1]) / math.sqrt ((a[0] * a[0] + a[1] * a[1] + a[2] * a[2]) * (b[0] * b[0] + b[1] * b[1] + b[2] * b[2])));
	return theta;



*/

struct vector vectorNormalize (const struct vector * v) {
  struct vector r;
  float t = vectorMagnitude (v);
  r.x = v->x / t;
  r.y = v->y / t;
  r.z = v->z / t;
  return r;
}

struct vector vectorAdd (const struct vector * a, const struct vector * b) {
  struct vector r;
  r.x = a->x + b->x;
  r.y = a->y + b->y;
  r.z = a->z + b->z;
  return r;
}

struct vector vectorSubtract (const struct vector * a, const struct vector * b) {
  struct vector r;
  r.x = a->x - b->x;
  r.y = a->y - b->y;
  r.z = a->z - b->z;
  return r;
}

struct vector vectorAverage (int i, ...) {
  struct vector
    r = vectorCreate (0, 0, 0),
    * v = NULL;
  int c = i;
  va_list ap;
  va_start (ap, i);
  while (c > 0) {
    v = va_arg (ap, struct vector *);
    r = vectorAdd (&r, v);
    --c;
  }
  va_end (ap);
  r.x /= i;
  r.y /= i;
  r.z /= i;
  return r;
}

struct vector vectorMultiplyByScalar (const struct vector * v, float m) {
  struct vector r;
  r.x = v->x * m;
  r.y = v->y * m;
  r.z = v->z * m;
  return r;
}

VECTOR3 vectorDivideByScalar (const struct vector * v, float d)
{
	struct vector
		r;
	r.x = v->x / d;
	r.y = v->y / d;
	r.z = v->z / d;
	return r;
}

struct vector vectorMultiplyByMatrix (const struct vector * v, float * m) {
  struct vector r;
  r.x = v->x * m[0] +
        v->y * m[4] +
        v->z * m[8];
  r.y = v->x * m[1] +
        v->y * m[5] +
        v->z * m[9];
  r.z = v->x * m[2] +
        v->y * m[6] +
        v->z * m[10];
  return r;
}

struct vector vectorCross (const struct vector * a, const struct vector *b) {
  struct vector r;
  r.x = a->y * b->z - b->y * a->z;
  r.y = a->z * b->x - b->z * a->x;
  r.z = a->x * b->y - b->x * a->y;
  return r;
}

float vectorDot (const struct vector * a, const struct vector * b) {
  return a->x * b->x + a->y * b->y + a->z + b->z;
}

bool vector_cmp (const struct vector * a, const struct vector * b) {
  if (fcmp (a->x, b->x) == FALSE) {
    //printf ("x != x: %f, %f\n", a->x, b->x);
    return FALSE;
  } else if (!fcmp (a->y, b->y)) {
    //printf ("y != y: %f, %f\n", a->y, b->y);
    return FALSE;
  } else if (!fcmp (a->z, b->z)) {
    //printf ("z != z: %f, %f\n", a->z, b->z);
    return FALSE;
  }
  return TRUE;
}
