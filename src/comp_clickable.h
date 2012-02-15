/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#ifndef XPH_COMP_CLICKABLE_H
#define XPH_COMP_CLICKABLE_H

#include "entity.h"
#include "component_input.h"

typedef void (* actionCallback)(Entity);
typedef bool (* insideCheck)(Entity, int, int);

struct xph_clickable
{
	bool
		hasHover,
		hasClick;
	actionCallback
		hover,
		click;
	enum input_responses
		inputResponse;
	insideCheck
		inside;
};

typedef struct xph_clickable * Clickable;

void clickable_define (EntComponent comp, EntSpeech speech);

void clickable_setHoverCallback (Entity this, actionCallback hover);
void clickable_setClickCallback (Entity this, actionCallback click);
void clickable_setClickInputResponse (Entity this, enum input_responses response);

bool clickable_hasHover (Entity this);
bool clickable_hasClick (Entity this);

void clickable_draw (Entity this);

#endif /* XPH_COMP_CLICKABLE_H */