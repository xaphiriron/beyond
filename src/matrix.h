/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#ifndef XPH_MATRIX_H
#define XPH_MATRIX_H

float matrixHeading (const float * const m);

void matrixMultiplyf (const float * a, const float * b, float * r);

#endif /* XPH_MATRIX_H */