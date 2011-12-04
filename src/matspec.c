#include "matspec.h"

#include "xph_memory.h"

struct material_specification
{
	unsigned int
		id;
	unsigned char
		color[4];
	enum matOpacity
		opacity;
};

static unsigned int
	matID = 0;
MATSPEC makeMaterial (enum matOpacity opacity)
{
	MATSPEC
		spec = xph_alloc (sizeof (struct material_specification));
	spec->id = ++matID;
	spec->opacity = opacity;
	switch (opacity)
	{
		case MAT_OPAQUE:
			spec->color[0] = 0xcf;
			spec->color[1] = 0xcf;
			spec->color[2] = 0xcf;
			spec->color[3] = 0xff;
			break;
		case MAT_TRANSPARENT:
		default:
			spec->color[0] = 0xff;
			spec->color[1] = 0xff;
			spec->color[2] = 0xff;
			spec->color[3] = 0x00;
			break;
	}
	return spec;
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
