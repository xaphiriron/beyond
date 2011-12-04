#ifndef XPH_MATSPEC_H
#define XPH_MATSPEC_H

#include <stdbool.h>

typedef struct material_specification * MATSPEC;
enum matOpacity
{
	MAT_OPAQUE,
	MAT_TRANSPARENT,
};

MATSPEC makeMaterial (enum matOpacity opacity);

void matspecColor (const MATSPEC mat, unsigned char * r, unsigned char * g, unsigned char * b, unsigned char * a);

unsigned char materialParam (const MATSPEC mat, const char * param);

#endif /* XPH_MATSPEC_H */