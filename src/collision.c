/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#include "collision.h"

bool line_planeHit (const LINE3 * const line, const VECTOR3 * plane, float * tAtIntersection)
{
	float
		denom =
			plane->x * line->dir.x +
			plane->y * line->dir.y +
			plane->z * line->dir.z,
		t;
	if (fabs (denom) < FLT_EPSILON)
		return false;
	t = -(plane->x * line->origin.x +
		  plane->y * line->origin.y +
		  plane->z * line->origin.z) / denom;
	if (tAtIntersection)
		*tAtIntersection = t;
	return true;
}

bool line_triHit (const LINE3 * const line, const VECTOR3 * const tri, float * tAtIntersection)
{
	VECTOR3
		u, v,
		triPlane,
		hit;
	LINE3
		transLine;
	float
		t,
		weight[3];
	u = vectorSubtract (&tri[2], &tri[0]);
	v = vectorSubtract (&tri[1], &tri[0]);
	triPlane = vectorCross (&u, &v);
	// the planeHit code doesn't have a magnitude argument so it's always a test against the plane through the origin. thus to accurately test we have to translate the line so that it's positioned relative to the plane origin (which we're arbitrarily using tri->pts[0] as) - xph 02 13 2012
	transLine.origin = vectorSubtract (&line->origin, &tri[0]);
	transLine.dir = line->dir;
	if (!line_planeHit (&transLine, &triPlane, &t))
		return false;
	hit = vectorCreate
	(
		transLine.origin.x + line->dir.x * t,
		transLine.origin.y + line->dir.y * t,
		transLine.origin.z + line->dir.z * t
	);
	baryWeights (&hit, &u, &v, weight);
	if (weight[0] >= 0.00 && weight[0] <= 1.00 &&
		weight[1] >= 0.00 && weight[1] <= 1.00 &&
		weight[2] >= 0.00 && weight[2] <= 1.00)
	{
		if (tAtIntersection)
			*tAtIntersection = t;
		return true;
	}
	return false;
}

bool line_polyHit (const LINE3 * const line, int points, const VECTOR3 * const poly, float * tAtIntersection)
{
	VECTOR3
		u, v,
		plane,
		hit;
	LINE3
		transLine;
	float
		t;
	u = vectorSubtract (&poly[1], &poly[0]);
	v = vectorSubtract (&poly[2], &poly[0]);
	plane = vectorCross (&u, &v);
	plane = vectorNormalize (&plane);

	// see note in the line_triHit function
	transLine.origin = vectorSubtract (&line->origin, &poly[0]);
	transLine.dir = line->dir;
	if (!line_planeHit (&transLine, &plane, &t))
		return false;
	hit = vectorCreate
	(
		line->origin.x + line->dir.x * t,
		line->origin.y + line->dir.y * t,
		line->origin.z + line->dir.z * t
	);
	if (pointInPoly (&hit, points, poly))
	{
		if (tAtIntersection)
			*tAtIntersection = t;
		return true;
	}
	return false;
}

int turns (float ox, float oy, float lx1, float ly1, float lx2, float ly2)
{
	float
		cross = (lx1 - ox) * (ly2 - oy) - (lx2 - ox) * (ly1 - oy);
	return cross < 0.0
		? RIGHT
		: cross == 0.0
		? THROUGH
		: LEFT;
}


enum dim_drop
{
	DROP_X,
	DROP_Y,
	DROP_Z
};

bool pointInPoly (const VECTOR3 * const point, int points, const VECTOR3 * const poly)
{
	VECTOR3
		u, v,
		plane;
	enum dim_drop
		drop;
	int
		i = 0,
		side,
		sideMatch = 10;
	float
		x, y,
		px[2], py[2];
	if (points < 3)
		return false;
	u = vectorSubtract (&poly[1], &poly[0]);
	v = vectorSubtract (&poly[2], &poly[0]);
	plane = vectorCross (&u, &v);
	drop =
		(fabs(plane.x) > fabs(plane.y))
			? (fabs(plane.x) > fabs(plane.z))
				? DROP_X
				: DROP_Z
			: (fabs(plane.y) > fabs(plane.z))
				? DROP_Y
				: DROP_Z;

	//printf ("dropping %c\n", drop == DROP_X ? 'X' : drop == DROP_Y ? 'Y' : 'Z');
	x = (drop == DROP_X) ? point->y : point->x;
	y = (drop == DROP_Z) ? point->y : point->z;
	px[1] = (drop == DROP_X) ? poly[points-1].y : poly[points-1].x;
	py[1] = (drop == DROP_Z) ? poly[points-1].y : poly[points-1].z;
	while (i < points)
	{
		px[0] = px[1];
		py[0] = py[1];
		px[1] = (drop == DROP_X) ? poly[i].y : poly[i].x;
		py[1] = (drop == DROP_Z) ? poly[i].y : poly[i].z;
		side = turns (x, y, px[0], py[0], px[1], py[1]);

		//printf ("%.2f, %.2f vs. %.2f, %.2f -> %.2f, %.2f: %c\n", x, y, px[0], py[0], px[1], py[1], d == RIGHT ? 'R' : d == LEFT ? 'L' : 'T');

		if (sideMatch != 10 && side != sideMatch)
			return false;
		sideMatch = side;
		i++;
	}
	return true;
}

// find the barycentric weights on p from the points 0,0 (implicitly c1), c2, and c3. the weights are stored in weights[0],[1], and [2]
void baryWeights (const VECTOR3 const * p, const VECTOR3 const * c2, const VECTOR3 const * c3, float * weight)
{
	float
		base = c2->x * c3->z - c3->x * c2->z;
	assert (weight != NULL);
	weight[0] = ((c2->x - p->x) * (c3->z - p->z) -
				 (c3->x - p->x) * (c2->z - p->z)) / base;
	weight[1] = ((c3->x - p->x) * -p->z -
				 -p->x * (c3->z - p->z)) / base;
	weight[2] = (-p->x * (c2->z - p->z) -
				(c2->x - p->x) * -p->z) / base;
/*
	if (weight[0] < 0.0 || weight[0] > 1.0 ||
		weight[1] < 0.0 || weight[1] > 1.0 ||
		weight[2] < 0.0 || weight[2] > 1.0)
	{
		ERROR ("Coordinate outside barycentric triangle; final result was %.2f %.2f %.2f", weight[0], weight[1], weight[2]);
	}
*/
}

/* the full method for calculating the barycentric coordinates of a point :p {0} with three vertices (in this case, 0,0 implicitly, :adjCoords[dir], and :adjCoords[lastDir] {1,2,3}) is the first calculate a normalizing factor, bc, by way of
 * b0 = (2.x - 1.x) * (3.y - 1.y) - (3.x - 1.x) * (2.y - 1.y);
 * in this case since the '1' is 0,0 this simplifies things substantially
 * from that the coordinates themselves are calculated:
 * b1 = ((2.x - 0.x) * (y.3 - 0.y) - (3.x - 0.x) * (2.y -0.y)) / bc;
 * b2 = ((3.x - 0.x) * (y.1 - 0.y) - (1.x - 0.x) * (3.y -0.y)) / bc;
 * b3 = ((1.x - 0.x) * (y.2 - 0.y) - (2.x - 0.x) * (1.y -0.y)) / bc;
 * which, again, can be simplified on the values involving 1, since
 * it's always 0,0. remember also that the world plane is the x,z
 * plane; that's why we're using z here instead of y. to interpolate
* any map data point, multiply the coordinate values with their respective map point value, so that:
* value at :p = b1 * [map value of the subhex corresponding to 1] + b2 * [same w/ 2] + b3 * [same w/ 3];
 */
unsigned int baryInterpolate (const VECTOR3 const * p, const VECTOR3 const * c2, const VECTOR3 const * c3, const unsigned int v1, const unsigned int v2, const unsigned int v3)
{
	float
		weight[3] = {0, 0, 0};
	baryWeights (p, c2, c3, weight);
	return ceil (weight[0] * v1 + weight[1] * v2 + weight[2] * v3);
}
