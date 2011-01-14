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
	// todo: since t's max value is always r * 6 - 1, there's much more of a danger of t overflowing than of r overflowing. however, this issue can be resolved by using bitfields: two bytes are required to store the pole information, and three bytes are required to store a 'k' value for the polar coordinate, where the full phi value is k * 6 + t instead of just t. this constrains t to strictly < r in all cases and doesn't require adding any additional data to the position struct.
};

#define WORLD_POLE_BITS		2
#define WORLD_POLE_SHIFT	0
#define WORLD_CORNER_BITS	3
#define WORLD_CORNER_SHIFT	3
#define BIT_MASK(x)				((1 << x) - 1)
#define SET_BITS(bits,field,shift)	((bits & BIT_MASK(field)) << shift)
#define GET_BITS(bits,field,shift)	((bits & (BIT_MASK(field) << shift)) >> shift)

enum world_poles
{
	POLE_ERROR	= 0x00,
	POLE_A		= 0x01,
	POLE_B		= 0x02,
	POLE_C		= 0x03,
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
				pole = POLE_A;
		}
		else
		{
			// - 1 dir
			pole = pole - 1;
			if (!pole)
				pole = POLE_C;
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

worldPosition wp_create (char pole, unsigned int r, unsigned int k, unsigned int i)
{
	struct world_position
		* wp = xph_alloc (sizeof (struct world_position));
	unsigned char
		pole_enum = (pole - 'a') + 1;
	if (pole_enum < POLE_A || pole_enum > POLE_C)
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

unsigned int wp_distance (const worldPosition a, const worldPosition b)
{
	unsigned int
		r, k, i;
	signed int
		ax, ay,
		bx, by,
		x, y;
	// TODO: translate the coordinates so that they both have representations around the same pole. otherwise this next step will produce entirely wrong distance values at pole edges.
	wp_getCoords (a, &r, &k, &i);
	hex_rki2xy (r, k, i, &ax, &ay);
	wp_getCoords (b, &r, &k, &i);
	hex_rki2xy (r, k, i, &bx, &by);
	x = ax - bx;
	y = ay - by;
	if ((x ^ y) < 0)
		return abs (x) > abs (y)
			? abs (x)
			: abs (y);
	else
		return abs (x + y);
}


// TODO: there is probably an overflow bug here
// ADDITIONAL TODO: what the fuck is this? to start with practical concerns, the shift doesn't work right and will overflow, and also it limits pole radius to a maximum of 4096 (or maybe 2048?), after which everything will break.
int wp_compare (const worldPosition a, const worldPosition b)
{
	unsigned int
		aa = 0, bb = 0,
		shift;
	shift = sizeof (int) * CHAR_BIT - 1;

/*
	printf ("got %p:\n", a);
	wp_print (a);
	printf ("got %p:\n", b);
	wp_print (b);
*/
	
	aa |= GET_BITS (a->bits, WORLD_POLE_BITS, WORLD_POLE_SHIFT) << (shift - (WORLD_POLE_BITS - 1));
	shift -= WORLD_POLE_BITS;
	aa |= GET_BITS (a->bits, WORLD_CORNER_BITS, WORLD_CORNER_SHIFT) << (shift - (WORLD_CORNER_BITS - 1));
	shift -= WORLD_CORNER_BITS;
	aa |= GET_BITS (a->r, 12, 0) << (shift - 11);
	shift -= 12;
	aa |= GET_BITS (a->t, 12, 0) << (shift - 11);
	shift -= 12;
	if (shift < 0)
		assert (0 && "ints not long enough :(");
	shift = sizeof (int) * CHAR_BIT - 1;
	bb |= GET_BITS (b->bits, WORLD_POLE_BITS, WORLD_POLE_SHIFT) << (shift - (WORLD_POLE_BITS - 1));
	shift -= WORLD_POLE_BITS;
	bb |= GET_BITS (b->bits, WORLD_CORNER_BITS, WORLD_CORNER_SHIFT) << (shift - (WORLD_CORNER_BITS - 1));
	shift -= WORLD_CORNER_BITS;
	bb |= GET_BITS (b->r, 12, 0) << (shift - 11);
	shift -= 12;
	bb |= GET_BITS (b->t, 12, 0) << (shift - 11);
	shift -= 12;
	//printf ("a hash: %d\nb hash: %d\n", aa, bb);
	return aa - bb;
}

void wp_print (const worldPosition pos)
{
	int
		corner = GET_BITS (pos->bits, WORLD_CORNER_BITS, WORLD_CORNER_SHIFT);
	unsigned char
		polestr = wp_getPole (pos);
	printf ("POSITION\n\tpole %c\n\t{%d %d %d} (%d)\n\n", polestr, pos->r, corner, pos->t, corner * pos->t);
}

unsigned char wp_getPole (const worldPosition pos)
{
	unsigned char
		r = (GET_BITS (pos->bits, WORLD_POLE_BITS, WORLD_POLE_SHIFT) + 'a') - POLE_A;
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
