/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#ifndef XPH_TEXTURE_INTERNAL_H
#define XPH_TEXTURE_INTERNAL_H

#include <GL/gl.h>
#include <SDL/SDL_image.h>

struct xph_texture
{
	signed short
		width,
		height;
	signed char
		mode;

	GLuint
		name;
	SDL_Surface
		* surface;

	unsigned char
		* data;
};

#endif /* XPH_TEXTURE_INTERNAL_H */