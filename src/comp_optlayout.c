#include "comp_optlayout.h"

#include "font.h"
#include "video.h"

#include "comp_clickable.h"

static void option_define (EntComponent comp, EntSpeech speech);

static void option_draw (Entity this);
static void option_setName (Entity this, const char * name);
static void option_setType (Entity this, enum option_data_types);
static void option_setValue (Entity this, const char * value);
static void option_setInfo (Entity this, const char * info);
static void option_clickCallback (Entity this);

static struct option * option_byName (Dynarr opts, const char * name);


void optlayout_create (EntComponent comp, EntSpeech speech);
void optlayout_destroy (EntComponent comp, EntSpeech speech);

void optlayout_draw (EntComponent comp, EntSpeech speech);
void optlayout_input (EntComponent comp, EntSpeech speech);

void optlayout_gainFocus (EntComponent comp, EntSpeech speech);
void optlayout_loseFocus (EntComponent comp, EntSpeech speech);

static void optlayout_cancelCallback (Entity cancel);

void optlayout_define (EntComponent comp, EntSpeech speech)
{
	component_registerResponse ("optlayout", "__create", optlayout_create);
	component_registerResponse ("optlayout", "__destroy", optlayout_destroy);

	component_registerResponse ("optlayout", "guiDraw", optlayout_draw);
	component_registerResponse ("optlayout", "FOCUS_INPUT", optlayout_input);

	component_registerResponse ("optlayout", "gainFocus", optlayout_gainFocus);
	component_registerResponse ("optlayout", "loseFocus", optlayout_loseFocus);

	component_register ("option", option_define);
}

// FIXME: we have to set the gui dimensions before creating the optlayout because there's not a good place to put the continue/cancel init aside from here. this is part of a general problem of not really being able to resize anything
void optlayout_create (EntComponent comp, EntSpeech speech)
{
	struct optlayout
		* layout = xph_alloc (sizeof (struct optlayout));
	layout->options = dynarr_create (2, sizeof (Entity));

	component_setData (comp, layout);

	Entity
		this = component_entityAttached (comp);
	int
		x, y,
		w, h,
		hm, vm,
		fontHeight;

	gui_xy (this, &x, &y);
	gui_wh (this, &w, &h);
	gui_vhMargin (this, &hm, &vm);
	fontHeight = fontLineHeight ();

	layout->confirm = entity_create ();
	component_instantiate ("gui", layout->confirm);
	component_instantiate ("clickable", layout->confirm);
	component_instantiate ("input", layout->confirm);
	entity_refresh (layout->confirm);
	gui_place (layout->confirm, x, y + h - (vm * 2 + fontHeight), w / 2, fontHeight + vm);
	gui_setFrame (layout->confirm, this);
	clickable_setClickCallback (layout->confirm, NULL);

	layout->cancel = entity_create ();
	component_instantiate ("gui", layout->cancel);
	component_instantiate ("clickable", layout->cancel);
	component_instantiate ("input", layout->cancel);
	entity_refresh (layout->cancel);
	gui_place (layout->cancel, x + w / 2, y + h - (vm * 2 + fontHeight), w / 2, fontHeight + vm);
	gui_setFrame (layout->cancel, this);
	clickable_setClickCallback (layout->cancel, optlayout_cancelCallback);
/*
	layout->confirm = gui_addTarget (this, x, y + h - (vm * 2 + fontHeight), w / 2, fontHeight + vm, NULL, optlayout_confirmClick);

	layout->cancel = gui_addTarget (this, x + w / 2, y + h - (vm * 2 + fontHeight), w / 2, fontHeight + vm, NULL, optlayout_cancelClick);
*/
	layout->confirmText = fontGenerate ("Continue", ALIGN_CENTRE, x, y + h - (vm * 2 + fontHeight), w / 2);
	layout->cancelText = fontGenerate ("Cancel", ALIGN_CENTRE, x + w / 2, y + h - (vm * 2 + fontHeight), w / 2);
}

void optlayout_destroy (EntComponent comp, EntSpeech speech)
{
	struct optlayout
		* opt = component_getData (comp);
	// FIXME: entities that destroy other entities in their destruction code can probably lead to a double-free bug if they're only destroyed when everything is destroyed (or alternately they could cause the entity list to get into a bad state if they destroy entities while other code is iterating through it) - xph 2012 02 05
	dynarr_map (opt->options, (void (*)(void *))entity_destroy);
	dynarr_destroy (opt->options);
	entity_destroy (opt->cancel);
	entity_destroy (opt->confirm);
	fontDestroyText (opt->cancelText);
	fontDestroyText (opt->confirmText);
	xph_free (opt);

	component_clearData (comp);
}

void optlayout_draw (EntComponent comp, EntSpeech speech)
{
	Entity
		this = component_entityAttached (comp);
	struct optlayout
		* layout = component_getData (comp);
	Entity
		option = NULL;
	int
		i = 0;
	glColor4ub (0x00, 0x00, 0x00, 0xaf);
	gui_drawPane (this);
	
	while ((option = *(Entity *)dynarr_at (layout->options, i++)))
	{
		option_draw (option);
	}
	clickable_draw (layout->confirm);
	glColor4ub (0xff, 0xff, 0xff, 0xff);
	fontTextPrint (layout->confirmText);

	clickable_draw (layout->cancel);
	glColor4ub (0xff, 0xff, 0xff, 0xff);
	fontTextPrint (layout->cancelText);
}

void optlayout_input (EntComponent comp, EntSpeech speech)
{
	struct optlayout
		* layout = component_getData (comp);
	inputEvent
		* event = speech->arg;
	Clickable
		cancel,
		confirm;
	if (!event->active)
		return;
	switch (event->code)
	{
		case IR_UI_CONFIRM:
			confirm = component_getData (entity_getAs (layout->confirm, "clickable"));
			if (confirm->click)
				confirm->click (layout->confirm);
			break;
		case IR_UI_CANCEL:
			cancel = component_getData (entity_getAs (layout->cancel, "clickable"));
			if (cancel->click)
				cancel->click (layout->cancel);
			break;
		default:
			break;
	}
}

void optlayout_gainFocus (EntComponent comp, EntSpeech speech)
{
	Entity
		this = component_entityAttached (comp);
	struct optlayout
		* layout = component_getData (entity_getAs (this, "optlayout"));

	entity_messageDynarr (layout->options, this, "gainFocus", NULL);
	entity_message (layout->confirm, this, "gainFocus", NULL);
	entity_message (layout->cancel, this, "gainFocus", NULL);
}

void optlayout_loseFocus (EntComponent comp, EntSpeech speech)
{
	Entity
		this = component_entityAttached (comp);
	struct optlayout
		* layout = component_getData (entity_getAs (this, "optlayout"));

	entity_messageDynarr (layout->options, this, "loseFocus", NULL);
	entity_message (layout->confirm, this, "loseFocus", NULL);
	entity_message (layout->cancel, this, "loseFocus", NULL);
}

void optlayout_confirm (Entity this, actionCallback callback)
{
	struct optlayout
		* layout = component_getData (entity_getAs (this, "optlayout"));
	if (!layout)
		return;
	clickable_setClickCallback (layout->confirm, callback);
}

static void optlayout_cancelCallback (Entity cancel)
{
	Entity
		frame = gui_getFrame (cancel);
	entity_message (frame, NULL, "loseFocus", NULL);
	entity_destroy (frame);
}

void optlayout_addOption (Entity this, const char * name, enum option_data_types type, const char * defaultVal, const char * info)
{
	struct optlayout
		* layout = component_getData (entity_getAs (this, "optlayout"));
	Entity
		option;
	int
		x, y,
		w, h,
		hm, vm,
		fontHeight;
	if (!layout)
		return;
	gui_xy (this, &x, &y);
	gui_wh (this, &w, &h);
	gui_vhMargin (this, &hm, &vm);
	fontHeight = fontLineHeight ();

	option = entity_create ();
	component_instantiate ("gui", option);
	component_instantiate ("clickable", option);
	component_instantiate ("input", option);
	component_instantiate ("option", option);
	entity_refresh (option);
	gui_place (option, x, y + vm + (fontHeight + vm / 2) * layout->lines, w, (fontHeight + vm / 2) * (info ? 2 : 1));
	gui_setFrame (option, this);
	gui_setMargin (option, vm, hm);
	clickable_setClickCallback (option, option_clickCallback);
	option_setName (option, name);
	option_setType (option, type);
	option_setValue (option, defaultVal);
	if (info)
		option_setInfo (option, info);
	dynarr_push (layout->options, option);

	if (input_hasFocus (this))
		entity_message (option, this, "gainFocus", NULL);

	layout->lines += info ? 2 : 1;
}

static struct option * option_byName (Dynarr opts, const char * name)
{
	Entity
		option;
	int
		i = 0;
	struct option
		* opt;
	while ((option = *(Entity *)dynarr_at (opts, i++)))
	{
		opt = component_getData (entity_getAs (option, "option"));
		if (strcmp (opt->name, name) == 0)
			return opt;
	}
	return NULL;
}

const char * optlayout_optionStrValue (Entity this, const char * name)
{
	struct optlayout
		* layout = component_getData (entity_getAs (this, "optlayout"));
	struct option
		* opt;
	if (!layout)
		return NULL;
	opt = option_byName (layout->options, name);
	if (!opt)
		return NULL;
	return opt->dataAsString;
}

signed long optlayout_optionNumValue (Entity this, const char * name)
{
	const char
		* str = optlayout_optionStrValue (this, name);
	if (optlayout_optionIsNumeric (this, name))
		return (signed long)strtod (str, NULL);
	return 0;
}

bool optlayout_optionIsNumeric (Entity this, const char * name)
{
	const char
		* val = optlayout_optionStrValue (this, name);
	char
		* unconverted;
	if (!val)
		return false;
	strtod (val, &unconverted);
	if (*unconverted != 0)
		return false;
	return true;
}

// hash function "djb2" from http://www.cse.yorku.ca/~oz/hash.html
unsigned long hash (const char * str)
{
	unsigned long
		hash = 5381;
	int
		c;

	while ((c = *str++))
		hash = ((hash << 5) + hash) ^ c; /* hash * 33 + c */
	return hash;
};

/***
 * OPTION COMPONENT
 ***/

static void option_create (EntComponent comp, EntSpeech speech);
static void option_destroy (EntComponent comp, EntSpeech speech);

static void option_input (EntComponent comp, EntSpeech speech);

static void option_define (EntComponent comp, EntSpeech speech)
{
	component_registerResponse ("option", "__create", option_create);
	component_registerResponse ("option", "__destroy", option_destroy);

	component_registerResponse ("option", "FOCUS_INPUT", option_input);
}

static void option_create (EntComponent comp, EntSpeech speech)
{
	struct option
		* opt = xph_alloc (sizeof (struct option));

	component_setData (comp, opt);
}

static void option_destroy (EntComponent comp, EntSpeech speech)
{
	struct option
		* opt = component_getData (comp);
	xph_free (opt);

	component_clearData (comp);
}

static void option_input (EntComponent comp, EntSpeech speech)
{
	struct option
		* opt = component_getData (comp);
	inputEvent
		* event = speech->arg;
	if (!opt->textFocus)
		return;
	if (!event || !event->active)
		return;
	switch (event->code)
	{
		case IR_TEXT:
			if (event->event->key.keysym.sym == SDLK_BACKSPACE)
			{
				if (opt->dataCursor > 0)
					opt->dataAsString[--opt->dataCursor] = 0;
			}
			else if (event->event->key.keysym.unicode && opt->dataCursor < 32)
				opt->dataAsString[opt->dataCursor++] = event->event->key.keysym.unicode;
			break;
		default:
			break;
	}
}

static void option_draw (Entity this)
{
	struct option
		* opt = component_getData (entity_getAs (this, "option"));
	int
		x, y,
		w, h,
		hm, vm,
		fontHeight;
	if (opt->textFocus)
	{
		glColor4ub (0xff, 0x00, 0x00, 0xff);
		gui_drawPane (this);
	}
	else
		clickable_draw (this);
	gui_xy (this, &x, &y);
	gui_wh (this, &w, &h);
	gui_vhMargin (this, &vm, &hm);
	fontHeight = fontLineHeight ();

	glColor4ub (0xff, 0xff, 0xff, 0xff);
	fontPrintAlign (ALIGN_RIGHT);
	fontPrint (opt->name, x + w / 2 - hm, y);
	fontPrintAlign (ALIGN_LEFT);
	fontPrint (opt->dataAsString, x + w / 2 + hm, y);
	if (opt->info[0] != 0)
	{
		glColor4ub (0xaf, 0xaf, 0xaf, 0xff);
		fontPrint (opt->info, x + hm, y + (fontHeight + vm / 2));
	}
}

static void option_setName (Entity this, const char * name)
{
	struct option
		* option = component_getData (entity_getAs (this, "option"));
	strncpy (option->name, name, 32);
}

static void option_setType (Entity this, enum option_data_types type)
{
	struct option
		* option = component_getData (entity_getAs (this, "option"));
	option->type = type;
}

static void option_setValue (Entity this, const char * value)
{
	struct option
		* option = component_getData (entity_getAs (this, "option"));
	strncpy (option->dataAsString, value, 32);
	option->dataCursor = strlen (value);
}

static void option_setInfo (Entity this, const char * info)
{
	struct option
		* option = component_getData (entity_getAs (this, "option"));
	strncpy (option->info, info, 256);
}

static void option_clickCallback (Entity this)
{
	Entity
		optFrame = gui_getFrame (this),
		opt;
	struct option
		* option;
	struct optlayout
		* layout = component_getData (entity_getAs (optFrame, "optlayout"));
	int
		i = 0;
	while ((opt = *(Entity *)dynarr_at (layout->options, i++)))
	{
		option = component_getData (entity_getAs (opt, "option"));
		if (opt == this)
			option->textFocus = true;
		else
			option->textFocus = false;
	}
	
}
