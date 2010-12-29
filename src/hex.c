#include "hex.h"

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

const int H [6][2] =
{
	{ 15, 26},
	{-15, 26},
	{-30,  0},
	{-15,-26},
	{ 15,-26},
	{ 30,  0},
 };
const char XY [6][2] =
{
	{1,0},
	{0,1},
	{-1,1},
	{-1,0},
	{0,-1},
	{1,-1},
};

int hex (int n)
{
	return 3 * n * (n + 1) + 1;
}

int hex_linearCoord (short r, short k, short i)
{
	return (r == 0 && k == 0 && i == 0)
		? 0
		: hex (r - 1) + (k * r) + i;
}

void hex_rki2xy (short r, short k, short i, short * xp, short * yp)
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

void hex_xy2rki (short x, short y, short * rp, short * kp, short * ip)
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

bool hex_wellformedRKI (short r, short k, short i)
{
	if (r == 0 && i == 0)
		return TRUE;
	if (r < 0 || k < 0 || k >= 6 || i < 0 || i >= r)
		return FALSE;
	return TRUE;
}

VECTOR3 hex_linearTileDistance (int length, int dir)
{
	int
		l = (dir + 5) % 6;
	VECTOR3
		r = vectorCreate (0.0, 0.0, 0.0);
	if (length <= 0 || dir < 0 || dir >= 6)
	{
		return r;
	}
	r = vectorCreate (H[dir][0] + H[l][0], 0.0, H[dir][1] + H[l][1]);
	r = vectorMultiplyByScalar (&r, length);
	return r;
}

VECTOR3 hex_coordOffset (short r, short k, short i)
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
	p = hex_linearTileDistance (r, k);
	if (i == 0)
	{
		return p;
	}
	q = hex_linearTileDistance (i, (k + 2) % 6);
	p = vectorAdd (&p, &q);
	return p;
}

bool hex_coordinateAtSpace (const VECTOR3 * space, int * xp, int * yp)
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
	int
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

struct hex * hex_create (short r, short k, short i, float height) {
  struct hex * h = xph_alloc (sizeof (struct hex));
  if (r < 0 || (r == 0 && (k != 0 || i != 0))) {
    fprintf (stderr, "%s (%d, %d, %d): invalid r-val\n", __FUNCTION__, r, k, i);
  }
  if (k < 0 || k >= 6) {
    fprintf (stderr, "%s (%d, %d, %d): invalid k-val\n", __FUNCTION__, r, k, i);
  }
  if (i >= r && i != 0) {
    fprintf (stderr, "%s (%d, %d, %d): invalid i-val\n", __FUNCTION__, r, k, i);
  }
  h->r = r;
  h->k = k;
  h->i = i;
  hex_rki2xy (r, k, i, &h->x, &h->y);
  hex_setSlope (h, HEX_TOP, height, height, height);
  hex_setSlope (h, HEX_BASE, -1.0, -1.0, -1.0);
  memset (h->neighbors, '\0', sizeof (struct hex *) * 6);
  memset (h->edgeDepth, '\0', sizeof (float) * 12);

  return h;
}

void hex_destroy (struct hex * h) {
  xph_free (h);
}

void hex_setSlope (struct hex * h, enum hex_sides side, float height, float a, float b) {
  // the a offset is H[0], the b offset is H[1]. if we change the hex mapping (which we probably will) then we will need to update this code or else everything that depends on it working (most notably, tile collisions) will break
  VECTOR3
    av, bv;
  // if we end up storing the other values, calculate them here.
  if (side == HEX_TOP) {
    h->top = height * 15;
    h->topA = a * 15;
    h->topB = b * 15;
    av = vectorCreate (H[0][0], h->topA - h->top, H[0][1]);
    bv = vectorCreate (H[1][0], h->topB - h->top, H[1][1]);
    h->topNormal = vectorCross (&bv, &av);
    h->topNormal = vectorNormalize (&h->topNormal);
  } else {
    h->base = height * 15;
    h->baseA = a * 15;
    h->baseB = b * 15;
    av = vectorCreate (H[0][0], h->baseA - h->base, H[0][1]);
    bv = vectorCreate (H[1][0], h->baseB - h->base, H[1][1]);
    h->baseNormal = vectorCross (&av, &bv);
    h->baseNormal = vectorNormalize (&h->baseNormal);
  }
}

void hex_bakeEdges (struct hex * h)
{
	struct hex
		* adj;
	int
		i = 0;
	float
		ra, rb,
		corners[6];
	if (h == NULL)
		return;
	while (i < 6)
	{
		adj = h->neighbors[(i + 1) % 6];
		if (adj == NULL)
		{
			i++;
			continue;
		}
		ra = adj->top - adj->topA;
		rb = adj->top - adj->topB;
		corners[0] = adj->topA;
		corners[1] = adj->topB;
		corners[3] = adj->top + ra;
		corners[4] = adj->top + rb;
		corners[2] = adj->top + ra + (adj->top - corners[4]);
		corners[5] = adj->top + rb + (adj->top - corners[3]);
		h->edgeDepth[i * 2] = corners[(i + 4) % 6];
		h->edgeDepth[i * 2 + 1] = corners[(i + 3) % 6];
		i++;
	}
}
