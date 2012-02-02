#ifndef XPH_COMP_GUI_H
#define XPH_COMP_GUI_H

#include "entity.h"

typedef struct xph_gui * GUI;
typedef struct xph_gui_target * GUITarget;
typedef void (* targetCallback)(Entity, GUITarget);

struct xph_gui
{
	int
		x, y,
		w, h,
		vertMargin,
		horizMargin;
	unsigned int
		stack;
	Dynarr
		targets;
	GUITarget
		lastHover;
	void (*confirmCallback)(Entity);
	void (*cancelCallback)(Entity);
};
struct xph_gui_target
{
	int
		x, y,
		w, h;
	bool
		hasHover;
	targetCallback
		hover,
		click;
};

void gui_place (Entity e, int x, int y, int w, int h);
void gui_setMargin (Entity e, int vert, int horiz);

void gui_define (EntComponent comp, EntSpeech speech);

void gui_update (Dynarr entities);
void gui_render (Dynarr entities);

void gui_placeOnStack (Entity this);

GUITarget gui_addTarget (Entity this, int x, int y, int w, int h, targetCallback hover, targetCallback click);
void gui_rmTarget (Entity this, GUITarget target);
GUITarget gui_hit (Entity this, int x, int y);

void gui_confirmCallback (Entity this, void (*callback)(Entity));
void gui_cancelCallback (Entity this, void (*callback)(Entity));

void gui_defaultCallback (Entity gui);
void gui_defaultHover (Entity this, GUITarget hit);

bool gui_xy (Entity e, int * x, int * y);
bool gui_wh (Entity e, int * w, int * h);
bool gui_vhMargin (Entity e, int * v, int * h);

// gui doesn't use these itself but it's an easy way to draw the "canonical" gui size/location
void gui_drawPane (Entity e);
void gui_drawTargetPane (GUITarget target);

void gui_drawPaneCoords (int left, int right, int top, int bottom);

#endif /* XPH_COMP_GUI_H */