#include "world_position.h"

struct world_position
{
	unsigned int
		r, t;
	unsigned char
		bits;
	/*
	 * the full rotation value is GET_BITS(.bits, ...) * 6 + t.
	 * the max valid value of t is r - 1, except when r == 0 (when it is 0)
	 */
	// since t's max value is always r * 6 - 1, there's much more of a danger of t overflowing than of r overflowing. however, this issue can be resolved by using bitfields: two bytes are required to store the pole information, and three bytes are required to store a 'k' value for the polar coordinate, where the full phi value is k * 6 + t instead of just t. this constrains t to strictly < r in all cases and doesn't require adding any additional data to the position struct.
};

struct worldHex // WORLDHEX
{
	struct world_position
		wp;
	signed int
		gx, gy;
};


#define WORLD_POLE_BITS		2
#define WORLD_POLE_SHIFT	0
#define WORLD_CORNER_BITS	3
#define WORLD_CORNER_SHIFT	3

enum world_poles
{
	POLE_ERROR	= 0x00,
	POLE_R		= 0x01,
	POLE_G		= 0x02,
	POLE_B		= 0x03,
};


/***
 * FIXME: this does not work if r > poleRadius, since there is a single
 * cross-ground check. either recurse (don't recurse) or put the latter half
 * of the function inside a while newr > poleRadius loop.
 */
worldPosition wp_fromRelativeOffset (const worldPosition pos, unsigned int poleRadius, unsigned int r, unsigned int k, unsigned int i)
{
	worldPosition
		newp = wp_createEmpty ();
	signed int
		relativeX, relativeY,
		globalX, globalY,
		poleX, poleY;
	unsigned int
		nr, nk, ni;
	unsigned char
		pole;
	if (pos->r > poleRadius)
	{
		fprintf (stderr, "%s: got invalid label; given radius (%d) larger than pole radius (%d).\n", __FUNCTION__, pos->r, poleRadius);
		return NULL;
	}
	hex_rki2xy (r, k, i, &relativeX, &relativeY);
	hex_rki2xy (pos->r, GET_BITS (pos->bits, WORLD_CORNER_BITS, WORLD_CORNER_SHIFT), pos->t, &globalX, &globalY);
	//printf ("x,y coordinates of base position: %d, %d\nx,y coordinate offset of target: %d, %d\n", globalX, globalY, relativeX, relativeY);
	globalX += relativeX;
	globalY += relativeY;
	hex_xy2rki (globalX, globalY, &nr, &nk, &ni);
	pole = GET_BITS (pos->bits, WORLD_POLE_BITS, WORLD_POLE_SHIFT);
	while (nr > poleRadius)
	{
		poleX = XY[nk][0] * (poleRadius + 1) + XY[(nk + 1) % 6][0] * poleRadius;
		poleY = XY[nk][1] * (poleRadius + 1) + XY[(nk + 1) % 6][1] * poleRadius;
		globalX = globalX - poleX;
		globalY = globalY - poleY;
		if (nk % 2)
		{
			// + 1 dir
			pole = (pole + 1) & 0x03;
			if (!pole)
				pole = POLE_R;
		}
		else
		{
			// - 1 dir
			pole = pole - 1;
			if (!pole)
				pole = POLE_B;
		}
		hex_xy2rki (globalX, globalY, &nr, &nk, &ni);
	}
	newp->bits |= SET_BITS (pole, WORLD_POLE_BITS, WORLD_POLE_SHIFT);
	newp->bits |= SET_BITS (nk, WORLD_CORNER_BITS, WORLD_CORNER_SHIFT);
	newp->r = nr;
	newp->t = ni;
	return newp;
}

worldPosition * wp_adjacent (const worldPosition pos, unsigned int poleRadius)
{
	struct world_position
		** r = xph_alloc (sizeof (struct world_position *) * 6);
	int
		i = 0;
	while (i < 6)
	{
		r[i] = wp_fromRelativeOffset (pos, poleRadius, 1, i, 0);
		i++;
	}
	return r;
}

worldPosition * wp_adjacentSweep (const worldPosition pos, unsigned int poleRadius, unsigned int radius)
{
	int
		o = 0,
		max = hx (radius) - 1;
	unsigned int
		r = 1,
		k = 0,
		i = 0;
	struct world_position
		** adj = xph_alloc (sizeof (struct world_position *) * max);
	//DEBUG ("%s: max is %d\n", __FUNCTION__, max);
	while (o < max)
	{
		adj[o] = wp_fromRelativeOffset (pos, poleRadius, r, k, i);
		o++;
		hex_nextValidCoord (&r, &k, &i);
	}
	return adj;
}

worldPosition wp_create (char pole, unsigned int r, unsigned int k, unsigned int i)
{
	struct world_position
		* wp = xph_alloc (sizeof (struct world_position));
	unsigned char
		pole_enum = (pole - 'a') + 1;
/*
	switch (pole)
	{
		case 'b':
			pole_enum = POLE_B;
			break;
		case 'g':
			pole_enum = POLE_G;
			break;
		case 'r':
			pole_enum = POLE_R;
			break;
		default:
			pole_enum = POLE_ERROR;
	}
*/
	if (pole_enum < POLE_R || pole_enum > POLE_B)
		pole_enum = POLE_ERROR;
	memset (&wp->bits, '\0', 1);
	wp->bits |= SET_BITS (pole_enum, WORLD_POLE_BITS, WORLD_POLE_SHIFT);
	wp->bits |= SET_BITS (k, WORLD_CORNER_BITS, WORLD_CORNER_SHIFT);
	wp->t = i;
	wp->r = r;
	return wp;
}

worldPosition wp_createEmpty ()
{
	struct world_position
		* wp = xph_alloc (sizeof (struct world_position));
	memset (wp, '\0', sizeof (struct world_position));
	return wp;
}

worldPosition wp_duplicate (const worldPosition wp)
{
	struct world_position
		* dup = xph_alloc (sizeof (struct world_position));
	memcpy (dup, wp, sizeof (struct world_position));
	return dup;
}

void wp_destroy (worldPosition wp)
{
	xph_free (wp);
}

void wp_destroyAdjacent (worldPosition * adj)
{
	unsigned char
		i = 0;
	while (i < 6)
	{
		wp_destroy (adj[i]);
		i++;
	}
	xph_free (adj);
}

/***
 * returns the distance in coordinate steps between the two worldPositions,
 * and additionally sets xp and yp to the cartesian coordinate offset from a
 * to b, if they're not NULL
 */
unsigned int wp_pos2xy (const worldPosition a, const worldPosition b, signed int * xp, signed int * yp)
{
	unsigned int
		r, k, i,
		distance[3];
	signed int
		ax, ay,
		bx, by,
		gx, gy,
		poleX, poleY,
		dx[3], dy[3];
	signed char
		pd = wp_getPole (b) - wp_getPole (a),
		dir[3],
		smallest;
	wp_getCoords (a, &r, &k, &i);
	hex_rki2xy (r, k, i, &ax, &ay);
	wp_getCoords (b, &r, &k, &i);
	hex_rki2xy (r, k, i, &bx, &by);
	if (pd == 0)
	{
		dx[0] = ax - bx;
		dy[0] = ay - by;
		if (xp)
			*xp = dx[0];
		if (yp)
			*yp = dy[0];
		return hex_coordinateMagnitude (dx[0], dy[0]);
	}
	if (pd == 2 || pd == -2)
		pd = pd * -1;

	if (pd < 0)
	{
		dir[0] = 0;
		dir[1] = 2;
		dir[2] = 4;
	}
	else
	{
		dir[0] = 1;
		dir[1] = 3;
		dir[2] = 5;
	}
	hexPole_centerDistanceCoord (dir[0], &poleX, &poleY);
	gx = bx + poleX;
	gy = by + poleY;
	dx[0] = ax - gx;
	dy[0] = ay - gy;
	distance[0] = hex_coordinateMagnitude (dx[0], dy[0]);

	hexPole_centerDistanceCoord (dir[1], &poleX, &poleY);
	gx = bx + poleX;
	gy = by + poleY;
	dx[1] = ax - gx;
	dy[1] = ay - gy;
	distance[1] = hex_coordinateMagnitude (dx[1], dy[1]);

	hexPole_centerDistanceCoord (dir[2], &poleX, &poleY);
	gx = bx + poleX;
	gy = by + poleY;
	dx[2] = ax - gx;
	dy[2] = ay - gy;
	distance[2] = hex_coordinateMagnitude (dx[2], dy[2]);

	smallest = distance[0] < distance[1]
		? 0
		: 1;
	smallest = distance[smallest] < distance[2]
		? smallest
		: 2;
	if (xp)
		*xp = dx[smallest];
	if (yp)
		*yp = dy[smallest];
	return distance[smallest];
}

unsigned int wp_distance (const worldPosition a, const worldPosition b, unsigned int poleRadius)
{
	unsigned int
		r, k, i,
		dist1, dist2, dist3,
		distance;
	signed int
		ax, ay,
		bx, by,
		gx, gy,
		poleX, poleY;
	signed char
		pd = wp_getPole (b) - wp_getPole (a);

	wp_getCoords (a, &r, &k, &i);
	hex_rki2xy (r, k, i, &ax, &ay);
	wp_getCoords (b, &r, &k, &i);
	hex_rki2xy (r, k, i, &bx, &by);
	if (pd == 0)
	{
		return hex_distanceBetween (ax, ay, bx, by);
	}
	if (pd == 2 || pd == -2)
		pd = pd * -1;
	//printf ("transfer \'%c\' to \'%c\': %d. (%d %d %d)\n", wp_getPole (a), wp_getPole (b), pd, pd < 0 ? 0 : 1, pd < 0 ? 2 : 3, pd < 0 ? 4 : 5);

	if (pd < 0)
	{
		hexGround_centerDistanceCoord (poleRadius, 0, &poleX, &poleY);
		gx = bx + poleX;
		gy = by + poleY;
		dist1 = hex_distanceBetween (ax, ay, gx, gy);

		hexGround_centerDistanceCoord (poleRadius, 2, &poleX, &poleY);
		gx = bx + poleX;
		gy = by + poleY;
		dist2 = hex_distanceBetween (ax, ay, gx, gy);

		hexGround_centerDistanceCoord (poleRadius, 4, &poleX, &poleY);
		gx = bx + poleX;
		gy = by + poleY;
		dist3 = hex_distanceBetween (ax, ay, gx, gy);
	}
	else
	{
		hexGround_centerDistanceCoord (poleRadius, 1, &poleX, &poleY);
		gx = bx + poleX;
		gy = by + poleY;
		dist1 = hex_distanceBetween (ax, ay, gx, gy);

		hexGround_centerDistanceCoord (poleRadius, 3, &poleX, &poleY);
		gx = bx + poleX;
		gy = by + poleY;
		dist2 = hex_distanceBetween (ax, ay, gx, gy);

		hexGround_centerDistanceCoord (poleRadius, 5, &poleX, &poleY);
		gx = bx + poleX;
		gy = by + poleY;
		dist3 = hex_distanceBetween (ax, ay, gx, gy);
	}
	//printf ("distances: %d %d %d\n", dist1, dist2, dist3);
	distance = dist1 < dist2
		? dist1
		: dist2;
	distance = dist3 < distance
		? dist3
		: distance;
	return distance;
}


int wp_compare (const worldPosition a, const worldPosition b)
{
	signed char
		pd = GET_BITS (a->bits, WORLD_POLE_BITS, WORLD_POLE_SHIFT) - GET_BITS (b->bits, WORLD_POLE_BITS, WORLD_POLE_SHIFT),
		kd = GET_BITS (a->bits, WORLD_CORNER_BITS, WORLD_CORNER_SHIFT) - GET_BITS (b->bits, WORLD_CORNER_BITS, WORLD_CORNER_SHIFT);
	if (pd != 0)
		return pd;
	if (a->r - b->r != 0)
		return a->r - b->r;
	if (kd != 0)
		return kd;
	return a->t - b->t;
}

char WpPrint[32];
const char * wp_print (const worldPosition pos)
{
	static unsigned char
		t = 0;
	int
		corner = GET_BITS (pos->bits, WORLD_CORNER_BITS, WORLD_CORNER_SHIFT);
	unsigned char
		polestr = wp_getPole (pos);
	switch (polestr)
	{
		case 'a':
			polestr = 'r';
			break;
		case 'b':
			polestr = 'g';
			break;
		case 'c':
			polestr = 'b';
			break;
		default:
			polestr = '?';
			break;
	}
	t ^= 1;
	snprintf (WpPrint+t*16, 16, "[%c:%d %d %d]", polestr - 32, pos->r, corner, pos->t);
	return WpPrint+t*16;
}


// FIXME: this returns [abc] instead of [rgb]; to fix it would involve changing code in wp_pos2xy and wp_distance.
unsigned char wp_getPole (const worldPosition pos)
{
	unsigned char
		r = (GET_BITS (pos->bits, WORLD_POLE_BITS, WORLD_POLE_SHIFT) + 'a') - POLE_R;
	if (r < 'a' || r > 'c')
		r = '?';
	return r;
}

bool wp_getCoords (const worldPosition pos, unsigned int * rp, unsigned int * kp, unsigned int * ip)
{
	if (rp == NULL || kp == NULL || ip == NULL)
		return FALSE;
	*rp = pos->r;
	*kp = GET_BITS (pos->bits, WORLD_CORNER_BITS, WORLD_CORNER_SHIFT);
	*ip = pos->t;
	return TRUE;
}

/***
 * WORLDHEX
 */

WORLDHEX worldhexFromPosition (const worldPosition wp, unsigned int r, unsigned int k, unsigned int i)
{
	WORLDHEX
		whx = xph_alloc (sizeof (struct worldHex));
	hex_rki2xy (r, k, i, &whx->gx, &whx->gy);
	memcpy (&whx->wp, wp, sizeof (struct world_position));
	return whx;
}

WORLDHEX worldhex (unsigned char pole, unsigned int pr, unsigned int pk, unsigned int pi, unsigned int gr, unsigned int gk, unsigned int gi)
{
	WORLDHEX
		whx = xph_alloc (sizeof (struct worldHex));
	worldPosition
		wp = NULL;
	switch (pole)
	{
		case 'r':
			pole = 'a';
			break;
		case 'g':
			pole = 'b';
			break;
		case 'b':
			pole = 'c';
			break;
		default:
			ERROR ("Invalid pole for worldhex; expecting one of ['r', 'g', 'b'] but got '%c' instead.", pole);
			return NULL;
	}
	wp = wp_create (pole, pr, pk, pi);
	hex_rki2xy (gr, gk, gi, &whx->gx, &whx->gy);
	memcpy (&whx->wp, wp, sizeof (struct world_position));
	wp_destroy (wp);
	return whx;
}

WORLDHEX worldhexDuplicate (const WORLDHEX whx)
{
	WORLDHEX
		new = xph_alloc (sizeof (struct worldHex));
	memcpy (new, whx, sizeof (struct worldHex));
	return new;
}

void worldhexDestroy (WORLDHEX whx)
{
	xph_free (whx);
}

const worldPosition worldhexAsWorldPosition (const WORLDHEX whx)
{
	return &whx->wp;
}

bool worldhexGroundOffset (const WORLDHEX whx, signed int * x, signed int * y)
{
	if (x == NULL || y == NULL)
		return FALSE;
	*x = whx->gx;
	*y = whx->gy;
	return TRUE;
}

static char WhxPrint[64];
const char * whxPrint (const WORLDHEX whx)
{
	static unsigned char
		t = 0;
	int
		corner = GET_BITS (whx->wp.bits, WORLD_CORNER_BITS, WORLD_CORNER_SHIFT);
	unsigned char
		polestr = wp_getPole (&whx->wp);
	switch (polestr)
	{
		case 'a':
			polestr = 'r';
			break;
		case 'b':
			polestr = 'g';
			break;
		case 'c':
			polestr = 'b';
			break;
		default:
			polestr = '?';
			break;
	}

	t ^= 1;
	snprintf (WhxPrint+t*32, 32, "[%c:%d %d %d, %d %d]", polestr - 32, whx->wp.r, corner, whx->wp.t, whx->gx, whx->gy);
	
	return WhxPrint+t*32;
}

signed char whxPoleDifference (const WORLDHEX a, const WORLDHEX b)
{
	signed char
		r = GET_BITS (a->wp.bits, WORLD_POLE_BITS, WORLD_POLE_SHIFT) - GET_BITS (b->wp.bits, WORLD_POLE_BITS, WORLD_POLE_SHIFT);
	if (r == 2)
		r = -1;
	else if (r == -2)
		r = 1;
	return r;
}

VECTOR3 whxDistanceVector (const WORLDHEX a, const WORLDHEX b, unsigned char dir)
{
	signed int
		bPoleXOffset = 0, bPoleYOffset = 0,
		ax, ay,
		bx, by;
	unsigned int
		ar, ak, ai,
		br, bk, bi;
	VECTOR3
		r;
	
	wp_getCoords (&a->wp, &ar, &ak, &ai);
	hex_rki2xy (ar, ak, ai, &ax, &ay);
	if (dir < 6)
		hexPole_centerDistanceCoord (dir, &bPoleXOffset, &bPoleYOffset);
	wp_getCoords (&b->wp, &br, &bk, &bi);
	hex_rki2xy (br, bk, bi, &bx, &by);
	DEBUG ("a: %d,%d; b: %d,%d + (pole: %d,%d)", ax, ay, bx, by, bPoleXOffset, bPoleYOffset);
	bx += bPoleXOffset;
	by += bPoleYOffset;
	r = hex_xyCoord2Space (bx - ax, by - ay);
	DEBUG ("final spaced value: %.2f, %.2f, %.2f", r.x, r.y, r.z);
	return r;
}
