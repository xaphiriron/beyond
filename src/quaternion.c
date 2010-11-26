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
