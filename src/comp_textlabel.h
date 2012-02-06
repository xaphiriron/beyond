#ifndef XPH_COMP_TEXTLABEL_H
#define XPH_COMP_TEXTLABEL_H

#include "entity.h"
#include "font.h"

struct xph_textlabel
{
	Text
		text;
};

void textlabel_define (EntComponent comp, EntSpeech speech);

void textlabel_set (Entity this, const char * text, enum textAlignType align, int x, int y, int w);
void textlabel_draw (Entity this);

#endif /* XPH_COMP_TEXTLABEL_H */