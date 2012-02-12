#ifndef XPH_HEX_UTILITY_H
#define XPH_HEX_UTILITY_H

#include <assert.h>
#include <float.h>
#include <stdlib.h>

#include <stdbool.h>
#include "vector.h"

/***
 * generic hex functions:
 */

extern const signed short H [6][2];
extern const signed char XY [6][2];

enum XYenum
{
	X = 0,
	Y = 1
};

#define		DIR1MOD6(x)		((x) + 1 >= 6 ? 0 : (x) + 1)

unsigned int hx (unsigned int n);
// like hx but without the off-by-one issue
unsigned int fx (unsigned int n);

void hex_rki2xy (unsigned int r, unsigned int k, unsigned int i, signed int * xp, signed int * yp);
void hex_xy2rki (signed int x, signed int y, unsigned int * rp, unsigned int * kp, unsigned int * ip);

unsigned int hex_linearCoord (unsigned int r, unsigned int k, unsigned int i);
unsigned int hex_linearXY (signed int x, signed int y);
void hex_unlineate (int l, signed int * x, signed int * y);

unsigned int hex_distanceBetween (signed int ax, signed int ay, signed int bx, signed int by);
unsigned int hexMagnitude (signed int x, signed int y);

/* this calculates the x,y coordinate offset between the central hex of two adjacent grounds, both of radius {radius}, with the direction between the two being {dir} and the result set to {xp} and {yp}
 */
bool hex_centerDistanceCoord (unsigned int radius, unsigned int dir, signed int * xp, signed int * yp);
bool hexGround_centerDistanceCoord (unsigned int radius, unsigned int dir, signed int * xp, signed int * yp);
bool hexPole_centerDistanceCoord (unsigned int dir, signed int * xp, signed int * yp);





enum turns_values
{
	LEFT = -1,
	THROUGH = 0,
	RIGHT = 1
};
int turns (float ox, float oy, float lx1, float ly1, float lx2, float ly2);

/***
 * generic hex functions involving vectors:
 */

VECTOR3 hex_tileDistance (int length, unsigned int dir);
VECTOR3 hex_coord2space (unsigned int r, unsigned int k, unsigned int i);
VECTOR3 hex_xyCoord2Space (signed int x, signed int y);

VECTOR3 hexGround_centerDistanceSpace (unsigned int radius, unsigned int dir);
VECTOR3 hexPole_centerDistanceSpace (unsigned int dir);

void v2c (const VECTOR3 * const vector, int * x, int * y);
// this assumes a regular hexagon and a vector with the hex centre as the origin
bool pointInHex (const VECTOR3 * const point);

unsigned int baryInterpolate (const VECTOR3 const * p, const VECTOR3 const * c1, const VECTOR3 const * c2, const unsigned int v1, const unsigned int v2, const unsigned int v3);
void baryWeights (const VECTOR3 const * p, const VECTOR3 const * c2, const VECTOR3 const * c3, float * weights);

#endif /* XPH_HEX_UTILITY_H */