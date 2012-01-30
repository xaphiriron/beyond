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
	unsigned int
		stack;
	void (*confirmCallback)(Entity);
	void (*cancelCallback)(Entity);
};
typedef struct xph_gui * GUI;

void gui_place (Entity e, int x, int y, int w, int h);
void gui_setMargin (Entity e, int vert, int horiz);

void gui_define (EntComponent comp, EntSpeech speech);

void gui_update (Dynarr entities);
void gui_render (Dynarr entities);

void gui_setStack (Entity this, unsigned int stack);

void gui_confirmCallback (Entity this, void (*callback)(Entity));
void gui_cancelCallback (Entity this, void (*callback)(Entity));
void gui_defaultCallback (Entity gui);

bool gui_xy (Entity e, int * x, int * y);
bool gui_wh (Entity e, int * w, int * h);
bool gui_vhMargin (Entity e, int * v, int * h);

// gui doesn't use this itself but it's an easy way to draw the "canonical" gui size/location
void gui_drawPane (Entity e);

#endif /* XPH_COMP_GUI_H */