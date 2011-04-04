#include "hex.h"

HEX hex_create (unsigned int r, unsigned int k, unsigned int i, float height) {
	HEX
		h = xph_alloc (sizeof (struct hex));
  if (r < 0 || (r == 0 && (k != 0 || i != 0))) {
	WARNING ("%s (%d, %d, %d): invalid r-val\n", __FUNCTION__, r, k, i);
  }
  if (k < 0 || k >= 6) {
	WARNING ("%s (%d, %d, %d): invalid k-val\n", __FUNCTION__, r, k, i);
  }
  if (i >= r && i != 0) {
	WARNING ("%s (%d, %d, %d): invalid i-val\n", __FUNCTION__, r, k, i);
  }
  h->r = r;
  h->k = k;
  h->i = i;
  hex_rki2xy (r, k, i, &h->x, &h->y);
	memset (h->corners, '\0', 3);
	memset (h->edgeBase, '\0', sizeof (int) * 12);

  return h;
}

void hex_destroy (struct hex * h) {
  xph_free (h);
}


void hexSetHeight (HEX hex, unsigned short height)
{
	hex->centre = height;
}

void hexSetCorners (HEX hex, short a, short b, short c, short d, short e, short f)
{
	hex->corners[0] =
		(a < 0 ? ((~a & 15) + 1) | 8 : a & 7) << 4 |
		(b < 0 ? ((~b & 15) + 1) | 8 : b & 7);
	hex->corners[1] =
		(c < 0 ? ((~c & 15) + 1) | 8 : c & 7) << 4 |
		(d < 0 ? ((~d & 15) + 1) | 8 : d & 7);
	hex->corners[2] =
		(e < 0 ? ((~e & 15) + 1) | 8 : e & 7) << 4 |
		(f < 0 ? ((~f & 15) + 1) | 8 : f & 7);
}

void hexPullCorner (HEX hex, short corner)
{
	signed char
		loop = 0,
		height,
		adj1, adj2,
		change;
	if (corner < 0 || corner > 5)
		return;
	while (loop < 6)
	{
		height = GETCORNER (hex->corners, corner);
		if (height == 7)
		{
			// lol an underflow bug per line. which is funny cause the entire point of this if is to avoid overflow.
			hex->centre--;
			SETCORNER (hex->corners, 0, GETCORNER (hex->corners, 0) - 1);
			SETCORNER (hex->corners, 1, GETCORNER (hex->corners, 1) - 1);
			SETCORNER (hex->corners, 2, GETCORNER (hex->corners, 2) - 1);
			SETCORNER (hex->corners, 3, GETCORNER (hex->corners, 3) - 1);
			SETCORNER (hex->corners, 4, GETCORNER (hex->corners, 4) - 1);
			SETCORNER (hex->corners, 5, GETCORNER (hex->corners, 5) - 1);
			height--;
		}
		if (height - hex->centre > 2)
		{
			change = (height + 1) / 2;
			hex->centre += change;
			SETCORNER (hex->corners, 0, GETCORNER (hex->corners, 0) - change);
			SETCORNER (hex->corners, 1, GETCORNER (hex->corners, 1) - change);
			SETCORNER (hex->corners, 2, GETCORNER (hex->corners, 2) - change);
			SETCORNER (hex->corners, 3, GETCORNER (hex->corners, 3) - change);
			SETCORNER (hex->corners, 4, GETCORNER (hex->corners, 4) - change);
			SETCORNER (hex->corners, 5, GETCORNER (hex->corners, 5) - change);
			height -= change;
		}
		adj1 = GETCORNER (hex->corners, (corner + 1) % 6);
		adj2 = GETCORNER (hex->corners, (corner + 5) % 6);
		SETCORNER (hex->corners, corner, height + 1);
		height++;
		if (height - adj1 > 2)
		{
			adj1++;
			SETCORNER (hex->corners, (corner + 1) % 6, adj1);
		}
		if (height - adj2 > 2)
		{
			adj2++;
			SETCORNER (hex->corners, (corner + 5) % 6, adj2);
		}
		corner = adj1 > adj2 ? (corner + 5) % 6 : (corner + 1) % 6;
		loop++;
	}
}

void hexSetCornersRandom (HEX hex)
{
	int
		i = rand ();
	if (i & 1)
		hexPullCorner (hex, 0);
	if (i & 2)
		hexPullCorner (hex, 1);
	if (i & 4)
		hexPullCorner (hex, 2);
	if (i & 8)
		hexPullCorner (hex, 3);
	if (i & 16)
		hexPullCorner (hex, 4);
	if (i & 32)
		hexPullCorner (hex, 5);
}

signed char hexGetCornerHeight (const HEX hex, short corner)
{
	return GETCORNER (hex->corners, corner);
}

/*
void hex_bakeEdges (struct hex * h)
{
	struct hex
		* adj;
	int
		i = 0;
	if (h == NULL)
		return;
	while (i < 6)
	{
		adj = h->neighbors[(i + 1) % 6];
		//printf ("%s: %d-th edge, hex %p\n", __FUNCTION__, i, adj);
		if (adj == NULL)
		{
			i++;
			continue;
		}
		h->edgeDepth[i * 2] = hex_getCornerHeight (adj, (i + 4) % 6);
		h->edgeDepth[i * 2 + 1] = hex_getCornerHeight (adj, (i + 3) % 6);
		i++;
	}
}
*/

/*
void hex_bakeEdge (struct hex * h, int dir, struct hex * adj)
{
	int
		valA, valB;
	if (h == NULL || adj == NULL || dir < 0 || dir > 5)
		return;
	switch (dir)
	{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		default:
	}

	h->edgeDepth[dir * 2] =
	h->edgeDepth[dir * 2 + 1] =

}
*/
