#ifndef POINT_H
#define POINT_H

#include "bool.h"
#include "list.h"
#include "vector.h"

typedef struct list * POINTS;

bool point_addNoDup (POINTS * pts, float x, float y);
bool point_findMinMax (const POINTS * pts, float * xminp, float * xmaxp, float * yminp, float * ymaxp);
bool pointcmp (const VECTOR3 * a, const VECTOR3 * b);

// expecting const VECTOR3 *
bool point_areColinear (int n, ...);

#endif /* POINT_H */