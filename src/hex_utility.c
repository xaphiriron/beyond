#include "hex_utility.h"

#include "xph_log.h"

/***
 * You don't want to change these values unless you _really_ know what you're
 * doing. Certain functions depend on the specific ordering of H, or of XY, or
 * of the mapping between them. Changing any value without regard for all the
 * functions which refer to it is just asking for everything to fail.
 * (Thankfully, most of those functions are in this file.)
 */

// VVV THIS IS OUTDATED VVV
/***
 * Here are the things that depend on these arrays being in this exact order:
 * label_distanceFromOrigin in ground_draw.c uses the XY array to calculating
 * spacing distance. It requires the indices of the {1,0} and {0,1} entries on
 * the array, for use as X and Y axes.
 * hex_draw in hex_draw.c uses the H array to draw hexes. More importantly, it
 * calculates the camera heading to do some basic hex edge culling, and those
 * calculations will fail if the order of H is changed at all.
 * hex_coordinateAtSpace requires a specific mapping between the H and the XY
 * values. Quite literally the only mapping that will work is one one you
 * (hopefully) see below. If you re-order one array for any reason, you MUST
 * maintain the ordering of the other array.
 *
 * Furthermore, ground_getGroundOverCoordinate in component_ground.c depends
 * on the grounds themselves having a certain linkage spin (clockwise).
 * "Linkage spin" is defined as the way the grounds themselves are drawn; e.g.,
 * if you are standing at the center of a ground and proceed straight along
 * the edges of the corner hexes, when you hit the edge of the ground you will
 * either end up on the rightmost column of the ground to your left (clockwise)
 * or the leftmost column of the ground to your right (counter-clockwise).
 * This is an important property of the grounds and I have no clue where it is
 * calculated. But if it changes, ground_bridgeConnections will break when
 * walking across grounds over the corner hexes;
 *
 * What I am trying to say here is don't change any of these values unless you
 * are very sure of what you are doing.
 */

const signed short H [6][2] =
{
	{ 15, 26},
	{-15, 26},
	{-30,  0},
	{-15,-26},
	{ 15,-26},
	{ 30,  0},
 };

const signed char XY [6][2] =
{
	{1,0},
	{0,1},
	{-1,1},
	{-1,0},
	{0,-1},
	{1,-1},
};

static unsigned int
	poleRadius = 65535,
	groundRadius = 8;

/***
 * this really ought to be 3 * n * (n + 1) + 1 instead, but so many fussy geometry functions depend on this that i'm afraid to change it
 * (the reason why is +1 leads to a progression of 1 7 19 37 etc, whereas -1 leads to a progression of 1 1 7 19 etc., and that leads to having to count from 1 instead of from 0)
 */
unsigned int hx (unsigned int n)
{
	return 3 * n * (n - 1) + 1;
}

unsigned int fx (unsigned int n)
{
	return 3 * n * (n + 1) + 1;
}


void hex_nextValidCoord (unsigned int * rp, unsigned int * kp, unsigned int * ip)
{
	if (rp == NULL || kp == NULL || ip == NULL)
		return;
	if (*rp != 0 && *ip < (*rp) - 1)
	{
		(*ip)++;
		return;
	}
	if (*kp < 5 && *rp > 0)
	{
		(*kp)++;
		*ip = 0;
		return;
	}
	*kp = 0;
	*ip = 0;
	(*rp)++;
}

bool hex_wellformedRKI (unsigned int r, unsigned int k, unsigned int i)
{
	if (r == 0 && i == 0)
		return true;
	if (r < 0 || k < 0 || k >= 6 || i < 0 || i >= r)
		return false;
	return true;
}

void hex_rki2xy (unsigned int r, unsigned int k, unsigned int i, signed int * xp, signed int * yp)
{
	int
		kval = k % 6,
		ival = (k + 2) % 6;
	if (xp != NULL && yp != NULL)
	{
		*xp = XY[kval][X] * r + XY[ival][X] * i;
		*yp = XY[kval][Y] * r + XY[ival][Y] * i;
	}
	//printf ("%s: returning {%d %d %d}/%d,%d\n", __FUNCTION__, r, k, i, XY[kval][0] * r + XY[ival][0] * i, XY[kval][1] * r + XY[ival][1] * i);
}

void hex_xy2rki (signed int x, signed int y, unsigned int * rp, unsigned int * kp, unsigned int * ip)
{
	int
		o = 0,
		kx, ky,
		ix, iy,
		r, k, i;
	if (x == 0 && y == 0)
	{
		r = 0;
		k = 0;
		i = 0;
	}
	else if ((x < 0) == (y < 0))
	{
		r = abs (x + y);
	}
	else
	{
		r = abs (x) > abs (y)
			? abs (x)
			: abs (y);
	}
	while (o < 6)
	{
		kx = x - XY[o][X] * r;
		ky = y - XY[o][Y] * r;
		k = o;
		if (kx == 0 && ky == 0)
		{
			i = 0;
			break;
		}
		if ((kx < 0) == (ky < 0))
		{
			i = abs (kx + ky);
		}
		else
		{
			i = abs (kx) > abs (ky)
				? abs (kx)
				: abs (ky);
		}
		if (r == i)
		{
			/* without this, the function calculates things like {1 1 0} as
			 * {1 0 1}, since they resolve to the same tile (despite the
			 * latter being an invalid coordinate)
			 */
			o++;
			continue;
		}
		ix = kx - XY[(o + 2) % 6][X] * i;
		iy = ky - XY[(o + 2) % 6][Y] * i;
		if (ix == 0 && iy == 0)
		{
			break;
		}
		o++;
	}
	if (rp != NULL && kp != NULL && ip != NULL)
	{
		*rp = r;
		*kp = k;
		*ip = i;
	}
	//printf ("%s: returning %d,%d/{%d %d %d}\n", __FUNCTION__, x, y, r, k, i);
}

unsigned int hex_linearCoord (unsigned int r, unsigned int k, unsigned int i)
{
	return (r == 0 && k == 0 && i == 0)
		? 0
		: hx (r) + (k * r) + i;
}

unsigned int hex_linearXY (signed int x, signed int y)
{
	unsigned int
		r, k, i;
	hex_xy2rki (x, y, &r, &k, &i);
	return hex_linearCoord (r, k, i);
}

void hex_unlineate (int l, signed int * x, signed int * y)
{
	unsigned int
		r = 0,
		k = 0,
		i = 0;
	int
		remainder;
	if (l == 0)
	{
		*x = 0;
		*y = 0;
		return;
	}
	//printf ("%s: have %d\n", __FUNCTION__, l);
	while (fx (r) <= l)
	{
		r++;
	}
	//printf ("%s: %d (fx (%d)) is smallest hex larger than l-val\n", __FUNCTION__, fx (r), r);
	remainder = l - fx (r - 1);
	//printf ("%s: %d remaining with r assigned as %d\n", __FUNCTION__, remainder, r);
	assert (r != 0);
	k = remainder / r;
	i = remainder % r;
	//printf ("got {%d %d %d} from %d\n", r, k, i, l);
	hex_rki2xy (r, k, i, x, y);
	
	return;
}

unsigned int hex_distanceBetween (signed int ax, signed int ay, signed int bx, signed int by)
{
	signed int
		x = ax - bx,
		y = ay - by;
	return hexMagnitude (x, y);
}

unsigned int hexMagnitude (signed int x, signed int y)
{
	if ((x ^ y) < 0)
		return abs (x) > abs (y)
			? abs (x)
			: abs (y);
	else
		return abs (x + y);
}

bool hex_centerDistanceCoord (unsigned int radius, unsigned int dir, signed int * xp, signed int * yp)
{
	if (xp == NULL || yp == NULL)
		return false;
	*xp = XY[dir][X] * (radius + 1) + XY[DIR1MOD6 (dir)][X] * radius;
	*yp = XY[dir][Y] * (radius + 1) + XY[DIR1MOD6 (dir)][Y] * radius;
	return true;
}

bool hexGround_centerDistanceCoord (unsigned int UNUSED, unsigned int dir, signed int * xp, signed int * yp)
{
	return hex_centerDistanceCoord (groundRadius, dir, xp, yp);
}

bool hexPole_centerDistanceCoord (unsigned int dir, signed int * xp, signed int * yp)
{
	return hex_centerDistanceCoord (poleRadius, dir, xp, yp);
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

/***
 * generic hex functions involving vectors:
 */

VECTOR3 hex_tileDistance (int length, unsigned int dir)
{
	int
		l = (dir + 5) % 6;
	VECTOR3
		r = vectorCreate (0.0, 0.0, 0.0);
	if (length == 0 || dir < 0 || dir >= 6)
	{
		return r;
	}
	r = vectorCreate (H[dir][0] + H[l][0], 0.0, H[dir][1] + H[l][1]);
	r = vectorMultiplyByScalar (&r, length);
	return r;
}

VECTOR3 hex_coord2space (unsigned int r, unsigned int k, unsigned int i)
{
	VECTOR3
		p = vectorCreate (0.0, 0.0, 0.0),
		q;
	if (r == 0)
	{
		return p;
	}
	assert (k >= 0 && k < 6);
	assert (i < r);
	p = hex_tileDistance (r, k);
	if (i == 0)
	{
		return p;
	}
	q = hex_tileDistance (i, (k + 2) % 6);
	p = vectorAdd (&p, &q);
	return p;
}

VECTOR3 hex_xyCoord2Space (signed int x, signed int y)
{
	VECTOR3
		p,
		q;
	p = hex_tileDistance (x, 0);
	q = hex_tileDistance (y, 1);
	q = vectorAdd (&p, &q);
	return q;
}

VECTOR3 hexGround_centerDistanceSpace (unsigned int UNUSED, unsigned int dir)
{
	signed int
		x, y;
	VECTOR3
		t, u;
	hexGround_centerDistanceCoord (groundRadius, dir, &x, &y);
	t = hex_tileDistance (x, 0);
	u = hex_tileDistance (y, 1);
	t = vectorAdd (&t, &u);
	return t;
}

VECTOR3 hexPole_centerDistanceSpace (unsigned int dir)
{
	signed int
		x, y;
	VECTOR3
		t, u;
	hexPole_centerDistanceCoord (dir, &x, &y);
	t = hex_tileDistance (x, 0);
	u = hex_tileDistance (y, 1);
	t = vectorAdd (&t, &u);
	return t;
}


void v2c (const VECTOR3 * const vector, int * x, int * y)
{
	int
		xGrid,
		yGrid,
		i = 0;
	VECTOR3
		offset,
		centre;
	assert (x != NULL && y != NULL);
	
	xGrid = (int)(vector->x / 45.0);
	yGrid = (int)((vector->z - xGrid * 26.0) / 52.0);
	centre = hex_xyCoord2Space (xGrid, yGrid);
	offset = vectorSubtract (vector, &centre);
	if (pointInHex (&offset))
	{
		*x = xGrid;
		*y = yGrid;
		//printf ("%.2f, %.2f -> %d, %d\n", vector->x, vector->z, *x, *y);
		return;
	}
	while (i < 6)
	{
		centre = hex_xyCoord2Space (xGrid + XY[i][X], yGrid + XY[i][Y]);
		offset = vectorSubtract (vector, &centre);
		if (pointInHex (&offset))
		{
			*x = xGrid + XY[i][X];
			*y = yGrid + XY[i][Y];
			//printf ("%.2f, %.2f -> %d, %d\n", vector->x, vector->z, *x, *y);
			return;
		}
		i++;
	}
	assert (false);
}

bool pointInHex (const VECTOR3 * const point)
{
	int
		i = 0,
		j,
		t;
	//printf ("intersection point: %.2f, %.2f, %.2f\n", point->x, point->y, point->z);
	while (i < 6)
	{
		j = i == 5 ? 0 : i + 1;
		t = turns (point->x, point->z, H[i][X], H[i][Y], H[i][X] + H[j][X], H[i][Y] + H[j][Y]);
		//printf ("%d: %s\n", i, t == LEFT ? "LEFT" : t == RIGHT ? "RIGHT" : "THROUGH");
		if (t == RIGHT)
			return false;
		i++;
	}
	return true;
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

	if (weight[0] < 0.0 || weight[0] > 1.0 ||
		weight[1] < 0.0 || weight[1] > 1.0 ||
		weight[2] < 0.0 || weight[2] > 1.0)
	{
		ERROR ("Coordinate outside barycentric triangle; final result was %.2f %.2f %.2f", weight[0], weight[1], weight[2]);
	}
}