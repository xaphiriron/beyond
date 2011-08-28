#ifndef XPH_TEXTURE_INTERNAL_H
#define XPH_TEXTURE_INTERNAL_H

#include <GL/gl.h>

struct xph_texture
{
	signed short
		width,
		height;
	signed char
		mode;

	GLuint
		name;

	unsigned char
		* data;
};

#endif /* XPH_TEXTURE_INTERNAL_H */