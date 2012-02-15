/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#ifndef XPH_COMP_WORLDMAP_H
#define XPH_COMP_WORLDMAP_H

#include "entity.h"
#include "texture.h"

struct xph_worldmap_data
{
	unsigned char
		spanFocus,
		worldSpan;
	int
		typeFocus;
	Dynarr
		types;
	enum spantypefocus
	{
		FOCUS_SPAN,
		FOCUS_TYPE
	} spanTypeFocus;
	TEXTURE
		* spanTextures;
};
typedef struct xph_worldmap_data * worldmapData;

void worldmap_define (EntComponent comp, EntSpeech speech);

#endif /* XPH_COMP_WORLDMAP_H */