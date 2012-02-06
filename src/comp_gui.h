#ifndef XPH_COMP_GUI_H
#define XPH_COMP_GUI_H

#include "entity.h"

typedef struct xph_gui * GUI;

struct xph_gui
{
	int
		x, y,
		w, h,
		vertMargin,
		horizMargin;
	unsigned int
		stack;
	Entity
		frame;
	Dynarr
		subs;
};

void gui_place (Entity e, int x, int y, int w, int h);
void gui_setMargin (Entity e, int vert, int horiz);

void gui_setFrame (Entity this, Entity frame);

void gui_define (EntComponent comp, EntSpeech speech);

void gui_update (Dynarr entities);
void gui_render (Dynarr entities);

void gui_placeOnStack (Entity this);

bool gui_inside (Entity this, int x, int y);

bool gui_xy (Entity e, int * x, int * y);
bool gui_wh (Entity e, int * w, int * h);
bool gui_vhMargin (Entity e, int * v, int * h);

Entity gui_getFrame (Entity this);
Dynarr gui_getSubs (Entity this);

// gui doesn't use these itself but it's an easy way to draw the "canonical" gui size/location
void gui_drawPane (Entity e);

void gui_drawPaneCoords (int left, int right, int top, int bottom);

#endif /* XPH_COMP_GUI_H */