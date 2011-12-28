#include "matspec.h"

#include "xph_memory.h"
#include "dynarr.h"

enum matOpacity
{
	MAT_OPAQUE,
	MAT_TRANSPARENT,
};

struct material_specification
{
	unsigned int
		id;
	unsigned char
		color[4];
	enum matOpacity
		opacity;
};

Dynarr
	materialList = NULL;

void materialsGenerate ()
{
	MATSPEC
		mat;
	if (materialList != NULL)
		return;
	materialList = dynarr_create (8, sizeof (MATSPEC));

	mat = xph_alloc (sizeof (struct material_specification));
	mat->id = MAT_AIR;
	mat->opacity = MAT_TRANSPARENT;
	mat->color[0] = 0xff;
	mat->color[1] = 0xff;
	mat->color[2] = 0xff;
	mat->color[3] = 0x00;
	dynarr_assign (materialList, MAT_AIR, mat);

	mat = xph_alloc (sizeof (struct material_specification));
	mat->id = MAT_STONE;
	mat->opacity = MAT_OPAQUE;
	mat->color[0] = 0xbf;
	mat->color[1] = 0x9f;
	mat->color[2] = 0xcf;
	mat->color[3] = 0xff;
	dynarr_assign (materialList, MAT_STONE, mat);

	mat = xph_alloc (sizeof (struct material_specification));
	mat->id = MAT_DIRT;
	mat->opacity = MAT_OPAQUE;
	mat->color[0] = 0xdf;
	mat->color[1] = 0xcf;
	mat->color[2] = 0xbf;
	mat->color[3] = 0xff;
	dynarr_assign (materialList, MAT_DIRT, mat);

	mat = xph_alloc (sizeof (struct material_specification));
	mat->id = MAT_GRASS;
	mat->opacity = MAT_OPAQUE;
	mat->color[0] = 0xbf;
	mat->color[1] = 0xdf;
	mat->color[2] = 0xbf;
	mat->color[3] = 0xff;
	dynarr_assign (materialList, MAT_GRASS, mat);

}

MATSPEC material (enum materials mat)
{
	if (!materialList)
		return NULL;
	return *(MATSPEC *)dynarr_at (materialList, mat);
}



void matspecColor (const MATSPEC mat, unsigned char * r, unsigned char * g, unsigned char * b, unsigned char * a)
{
	if (mat == NULL)
	{
		if (r != NULL)
			*r = 0xff;
		if (g != NULL)
			*g = 0x00;
		if (b != NULL)
			*b = 0xff;
		if (a != NULL)
			*a = 0xff;
		return;
	}
	if (r != NULL)
		*r = mat->color[0];
	if (g != NULL)
		*g = mat->color[1];
	if (b != NULL)
		*b = mat->color[2];
	if (a != NULL)
		*a = mat->color[3];
}

unsigned char materialParam (const MATSPEC mat, const char * param)
{
	if (mat == NULL)
	{
		if (strcmp (param, "transparent") == 0)
			return true;
		return false;
	}
	if (strcmp (param, "opaque") == 0)
		return mat->opacity == MAT_OPAQUE;
	else if (strcmp (param, "visible") == 0)
		return mat->opacity != MAT_TRANSPARENT;
	return 0;
}

bool matspecIsOpaque (const MATSPEC mat)
{
	if (mat == NULL)
		return false;
	return mat->opacity == MAT_OPAQUE;
}
