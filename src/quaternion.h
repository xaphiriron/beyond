/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#ifndef XPH_QUATERNION_H
#define XPH_QUATERNION_H

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include "fcmp.h"

typedef struct Quaternion
{
	float w, x, y, z;
} QUAT;

QUAT quat_create (const float w, const float x, const float y, const float z);

QUAT quat_eulerToQuat (const float x, const float y, const float z);

QUAT quat_normalize (const QUAT * q);

QUAT quat_conjugate (const QUAT * q);
QUAT quat_multiply (const QUAT * a, const QUAT * b);

void quat_quatToMatrixf (const QUAT * q, float * m);
void quat_quatToMatrixd (const QUAT * q, double * m);

bool quat_cmp (const QUAT * a, const QUAT * b);

#endif /* XPH_QUATERNION_H */