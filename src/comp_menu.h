#ifndef XPH_COMP_MENU_H
#define XPH_COMP_MENU_H

#include "entity.h"

#include "component_input.h"

void menu_define (EntComponent comp, EntSpeech speech);

void menu_addItem (Entity this, const char * text, enum input_responses action);

#endif /* XPH_COMP_MENU_H */