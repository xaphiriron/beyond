#ifndef XPH_COMPONENT_UI_H
#define XPH_COMPONENT_UI_H

#include "entity.h"

typedef union uiPanels * UIPANEL;

enum uiPanelTypes
{
	UI_NONE = 0x00,
	UI_STATICTEXT,

	UI_DEBUG_OVERLAY,
	UI_WORLDMAP,
};


void uiDrawCursor ();

void ui_classInit (EntComponent ui, void * arg);

void ui_create (EntComponent ui, void * arg);
void ui_destroy (EntComponent ui, void * arg);

void ui_setType (EntComponent ui, void * arg);
void ui_getType (EntComponent ui, void * arg);

void ui_draw (EntComponent ui, void * arg);

#endif /* XPH_COMPONENT_UI_H */