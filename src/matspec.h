#ifndef XPH_MATSPEC_H
#define XPH_MATSPEC_H

#include <stdbool.h>
enum materials
{
	MAT_AIR,
	MAT_STONE,
	MAT_DIRT,
	MAT_GRASS,
};

typedef struct material_specification * MATSPEC;

void materialsGenerate ();

MATSPEC material (enum materials);

void matspecColor (const MATSPEC mat, unsigned char * r, unsigned char * g, unsigned char * b, unsigned char * a);

unsigned char materialParam (const MATSPEC mat, const char * param);
#define matParam(x, y)	materialParam(x, y)

#endif /* XPH_MATSPEC_H */