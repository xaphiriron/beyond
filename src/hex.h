#ifndef XPH_HEX_H
#define XPH_HEX_H

#include "xph_memory.h"
#include "xph_log.h"
#include "vector.h"
#include "hex_utility.h"
#include <limits.h>

typedef struct hex * Hex;

struct hex {
	unsigned int
		r, k, i;	// polar coordinates from center of ground
	signed int
		x, y;		// cartesian coordinates from center of ground

// ^ DEPRECATED

	unsigned int
		centre;
	unsigned int
		edgeBase[12];
	unsigned char
		corners[3];
};

/*
void hex_bakeEdges (struct hex * h);
void hex_bakeEdge (struct hex * h, int dir, struct hex * adj);
*/



int __v, __n;
#define GETCORNER(p, n)		(__n = (n), __v = (__n % 2 ? p[__n/2] & 0x0f : (p[__n/2] & 0xf0) >> 4), (__v & 8) ? 0 - ((__v & 7)) : __v)
#define SETCORNER(p, n, v)	(__n = (n), __v = (v), __v = (__v < 0 ? ((~__v & 15) + 1) | 8 : __v & 7), p[__n/2] = (__n % 2 ? (p[__n/2] & 0x0f) | (__v << 4) : (p[__n/2] & 0xf0) | __v))

#define FULLHEIGHT(hex, i)		(__v = GETCORNER (hex->corners, i), (__v < 0 && hex->centre < abs (__v)) ? 0 : ((__v > 0 && hex->centre > (UINT_MAX - __v)) ? UINT_MAX : hex->centre + __v))


#define HEX_SIZE	(float)30
#define HEX_SIZE_4	(float)7.5

typedef struct hex * HEX;

HEX hex_create (unsigned int r, unsigned int k, unsigned int i, float height);
void hex_destroy (HEX h);
/*
HEX hex_create ();
*/
void hexSetHeight (HEX hex, unsigned short height);
void hexSetCorners (HEX hex, short a, short b, short c, short d, short e, short f);
// i want some kind of 'pullCorner' function that takes a hex and a corner and shifts that corner up, tugging up the center and its adjacent corners if their difference is above a certain value. ...except that since that value would likely be '1', it reduces to something like "if value >= 2, shift centre up by value, shift all corners down by value", except not really.
// it should be noted that the above is not actually what pullcorner does. right now
void hexPullCorner (HEX hex, short corner);
void hexSetCornersRandom (HEX hex);
signed char hexGetCornerHeight (const HEX hex, short corner);

#endif /* XPH_HEX_H */