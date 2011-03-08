#include "hex_utility.h"

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

unsigned int hex (unsigned int n)
{
	return 3 * n * (n - 1) + 1;
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
		return TRUE;
	if (r < 0 || k < 0 || k >= 6 || i < 0 || i >= r)
		return FALSE;
	return TRUE;
}

void hex_rki2xy (unsigned int r, unsigned int k, unsigned int i, signed int * xp, signed int * yp)
{
	int
		kval = k % 6,
		ival = (k + 2) % 6;
	if (xp != NULL && yp != NULL)
	{
		*xp = XY[kval][0] * r + XY[ival][0] * i;
		*yp = XY[kval][1] * r + XY[ival][1] * i;
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
		kx = x - XY[o][0] * r;
		ky = y - XY[o][1] * r;
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
			i = abs (kx) > abs (ky) ? abs (kx) : abs (ky);
		}
		if (r == i)
		{
			// without this, it calculates things like {1 1 0} as {1 0 1},
			// since they resolve to the same tile (despite the latter being
			// an invalid coordinate)
			o++;
			continue;
		}
		ix = kx - XY[(o + 2) % 6][0] * i;
		iy = ky - XY[(o + 2) % 6][1] * i;
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
		: hex (r) + (k * r) + i;
}

unsigned int hex_distanceBetween (signed int ax, signed int ay, signed int bx, signed int by)
{
	signed int
		x = ax - bx,
		y = ay - by;
	return hex_coordinateMagnitude (x, y);
}

unsigned int hex_coordinateMagnitude (signed int x, signed int y)
{
	if ((x ^ y) < 0)
		return abs (x) > abs (y)
			? abs (x)
			: abs (y);
	else
		return abs (x + y);
}

bool hexGround_centerDistanceCoord (unsigned int radius, unsigned int dir, signed int * xp, signed int * yp)
{
	if (xp == NULL || yp == NULL)
		return FALSE;
	*xp = XY[dir][0] * (radius + 1) + XY[DIR1MOD6 (dir)][0] * radius;
	*yp = XY[dir][1] * (radius + 1) + XY[DIR1MOD6 (dir)][1] * radius;
	return TRUE;
}



unsigned char hex_dirHashFromYaw (float yaw)
{
	float
		f = yaw < 0
			? (yaw + M_PI * 2) / (M_PI * 2) * 6
			: yaw / (M_PI * 2) * 6;
	unsigned char
		h = 0;
	if (f >= 6.0)
		f -= 6.0;
	h |= (int)f << 5;
	h |= (int)((f - (int)f) * 31) & 0x1f;
	return h;
}

unsigned char hex_dirHashFromCoord (signed int x, signed int y)
{
	unsigned int
		r, k, i;
	unsigned char
		h = 0;
	float
		f = ((i / (float)r) * 31 + .5);
	if (f > 32.0)
	{
		k = (k + 1) % 6;
		f -= 32.0;
	}
	hex_xy2rki (x, y, &r,&k, &i);
	h |= k << 5;
	h |= (int)f & 0x1f;
	return h;
}

unsigned char hex_dirHashCmp (unsigned char a, unsigned char b)
{
	signed short
		r1, r2;
	r1 = a - b;
	r2 = b - a;
/*
	printf ("r1: %d; r2: %d; picked: %d\n", r1, r2, (r1 < r2 || r2 < 0
		? r1
		: r2));
*/
	return r1 < r2 || r2 < 0
		? r1
		: r2;
}
/*

def angle2hash (n):
  fi = int(n);
  ff = n - fi;
  h = 0;
  h |= fi << 5;
  h |= int (ff * 31);
  return h;


def hashCmp (a, b):
  r1 = a - b;
  ax = a + 96;
  axi = ((ax & 224) >> 5) % 6;
  ax &= ~224;
  ax |= axi << 5;
  bx = b + 96;
  bxi = ((bx & 224) >> 5) % 6;
  bx &= ~224;
  bx |= bxi << 5;
  ax &= 255;
  bx &= 255;
  r2 = ax - bx;
  r2 += -96 if r2 < 0 else 96;
  return [a, b, ax, bx, r1, r2];





  return r1 if (r1 < r2 and r1 > 0) or r2 < 0 else r2;



*/



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

/***
 * the magic numbers here are based on the x and y spacing of tiles. see the H
 * value above, or just assume this is correct: the x value is 15 and seen most
 * often in the form 45 or 90 (since the x magnitude of each tile is 45 units).
 * the y value is 26 and seen most often in the form 26 or 52 (since the y
 * magnitude of each tile is 52 units)
 */
bool hex_space2coord (const VECTOR3 * space, signed int * xp, signed int * yp)
{
	float
		xPosOffsetFromGridPoint = fmod (space->x, 45.0),
		xGrid1 = space->x - xPosOffsetFromGridPoint,
		xGrid2 = space->x + (45 - xPosOffsetFromGridPoint),
		yPosOffsetFromGridPoint = fmod (space->z, 26.0),
		yGrid1 = space->z - yPosOffsetFromGridPoint,
		yGrid2 = space->z + (26 - yPosOffsetFromGridPoint),
		swap = 0,
		xDist, yDist,
		mag1, mag2;
	signed int
		xCoord,
		yCoord;
	if (xp == NULL || yp == NULL)
	{
		return FALSE;
	}
	if (fcmp (fmod (xGrid1, 90.0), 0.0) == TRUE)
	{
		swap = xGrid1;
		xGrid1 = xGrid2;
		xGrid2 = swap;
	}
	if (fcmp (fmod (yGrid1, 52.0), 0.0) == TRUE)
	{
		swap = yGrid1;
		yGrid1 = yGrid2;
		yGrid2 = swap;
	}
	xDist = space->x - xGrid1;
	yDist = space->z - yGrid1;
	mag1 = sqrt (xDist * xDist + yDist * yDist);
	xDist = space->x - xGrid2;
	yDist = space->z - yGrid2;
	mag2 = sqrt (xDist * xDist + yDist * yDist);
	//printf ("#1: %5.2f, %5.2f (d: %f); #2: %5.2f, %5.2f (d: %f)\n", xGrid1, yGrid1, mag1, xGrid2, yGrid2, mag2);
	if (mag1 < mag2)
	{
		xCoord = (int)(xGrid1 / 45.0);
		yCoord = (int)((yGrid1 - xCoord * 26.0) / 52.0);
	}
	else
	{
		xCoord = (int)(xGrid2 / 45.0);
		yCoord = (int)((yGrid2 - xCoord * 26.0) / 52.0);
	}
	//printf ("\tfinal coord @ %d, %d\n", xCoord, yCoord);
	*xp = xCoord;
	*yp = yCoord;
	return TRUE;
}

VECTOR3 hexGround_centerDistanceSpace (unsigned int radius, unsigned int dir)
{
	signed int
		x, y;
	VECTOR3
		t, u;
	hexGround_centerDistanceCoord (radius, dir, &x, &y);
	t = hex_tileDistance (x, 0);
	u = hex_tileDistance (y, 1);
	t = vectorAdd (&t, &u);
	return t;
}
