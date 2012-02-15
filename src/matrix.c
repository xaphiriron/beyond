/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#include "matrix.h"

#include "fcmp.h"

/* MATRIX INDICES:
 * 0  4  8 12
 * 1  5  9 13
 * 2  6 10 14
 * 3  7 11 15
 */

float matrixHeading (const float * const m)
{
	if (m == (void *)0)
		return 0.0;
	if (fcmp (m[1], 1.0) || fcmp (m[1], -1.0))
		return atan2 (m[8], m[10]);
	return atan2 (-m[2], m[0]);
}

/* the args are 'reversed' since I wrote the code to postmultiply (i think)
 * when I really wanted it to premultiply (i think). I am not very good at
 * math. :(
 */
void matrixMultiplyf (const float * b, const float * a, float * r)
{
	r[0]  = a[ 0] * b[ 0] +
			a[ 1] * b[ 4] +
			a[ 2] * b[ 8] +
			a[ 3] * b[12];
	r[1]  = a[ 0] * b[ 1] +
			a[ 1] * b[ 5] +
			a[ 2] * b[ 9] +
			a[ 3] * b[13];
	r[2]  = a[ 0] * b[ 2] +
			a[ 1] * b[ 6] +
			a[ 2] * b[10] +
			a[ 3] * b[14];
	r[3]  = a[ 0] * b[ 3] +
			a[ 1] * b[ 7] +
			a[ 2] * b[11] +
			a[ 3] * b[15];

	r[4]  = a[ 4] * b[ 0] +
			a[ 5] * b[ 4] +
			a[ 6] * b[ 8] +
			a[ 7] * b[12];
	r[5]  = a[ 4] * b[ 1] +
			a[ 5] * b[ 5] +
			a[ 6] * b[ 9] +
			a[ 7] * b[13];
	r[6]  = a[ 4] * b[ 2] +
			a[ 5] * b[ 6] +
			a[ 6] * b[10] +
			a[ 7] * b[14];
	r[7]  = a[ 4] * b[ 3] +
			a[ 5] * b[ 7] +
			a[ 6] * b[11] +
			a[ 7] * b[15];

	r[8]  = a[ 8] * b[ 0] +
			a[ 9] * b[ 4] +
			a[10] * b[ 8] +
			a[11] * b[12];
	r[9]  = a[ 8] * b[ 1] +
			a[ 9] * b[ 5] +
			a[10] * b[ 9] +
			a[11] * b[13];
	r[10] = a[ 8] * b[ 2] +
			a[ 9] * b[ 6] +
			a[10] * b[10] +
			a[11] * b[14];
	r[11] = a[ 8] * b[ 3] +
			a[ 9] * b[ 7] +
			a[10] * b[11] +
			a[11] * b[15];

	r[12] = a[12] * b[ 0] +
			a[13] * b[ 4] +
			a[14] * b[ 8] +
			a[15] * b[12];
	r[13] = a[12] * b[ 1] +
			a[13] * b[ 5] +
			a[14] * b[ 9] +
			a[15] * b[13];
	r[14] = a[12] * b[ 2] +
			a[13] * b[ 6] +
			a[14] * b[10] +
			a[15] * b[14];
	r[15] = a[12] * b[ 3] +
			a[13] * b[ 6] +
			a[14] * b[11] +
			a[15] * b[15];
}
