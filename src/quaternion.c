/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#include "quaternion.h"

QUAT quat_create (const float w, const float x, const float y, const float z)
{
	QUAT
		q;
	q.w = w;
	q.x = x;
	q.y = y;
	q.z = z;
	return q;
}

QUAT quat_eulerToQuat (const float x, const float y, const float z)
{
	QUAT
		q;
	float
		rx = x / 180.0 * M_PI_2,
		ry = y / 180.0 * M_PI_2,
		rz = z / 180.0 * M_PI_2,
		cx = cos (rx),
		cy = cos (ry),
		cz = cos (rz),
		sx = sin (rx),
		sy = sin (ry),
		sz = sin (rz);
	q.w = cx * cy * cz + sx * sy * sz;
	q.x = sx * cy * cz - cx * sy * sz;
	q.y = cx * sy * cz + sx * cy * sz;
	q.z = cx * cy * sz - sx * sy * cz;
	q = quat_normalize (&q);
	return q;
}

QUAT quat_normalize (const QUAT * q)
{
	QUAT
		r;
	float
		d = sqrt (
			q->w * q->w +
			q->x * q->x +
			q->y * q->y +
			q->z * q->z
		);
	r.w = q->w / d;
	r.x = q->x / d;
	r.y = q->y / d;
	r.z = q->z / d;
	return r;
}

QUAT quat_conjugate (const QUAT * q)
{
	QUAT
		r;
	r.w = q->w;
	r.x = -q->x;
	r.y = -q->y;
	r.z = -q->z;
	return r;
}

QUAT quat_multiply (const QUAT * a, const QUAT * b)
{
	QUAT
		r;
	r.w = a->w * b->w - a->x * b->x - a->y * b->y - a->z * b->z;
	r.x = a->w * b->x + a->x * b->w + a->y * b->z - a->z * b->y;
	r.y = a->w * b->y - a->x * b->z + a->y * b->w + a->z * b->x;
	r.z = a->w * b->z + a->x * b->y - a->y * b->x + a->z * b->w;
	return r;
}

void quat_quatToMatrixf (const QUAT * q, float * m)
{
	if (m == (void *)0)
		return;
	m[3] = m[7] = m[11] = m[12] = m[13] = m[14] = 0.0;
	m[15] = 1.0;
	m[0] =
		1 -
		2 * q->y * q->y -
		2 * q->z * q->z;
	m[1] =
		2 * q->x * q->y +
		2 * q->w * q->z;
	m[2] =
		2 * q->x * q->z -
		2 * q->w * q->y;
	m[4] =
		2 * q->x * q->y -
		2 * q->w * q->z;
	m[5] =
		1 -
		2 * q->x * q->x -
		2 * q->z * q->z;
	m[6] =
		2 * q->y * q->z +
		2 * q->w * q->x;
	m[8] =
		2 * q->x * q->z +
		2 * q->w * q->y;
	m[9] =
		2 * q->y * q->z -
		2 * q->w * q->x;
	m[10] =
		1 -
		2 * q->x * q->x -
		2 * q->y * q->y;
}

void quat_quatToMatrixd (const QUAT * q, double * m)
{
	if (m == (void *)0)
		return;
	m[3] = m[7] = m[11] = m[12] = m[13] = m[14] = 0.0;
	m[15] = 1.0;
	m[0] =
		1 -
		2 * q->y * q->y -
		2 * q->z * q->z;
	m[1] =
		2 * q->x * q->y +
		2 * q->w * q->z;
	m[2] =
		2 * q->x * q->z -
		2 * q->w * q->y;
	m[4] =
		2 * q->x * q->y -
		2 * q->w * q->z;
	m[5] =
		1 -
		2 * q->x * q->x -
		2 * q->z * q->z;
	m[6] =
		2 * q->y * q->z +
		2 * q->w * q->x;
	m[8] =
		2 * q->x * q->z +
		2 * q->w * q->y;
	m[9] =
		2 * q->y * q->z -
		2 * q->w * q->x;
	m[10] =
		1 -
		2 * q->x * q->x -
		2 * q->y * q->y;
}

bool quat_cmp (const QUAT * a, const QUAT * b)
{
	return
		fcmp (a->w, b->w) &&
		fcmp (a->x, b->x) &&
		fcmp (a->y, b->y) &&
		fcmp (a->z, b->z);
}
