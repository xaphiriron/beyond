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