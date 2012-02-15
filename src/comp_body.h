/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#ifndef XPH_COMPONENT_BODY_H
#define XPH_COMPONENT_BODY_H

#include "entity.h"

struct xph_body
{
	float
		height;
};
typedef struct xph_body * Body;

void body_define (EntComponent comp, EntSpeech speech);

void bodyRender_system (Dynarr entities);


float body_height (Entity e);

#endif /* XPH_COMPONENT_BODY_H */