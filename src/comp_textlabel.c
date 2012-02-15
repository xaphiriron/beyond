/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#include "comp_textlabel.h"

static void textlabel_create (EntComponent comp, EntSpeech speech);
static void textlabel_destroy (EntComponent comp, EntSpeech speech);

void textlabel_define (EntComponent comp, EntSpeech speech)
{
	component_registerResponse ("textlabel", "__create", textlabel_create);
	component_registerResponse ("textlabel", "__destroy", textlabel_destroy);
}

static void textlabel_create (EntComponent comp, EntSpeech speech)
{
	struct xph_textlabel
		* textlabel = xph_alloc (sizeof (struct xph_textlabel));

	component_setData (comp, textlabel);
}

static void textlabel_destroy (EntComponent comp, EntSpeech speech)
{
	struct xph_textlabel
		* textlabel = component_getData (comp);
	if (textlabel->text)
		fontDestroyText (textlabel->text);
	xph_free (textlabel);

	component_clearData (comp);
}

void textlabel_set (Entity this, const char * text, enum textAlignType align, int x, int y, int w)
{
	struct xph_textlabel
		* textlabel = component_getData (entity_getAs (this, "textlabel"));
	if (!textlabel)
		return;
	if (textlabel->text)
		fontDestroyText (textlabel->text);
	textlabel->text = fontGenerate (text, align, x, y, w);
}

void textlabel_draw (Entity this)
{
	struct xph_textlabel
		* textlabel = component_getData (entity_getAs (this, "textlabel"));
	if (!textlabel)
		return;
	fontTextPrint (textlabel->text);
}
