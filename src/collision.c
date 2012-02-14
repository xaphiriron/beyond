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

bool line_triHit (const LINE3 * const line, const TRI3 * const tri, float * tAtIntersection)
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

	//printf ("RAY:\n\tORIGIN: %.2f, %.2f, %2.f\n\tDIR: %.2f, %.2f, %.2f\n", line->origin.x, rayOrigin->y, rayOrigin->z, rayDir->x, rayDir->y, rayDir->z);
	//printf ("TRI:\n\t[0]: %.2f, %.2f, %.2f\n\t[1]: %.2f, %.2f, %.2f\n\t[2]: %.2f, %.2f, %.2f\n", tri->pts[0].x, tri->pts[0].y, tri->pts[0].z, tri->pts[1].x, tri->pts[1].y, tri->pts[1].z, tri->pts[2].x, tri->pts[2].y, tri->pts[2].z);
	u = vectorSubtract (&tri->pts[2], &tri->pts[0]);
	v = vectorSubtract (&tri->pts[1], &tri->pts[0]);
	triPlane = vectorCross (&u, &v);
	triPlane = vectorNormalize (&triPlane);
	// the planeHit code doesn't have a magnitude argument so it's always a test against the plane through the origin. thus to accurately test we have to translate the line so that it's positioned relative to the plane origin (which we're arbitrarily using tri->pts[0] as) - xph 02 13 2012
	transLine.origin = vectorSubtract (&line->origin, &tri->pts[0]);
	transLine.dir = line->dir;
	//printf ("collision test: %.2f, %.2f, %.2f -> %.2f, %.2f, %.2f\n", transLine.origin.x, transLine.origin.y, transLine.origin.z, transLine.dir.x, transLine.dir.y, transLine.dir.z);
	if (!line_planeHit (&transLine, &triPlane, &t))
		return false;
	//printf ("line %.2f, %.2f, %.2f -> %.2f, %.2f, %.2f hit plane w/ normal %.2f, %.2f, %.2f at %.2f, %.2f, %.2f\n", line->origin.x, line->origin.y, line->origin.z, line->dir.x, line->dir.y, line->dir.z, triPlane.x, triPlane.y, triPlane.z, line->origin.x + line->dir.x * t, line->origin.y + line->dir.y * t, line->origin.z + line->dir.z * t);
	hit = vectorCreate
	(
		transLine.origin.x + line->dir.x * t,
		transLine.origin.y + line->dir.y * t,
		transLine.origin.z + line->dir.z * t
	);

	//printf ("BARYCENTRIC COORDS:\n\thit: %.2f, %.2f, %.2f\n\tu: %.2f, %.2f, %.2f\n\tv: %.2f, %.2f, %2.f\n", hit.x, hit.y, hit.z, u.x, u.y, u.z, v.x, v.y, v.z);

	baryWeights (&hit, &u, &v, weight);
	//printf ("WEIGHTS:\n\t%.2f, %.2f, %.2f\n", weight[0], weight[1], weight[2]);
	if (weight[0] >= 0.00 && weight[0] <= 1.00 &&
		weight[1] >= 0.00 && weight[1] <= 1.00 &&
		weight[2] >= 0.00 && weight[2] <= 1.00)
	{
		if (tAtIntersection)
			*tAtIntersection = t;
		//printf (" -- HIT TRI -- \n");
		return true;
	}

	return false;
}

bool line_quadHit (const LINE3 * const line, const VECTOR3 * const quad)
{
	VECTOR3
		u, v,
		plane,
		hit;
	LINE3
		transLine;
	float
		t;
	u = vectorSubtract (&quad[1], &quad[0]);
	v = vectorSubtract (&quad[2], &quad[0]);
	plane = vectorCross (&u, &v);
	plane = vectorNormalize (&plane);

	// see note in the line_triHit function
	transLine.origin = vectorSubtract (&line->origin, &quad[0]);
	transLine.dir = line->dir;
	if (!line_planeHit (&transLine, &plane, &t))
		return false;
	hit = vectorCreate
	(
		transLine.origin.x + line->dir.x * t,
		transLine.origin.y + line->dir.y * t,
		transLine.origin.z + line->dir.z * t
	);
	
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

bool pointInPoly (const VECTOR3 * const point, enum dim_drop drop, int points, const VECTOR3 * const poly)
{
	int
		i = 0;
	while (i < points)
	{
		//if (turns () != RIGHT)
			return false;
		i++;
	}
	return false;
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
