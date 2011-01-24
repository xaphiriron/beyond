#include "hex.h"

struct hex * hex_create (unsigned int r, unsigned int k, unsigned int i, float height) {
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

float hex_getCornerHeight (const struct hex * h, int corner)
{
	if (h == NULL || corner < 0 || corner >= 6)
		return 0.0;
	switch (corner)
	{
		case 0:
			return h->topA;
		case 1:
			return h->topB;
		case 2:
			return h->top + (h->top - h->topA) - (h->top - h->topB);
		case 3:
			return h->top + (h->top - h->topA);
		case 4:
			return h->top + (h->top - h->topB);
		case 5:
		default:
			return h->top + (h->top - h->topB) - (h->top - h->topA);
	}
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
