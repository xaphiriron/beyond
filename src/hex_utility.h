#ifndef XPH_HEX_UTILITY_H
#define XPH_HEX_UTILITY_H

#include <assert.h>
#include <float.h>
#include <stdlib.h>

#include "bool.h"
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

void hex_nextValidCoord (unsigned int * rp, unsigned int * kp, unsigned int * ip);
bool hex_wellformedRKI (unsigned int r, unsigned int k, unsigned int i);
void hex_rki2xy (unsigned int r, unsigned int k, unsigned int i, signed int * xp, signed int * yp);
void hex_xy2rki (signed int x, signed int y, unsigned int * rp, unsigned int * kp, unsigned int * ip);

unsigned int hex_linearCoord (unsigned int r, unsigned int k, unsigned int i);
unsigned int hex_distanceBetween (signed int ax, signed int ay, signed int bx, signed int by);
unsigned int hexMagnitude (signed int x, signed int y);

/* this calculates the x,y coordinate offset between the central hex of two adjacent grounds, both of radius {radius}, with the direction between the two being {dir} and the result set to {xp} and {yp}
 */
bool hex_centerDistanceCoord (unsigned int radius, unsigned int dir, signed int * xp, signed int * yp);
bool hexGround_centerDistanceCoord (unsigned int radius, unsigned int dir, signed int * xp, signed int * yp);
bool hexPole_centerDistanceCoord (unsigned int dir, signed int * xp, signed int * yp);

unsigned char hex_dirHashFromYaw (float yaw);
unsigned char hex_dirHashFromCoord (signed int x, signed int y);
unsigned char hex_dirHashCmp (unsigned char a, unsigned char b);

/***
 * generic hex functions involving vectors:
 */

VECTOR3 hex_tileDistance (int length, unsigned int dir);
VECTOR3 hex_coord2space (unsigned int r, unsigned int k, unsigned int i);
VECTOR3 hex_xyCoord2Space (signed int x, signed int y);
bool hex_space2coord (const VECTOR3 * space, signed int * xp, signed int * yp);
VECTOR3 hexGround_centerDistanceSpace (unsigned int radius, unsigned int dir);
VECTOR3 hexPole_centerDistanceSpace (unsigned int dir);


#endif /* XPH_HEX_UTILITY_H */