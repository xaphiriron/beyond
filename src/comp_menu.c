#include "comp_menu.h"

#include "comp_gui.h"
#include "comp_textlabel.h"

static void menu_draw (EntComponent comp, EntSpeech speech);

void menu_define (EntComponent comp, EntSpeech speech)
{
	component_registerResponse ("menu", "guiDraw", menu_draw);
}

static void menu_draw (EntComponent comp, EntSpeech speech)
{
	Entity
		this = component_entityAttached (comp),
		menuItem;
	Dynarr
		items = gui_getSubs (this);
	int
		i = 0;
	glColor4ub (0x00, 0x00, 0x00, 0xaf);
	gui_drawPane (this);
	glColor4ub (0xff, 0xff, 0xff, 0xff);
	textlabel_draw (this);
	while ((menuItem = *(Entity *)dynarr_at (items, i++)))
	{
		clickable_draw (menuItem);
		glColor4ub (0xff, 0xff, 0xff, 0xff);
		textlabel_draw (menuItem);
	}
}

void menu_addItem (Entity this, const char * text, enum input_responses action, actionCallback callback)
{
	Entity
		menuItem;
	int
		x, y,
		w, h,
		hm, vm,
		fontHeight;
	Dynarr
		items;
	int
		itemCount,
		yPlacement;
	
	menuItem = entity_create ();
	component_instantiate ("gui", menuItem);
	component_instantiate ("clickable", menuItem);
	component_instantiate ("input", menuItem);
	component_instantiate ("textlabel", menuItem);
	entity_refresh (menuItem);

	gui_xy (this, &x, &y);
	gui_wh (this, &w, &h);
	gui_vhMargin (this, &vm, &hm);
	fontHeight = fontLineHeight ();
	items = gui_getSubs (this);
	itemCount = dynarr_size (items);
	yPlacement = y + vm + itemCount * (fontHeight + vm);

	gui_place (menuItem, x + hm, yPlacement, w - (hm * 2), fontHeight + vm);
	if (action != IR_NOTHING)
		clickable_setClickInputResponse (menuItem, action);
	if (callback != NULL)
		clickable_setClickCallback (menuItem, callback);
	textlabel_set (menuItem, text, ALIGN_CENTRE, x + hm, yPlacement, w - (hm * 2));

	gui_setFrame (menuItem, this);
	if (input_hasFocus (this))
		entity_message (menuItem, this, "gainFocus", NULL);
}

void menu_addPositionedItem (Entity this, int x, int y, int w, int h, const char * text, enum input_responses action, actionCallback callback)
{
	Entity
		menuItem;
	
	menuItem = entity_create ();
	component_instantiate ("gui", menuItem);
	component_instantiate ("clickable", menuItem);
	component_instantiate ("input", menuItem);
	component_instantiate ("textlabel", menuItem);
	entity_refresh (menuItem);

	gui_place (menuItem, x, y, w, h);
	if (action != IR_NOTHING)
		clickable_setClickInputResponse (menuItem, action);
	if (callback != NULL)
		clickable_setClickCallback (menuItem, callback);
	textlabel_set (menuItem, text, ALIGN_CENTRE, x, y, w);

	gui_setFrame (menuItem, this);
	if (input_hasFocus (this))
		entity_message (menuItem, this, "gainFocus", NULL);
}