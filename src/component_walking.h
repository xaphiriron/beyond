/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#ifndef XPH_COMPONENT_WALKING_H
#define XPH_COMPONENT_WALKING_H

#include <math.h>

#include "vector.h"

#include "entity.h"

#include "component_position.h"
#include "component_input.h"

#include "system.h"

void walking_start (Entity e);
void walking_stop (Entity e);

void walking_define (EntComponent comp, EntSpeech speech);

void walking_system (Dynarr entities);

#endif