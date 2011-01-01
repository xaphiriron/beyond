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
		//printf ("%s: %d-th edge, hex %p\n", __FUNCTION__, i, adj);
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
