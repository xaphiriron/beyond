#ifndef XPH_COMPONENT_UI_H
#define XPH_COMPONENT_UI_H

#include "entity.h"

typedef union uiPanels * UIPANEL;

enum uiPanelTypes
{
	UI_NONE = 0,
	UI_STATICTEXT,

	UI_DEBUG_OVERLAY,
	UI_WORLDMAP,

	UI_MENU,
};

enum uiFramePos
{
	PANEL_X_FREE		= 0x01,	/* x corner specified*/
	PANEL_X_ALIGN_14	= 0x02, /* left edge aligned to 1/4 screen width */
	PANEL_X_ALIGN_12	= 0x03, /* left edge aligned to 1/2 screen width */
	PANEL_X_ALIGN_34	= 0x04, /* left edge aligned to 3/4 screen width */
	PANEL_X_CENTER		= 0x05, /* left edge aligned to 1/2 screen width - panel width / 2 */
	PANEL_X_MASK		= 0x0f,

	PANEL_Y_FREE		= 0x10,
	PANEL_Y_ALIGN_13	= 0x20,
	PANEL_Y_ALIGN_12	= 0x30,
	PANEL_Y_ALIGN_23	= 0x40,
	PANEL_Y_CENTER		= 0x50, /* top edge aligned to 1/2 screen height - panel height / 2 */
	PANEL_Y_MASK		= 0xf0,
};

enum uiFrameBackground {
	FRAMEBG_TRANSPARENT = 0,
	FRAMEBG_SOLID,
};

void uiDrawCursor ();

void ui_classInit (EntComponent ui, void * arg);

void ui_create (EntComponent ui, void * arg);
void ui_destroy (EntComponent ui, void * arg);

void ui_update (EntComponent ui, void * arg);

void ui_setType (EntComponent ui, void * arg);
void ui_getType (EntComponent ui, void * arg);

void ui_addValue (EntComponent ui, void * arg);

void ui_setPositionType (EntComponent ui, void * arg);
void ui_setBackground (EntComponent ui, void * arg);
void ui_setBorder (EntComponent ui, void * arg);
void ui_setLineSpacing (EntComponent ui, void * arg);
void ui_setXPosition (EntComponent ui, void * arg);
void ui_setYPosition (EntComponent ui, void * arg);
void ui_setWidth (EntComponent ui, void * arg);

void ui_handleInput (EntComponent ui, void * inputEvent);

void ui_draw (EntComponent ui, void * arg);

#endif /* XPH_COMPONENT_UI_H */