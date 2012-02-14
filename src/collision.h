#ifndef XPH_COLLIDE_H
#define XPH_COLLIDE_H

#include "vector.h"

#include <float.h>
#include <assert.h>

typedef struct LINE3
{
	VECTOR3
		origin,
		dir;
} LINE3;

typedef struct TRI3
{
	VECTOR3
		pts[3];
} TRI3;

enum turns_values
{
	LEFT = -1,
	THROUGH = 0,
	RIGHT = 1
};

enum dim_drop
{
	DROP_X,
	DROP_Y,
	DROP_Z
};

bool line_planeHit (const LINE3 * const line, const VECTOR3 * planeNormal, float * tAtIntersection);

bool line_triHit (const LINE3 * const line, const TRI3 * const tri, float * tAtIntersection);
bool line_quadHit (const LINE3 * const line, const VECTOR3 * const quad);

int turns (float ox, float oy, float lx1, float ly1, float lx2, float ly2);

bool pointInPoly (const VECTOR3 * const point, enum dim_drop drop, int points, const VECTOR3 * const poly);

void baryWeights (const VECTOR3 const * p, const VECTOR3 const * c2, const VECTOR3 const * c3, float * weight);
unsigned int baryInterpolate (const VECTOR3 const * p, const VECTOR3 const * c2, const VECTOR3 const * c3, const unsigned int v1, const unsigned int v2, const unsigned int v3);

#endif /* XPH_COLLIDE_H */