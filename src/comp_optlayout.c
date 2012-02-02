#include "comp_optlayout.h"

#include "font.h"
#include "video.h"

static bool option_parseString (struct option * opt, const char * string);
static struct option * option_byName (Dynarr opts, const char * name);

static void optlayout_confirmClick (Entity this, GUITarget target);
static void optlayout_cancelClick (Entity this, GUITarget target);

void optlayout_create (EntComponent comp, EntSpeech speech);
void optlayout_destroy (EntComponent comp, EntSpeech speech);

void optlayout_draw (EntComponent comp, EntSpeech speech);
void optlayout_input (EntComponent comp, EntSpeech speech);

void optlayout_define (EntComponent comp, EntSpeech speech)
{
	component_registerResponse ("optlayout", "__create", optlayout_create);
	component_registerResponse ("optlayout", "__destroy", optlayout_destroy);

	component_registerResponse ("optlayout", "guiDraw", optlayout_draw);
	component_registerResponse ("optlayout", "FOCUS_INPUT", optlayout_input);
}

// FIXME: we have to set the gui dimensions before creating the optlayout because there's not a good place to put the continue/cancel init aside from here. this is part of a general problem of not really being able to resize anything
void optlayout_create (EntComponent comp, EntSpeech speech)
{
	struct optlayout
		* layout = xph_alloc (sizeof (struct optlayout));
	layout->options = dynarr_create (2, sizeof (struct option *));

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

	layout->confirm = gui_addTarget (this, x, y + h - (vm * 2 + fontHeight), w / 2, fontHeight + vm, NULL, optlayout_confirmClick);

	layout->cancel = gui_addTarget (this, x + w / 2, y + h - (vm * 2 + fontHeight), w / 2, fontHeight + vm, NULL, optlayout_cancelClick);

}

void optlayout_destroy (EntComponent comp, EntSpeech speech)
{
	struct optlayout
		* opt = component_getData (comp);
	dynarr_map (opt->options, xph_free);
	dynarr_destroy (opt->options);
	xph_free (opt);

	component_clearData (comp);
}

void optlayout_draw (EntComponent comp, EntSpeech speech)
{
	Entity
		this = component_entityAttached (comp);
	struct optlayout
		* layout = component_getData (comp);
	struct option
		* opt = NULL;
	int
		i = 0,
		infoLines = 0,
		guiX,
		guiY,
		guiW = 0,
		guiH = 0,
		guiVMargin = 0,
		guiHMargin = 0,
		fontHeight = fontLineHeight ();
	if (!gui_xy (this, &guiX, &guiY))
	{
		guiX = 0;
		guiY = 0;
	}
	if (!gui_wh (this, &guiW, &guiH) || guiW <= 0 || guiH <= 0)
	{
		video_getDimensions ((unsigned int *)&guiW, (unsigned int *)&guiH);
	}
	gui_vhMargin (this, &guiVMargin, &guiHMargin);
	glColor4ub (0x00, 0x00, 0x00, 0xaf);
	gui_drawPane (this);
	
	while ((opt = *(struct option **)dynarr_at (layout->options, i)))
	{
		if (opt->target->hasHover)
		{
			glColor4ub (0x00, 0x99, 0xff, 0xaf);
			gui_drawTargetPane (opt->target);
		}
		glColor4ub (0xff, 0xff, 0xff, 0xff);
		fontPrintAlign (ALIGN_RIGHT);
		fontPrint (opt->name, guiX + guiW / 2 - guiHMargin, guiY + guiVMargin + (i + infoLines) * (fontHeight + guiVMargin / 2));
		fontPrintAlign (ALIGN_LEFT);
		fontPrint (opt->dataAsString, guiX + guiW / 2 + guiHMargin, guiY + guiVMargin + (i + infoLines) * (fontHeight + guiVMargin / 2));
		if (opt->info[0] != 0)
		{
			infoLines++;
			glColor4ub (0xaf, 0xaf, 0xaf, 0xff);
			fontPrint (opt->info, guiX + guiHMargin + fontHeight / 2, guiY + guiVMargin + (i + infoLines) * (fontHeight + guiVMargin / 2));
		}
		i++;
	}
	if (layout->confirm->hasHover)
	{
		glColor4ub (0x00, 0x99, 0xff, 0xaf);
		gui_drawTargetPane (layout->confirm);
	}
	glColor4ub (0xff, 0xff, 0xff, 0xff);
	fontPrint ("Continue", guiX + guiHMargin * 2, guiY + guiH - (guiVMargin * 2 + fontHeight));
	if (layout->cancel->hasHover)
	{
		glColor4ub (0x00, 0x99, 0xff, 0xaf);
		gui_drawTargetPane (layout->cancel);
	}
	glColor4ub (0xff, 0xff, 0xff, 0xff);
	fontPrintAlign (ALIGN_RIGHT);
	fontPrint ("Cancel", guiX + guiW - guiHMargin * 2, guiY + guiH - (guiVMargin * 2 + fontHeight));
}

void optlayout_input (EntComponent comp, EntSpeech speech)
{
	inputEvent
		* event = speech->arg;
	if (!event->active)
		return;
	switch (event->code)
	{
		default:
			break;
	}

}

static void optlayout_confirmClick (Entity this, GUITarget target)
{
	input_sendAction (IR_UI_CONFIRM);
}

static void optlayout_cancelClick (Entity this, GUITarget target)
{
	entity_message (this, NULL, "loseFocus", NULL);
	entity_destroy (this);
}

void optlayout_addOption (Entity this, const char * name, enum option_data_types type, const char * defaultVal, const char * info)
{
	struct optlayout
		* layout = component_getData (entity_getAs (this, "optlayout"));
	struct option
		* opt;
	int
		x, y,
		w, h,
		hm, vm,
		fontHeight;
	if (!layout)
		return;
	opt = xph_alloc (sizeof (struct option));
	strncpy (opt->name, name, 32);
	strncpy (opt->dataAsString, defaultVal, 32);
	if (info)
		strncpy (opt->info, info, 256);
	option_parseString (opt, defaultVal);
	dynarr_push (layout->options, opt);

	gui_xy (this, &x, &y);
	gui_wh (this, &w, &h);
	gui_vhMargin (this, &hm, &vm);
	fontHeight = fontLineHeight ();
	opt->target = gui_addTarget (this, x, y + vm + (fontHeight + vm / 2) * layout->lines, w, (fontHeight + vm / 2) * (info ? 2 : 1), NULL, NULL);

	layout->lines += info ? 2 : 1;
}

static bool option_parseString (struct option * opt, const char * string)
{
	return 1;
}

static struct option * option_byName (Dynarr opts, const char * name)
{
	struct option
		* opt;
	int
		i = 0;
	while ((opt = *(struct option **)dynarr_at (opts, i++)))
	{
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