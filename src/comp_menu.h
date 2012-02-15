/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#ifndef XPH_COMP_MENU_H
#define XPH_COMP_MENU_H

#include "entity.h"

#include "component_input.h"
#include "comp_clickable.h"

void menu_define (EntComponent comp, EntSpeech speech);

void menu_addItem (Entity this, const char * text, enum input_responses action, actionCallback callback);

void menu_addPositionedItem (Entity this, int x, int y, int w, int h, const char * text, enum input_responses action, actionCallback callback);

#endif /* XPH_COMP_MENU_H */