#ifndef XPH_HEX_H
#define XPH_HEX_H

#include "xph_memory.h"
#include "vector.h"
#include "hex_utility.h"

typedef struct hex * Hex;

struct hex {
	struct hex
		* neighbors[6];
	VECTOR3
		topNormal,
		baseNormal;
	float
		top, topA, topB,
		base, baseA, baseB,
		edgeDepth[12];
	unsigned int
		r, k, i;	// polar coordinates from center of ground
	signed int
		x, y;		// cartesian coordinates from center of ground
};

enum hex_sides {
  HEX_TOP = 1,
  HEX_BASE
};

struct hex * hex_create (unsigned int r, unsigned int k, unsigned int i, float height);
void hex_destroy (struct hex * h);

void hex_setSlope (struct hex * h, enum hex_sides side, float a, float b, float c);
void hex_bakeEdges (struct hex * h);

#endif /* XPH_HEX_H */