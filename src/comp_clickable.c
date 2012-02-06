#include "comp_clickable.h"

#include "component_input.h"

// this is only required because of the default ->inside value of gui_inside and because of gui_drawPane in clickable_draw
#include "comp_gui.h"

#include "video.h"

static void clickable_create (EntComponent comp, EntSpeech speech);
static void clickable_destroy (EntComponent comp, EntSpeech speech);

static void clickable_input (EntComponent comp, EntSpeech speech);

void clickable_define (EntComponent comp, EntSpeech speech)
{
	component_registerResponse ("clickable", "__create", clickable_create);
	component_registerResponse ("clickable", "__destroy", clickable_destroy);

	component_registerResponse ("clickable", "FOCUS_INPUT", clickable_input);
}

static void clickable_create (EntComponent comp, EntSpeech speech)
{
	Clickable
		click = xph_alloc (sizeof (struct xph_clickable));
	// TODO: write some 3d picking code so it's possible to have clickable things in 3d space
	click->inside = gui_inside;
	click->inputResponse = IR_NOTHING;

	component_setData (comp, click);
}

static void clickable_destroy (EntComponent comp, EntSpeech speech)
{
	Clickable
		click = component_getData (comp);
	xph_free (click);

	component_clearData (comp);
}

static void clickable_input (EntComponent comp, EntSpeech speech)
{
	Entity
		this = component_entityAttached (comp);
	Clickable
		click = component_getData (comp);
	inputEvent
		* event = speech->arg;
	bool
		hit;
	if (!click->inside)
		return;
	switch (event->code)
	{
		case IR_MOUSEMOVE:
			hit = click->inside (this, event->event->motion.x, event->event->motion.y);
			if (!click->hasClick && hit)
			{
				click->hasHover = true;
				if (click->hover)
					click->hover (this);
			}
			else if (click->hasHover && !hit)
			{
				click->hasHover = false;
			}
			break;
		case IR_MOUSECLICK:
			hit = click->inside (this, event->event->button.x, event->event->button.y);
			if (event->active)
			{
				if (hit)
					click->hasClick = true;
			}
			// click release and this has click focus
			else if (click->hasClick)
			{
				// if pointer is still inside, fire click event
				if (hit)
				{
					if (click->inputResponse != IR_NOTHING)
						input_sendAction (click->inputResponse);
					if (click->click)
						click->click (this);
				}
				click->hasClick = false;
			}
			break;
		default:
			break;
	}
}

void clickable_setHoverCallback (Entity this, actionCallback hoverAct)
{
	Clickable
		click = component_getData (entity_getAs (this, "clickable"));
	if (!click)
		return;
	click->hover = hoverAct;
}

void clickable_setClickCallback (Entity this, actionCallback clickAct)
{
	Clickable
		click = component_getData (entity_getAs (this, "clickable"));
	if (!click)
		return;
	click->click = clickAct;
}

void clickable_setClickInputResponse (Entity this, enum input_responses response)
{
	Clickable
		click = component_getData (entity_getAs (this, "clickable"));
	if (!click)
		return;
	click->inputResponse = response;
}

bool clickable_hasHover (Entity this)
{
	Clickable
		click = component_getData (entity_getAs (this, "clickable"));
	if (!click)
		return false;
	return click->hasHover;
}

bool clickable_hasClick (Entity this)
{
	Clickable
		click = component_getData (entity_getAs (this, "clickable"));
	if (!click)
		return false;
	return click->hasClick;
}

void clickable_draw (Entity this)
{
	if (clickable_hasClick (this) || clickable_hasHover (this))
	{
		if (clickable_hasClick (this))
			glColor4ub (0xff, 0x99, 0x00, 0xaf);
		else
			glColor4ub (0x00, 0x99, 0xff, 0xaf);
		gui_drawPane (this);
	}
}