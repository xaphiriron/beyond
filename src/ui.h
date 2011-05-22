#ifndef XPH_UI_H
#define XPH_UI_H

enum uiPanelTypes
{
	UI_NONE = 0x00,
	UI_WORLDMAP,
};

typedef union uiPanels * UIPANEL;

UIPANEL uiCreatePanel (enum uiPanelTypes type, ...);
void uiDestroyPanel (UIPANEL p);
enum uiPanelTypes uiGetPanelType (const UIPANEL p);

void uiDrawPanel (const UIPANEL p);

void uiDrawWorldmap (const UIPANEL p);

#endif /* XPH_UI_H */