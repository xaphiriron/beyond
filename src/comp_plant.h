/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#ifndef XPH_COMP_PLANT_H
#define XPH_COMP_PLANT_H

#include "entity.h"

#include "lsystem.h"
#include "turtle3d.h"
#include "shapes.h"

struct xph_plant
{
	LSYSTEM
		* expansion;
	SymbolSet
		render;
	char
		* body;
	short
		lastUpdated,
		updateFrequency;

	SHAPE
		fauxShape;
};
typedef struct xph_plant * Plant;

void plant_define (EntComponent comp, EntSpeech speech);

void plantUpdate_system (Dynarr entities);
void plantRender_system (Dynarr entities);

#endif /* XPH_COMP_PLANT_H */