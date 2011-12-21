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
void uiLoadPanelTexture (const char * path);


/* don't know if it's really useful to have these public - xph 2011 09 29*/
void ui_getXY (UIPANEL ui, signed int * x, signed int * y);
void ui_getWH (UIPANEL ui, signed int * w, signed int * h);


void ui_define (EntComponent ui, EntSpeech speech);

void ui_system (Dynarr uiEntities);
void uiRender_system (Dynarr entities);

#endif /* XPH_COMPONENT_UI_H */