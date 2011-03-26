#ifndef XPH_QUATERNION_H
#define XPH_QUATERNION_H

#include <assert.h>
#include <math.h>
#include "bool.h"
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