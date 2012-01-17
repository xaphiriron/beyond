#ifndef XPH_COMP_GUI_H
#define XPH_COMP_GUI_H

#include "entity.h"

struct xph_gui
{
	int
		x, y,
		w, h,
		vertMargin,
		horizMargin;
};
typedef struct xph_gui * GUI;

void gui_place (Entity e, int x, int y, int w, int h);
void gui_setMargin (Entity e, int vert, int horiz);

void gui_define (EntComponent comp, EntSpeech speech);

void gui_update (Dynarr entities);
void gui_render (Dynarr entities);

#endif /* XPH_COMP_GUI_H */