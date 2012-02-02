#include "comp_gui.h"

#include "video.h"
#include "component_input.h"

static Dynarr
	GUIDepthStack = NULL;

void gui_place (Entity e, int x, int y, int w, int h)
{
	GUI
		gui = component_getData (entity_getAs (e, "gui"));
	if (!gui)
		return;
	gui->x = x;
	gui->y = y;
	gui->w = w;
	gui->h = h;
}

void gui_setMargin (Entity e, int vert, int horiz)
{
	GUI
		gui = component_getData (entity_getAs (e, "gui"));
	if (!gui)
		return;
	gui->vertMargin = vert;
	gui->horizMargin = horiz;
}

static void gui_classDestroy (EntComponent comp, EntSpeech speech);
static void gui_create (EntComponent comp, EntSpeech speech);
static void gui_destroy (EntComponent comp, EntSpeech speech);
static void gui_input (EntComponent comp, EntSpeech speech);

static void gui_gainFocus (EntComponent comp, EntSpeech speech);

void gui_define (EntComponent comp, EntSpeech speech)
{
	component_registerResponse ("gui", "__classDestroy", gui_classDestroy);

	component_registerResponse ("gui", "__create", gui_create);
	component_registerResponse ("gui", "__destroy", gui_destroy);

	component_registerResponse ("gui", "FOCUS_INPUT", gui_input);

	component_registerResponse ("gui", "gainFocus", gui_gainFocus);

	GUIDepthStack = dynarr_create (4, sizeof (Entity *));
}

static void gui_classDestroy (EntComponent comp, EntSpeech speech)
{
	dynarr_destroy (GUIDepthStack);
	GUIDepthStack = NULL;
}

static void gui_create (EntComponent comp, EntSpeech speech)
{
	GUI
		gui = xph_alloc (sizeof (struct xph_gui));
	gui->confirmCallback = gui_defaultCallback;
	gui->cancelCallback = gui_defaultCallback;
	gui->targets = dynarr_create (2, sizeof (struct xph_gui_target *));

	component_setData (comp, gui);
}

static void gui_destroy (EntComponent comp, EntSpeech speech)
{
	Entity
		this = component_entityAttached (comp),
		nextHighest;
	GUI
		gui = component_getData (comp);
	dynarr_map (gui->targets, xph_free);
	dynarr_destroy (gui->targets);
	xph_free (gui);

	component_clearData (comp);

	if (*(Entity *)dynarr_back (GUIDepthStack) == this)
	{
		dynarr_unset (GUIDepthStack, dynarr_size (GUIDepthStack) - 1);
		if ((nextHighest = *(Entity *)dynarr_back (GUIDepthStack)))
		{
			entity_message (nextHighest, NULL, "gainFocus", NULL);
		}
	}
	else
	{
		dynarr_remove_condense (GUIDepthStack, this);
	}
}

// the usage of guiEntities here seems to imply that this code would be better off in a gui system -- it deals with interrellations between GUI elements in a way that involves the global scope, which means it should be systems code instead of component code - xph 2012 01 29
static void gui_input (EntComponent comp, EntSpeech speech)
{
	Entity
		this = component_entityAttached (comp);
	GUI
		gui = component_getData (comp);
	inputEvent
		* event = speech->arg;
	struct xph_gui_target
		* hit;

	if (!event->active)
		return;
	switch (event->code)
	{
		case IR_UI_CONFIRM:
			if (gui->confirmCallback)
				gui->confirmCallback (this);
			break;
		case IR_UI_CANCEL:
			if (gui->cancelCallback)
				gui->cancelCallback (this);
			break;
		case IR_MOUSEMOVE:
			hit = gui_hit (this, event->event->motion.x, event->event->motion.y);
			if (hit && hit->hover)
				hit->hover (this, hit);
			break;
		case IR_MOUSECLICK:
			hit = gui_hit (this, event->event->button.x, event->event->button.y);
			if (hit && hit->click)
				hit->click (this, hit);
			break;
		default:
			break;
	}
}

void gui_gainFocus (EntComponent comp, EntSpeech speech)
{
	Entity
		this = component_entityAttached (comp);
	if (*(Entity *)dynarr_back (GUIDepthStack) != this)
	{
		dynarr_remove_condense (GUIDepthStack, this);
		dynarr_push (GUIDepthStack, this);
	}
}


void gui_update (Dynarr entities)
{
	EntSpeech
		message;
	inputEvent
		* event;

	while ((message = entitySystem_dequeueMessage ("gui")))
	{
		if (strcmp (message->message, "FOCUS_INPUT") == 0)
		{
			event = message->arg;
			switch (event->code)
			{
				default:
					break;
			}
		}
	}
}

void gui_render (Dynarr entities)
{
	Entity
		active;
	int
		i = 0;

	glLoadIdentity ();
	glDisable (GL_DEPTH_TEST);

	while ((active = *(Entity *)dynarr_at (GUIDepthStack, i++)))
	{
		entity_message (active, NULL, "guiDraw", NULL);
	}

	glEnable (GL_DEPTH_TEST);
}


void gui_placeOnStack (Entity this)
{
	Entity
		prev;
	GUI
		gui = component_getData (entity_getAs (this, "gui"));
	if (!gui)
		return;
	prev = *(Entity *)dynarr_back (GUIDepthStack);
	if (prev != this)
	{
		dynarr_remove_condense (GUIDepthStack, this);
		dynarr_push (GUIDepthStack, this);
		entity_message (prev, NULL, "loseFocus", NULL);
		entity_message (this, NULL, "gainFocus", NULL);
	}
}

GUITarget gui_addTarget (Entity this, int x, int y, int w, int h, targetCallback hover, targetCallback click)
{
	GUI
		gui = component_getData (entity_getAs (this, "gui"));
	struct xph_gui_target
		* target;
	if (!gui)
		return NULL;
	target = xph_alloc (sizeof (struct xph_gui_target));
	target->x = x;
	target->y = y;
	target->w = w;
	target->h = h;
	target->click = click;
	if (hover)
		target->hover = hover;
	else
		target->hover = gui_defaultHover;
	dynarr_push (gui->targets, target);
	return target;
}

struct xph_gui_target * gui_hit (Entity this, int x, int y)
{
	GUI
		gui = component_getData (entity_getAs (this, "gui"));
	struct xph_gui_target
		* target;
	int
		i = 0;
	if (!gui)
		return NULL;
	while ((target = *(struct xph_gui_target **)dynarr_at (gui->targets, i++)))
	{
		if (x >= target->x && x < target->x + target->w
		 && y >= target->y && y < target->y + target->h)
			return target;
	}
	return NULL;
}

void gui_rmTarget (Entity this, struct xph_gui_target * target)
{
	GUI
		gui = component_getData (entity_getAs (this, "gui"));
	if (!gui)
		return;
	dynarr_remove_condense (gui->targets, target);
	xph_free (target);
}

void gui_confirmCallback (Entity this, void (*callback)(Entity))
{
	GUI
		gui = component_getData (entity_getAs (this, "gui"));
	if (!gui)
		return;
	gui->confirmCallback = callback;
}

void gui_cancelCallback (Entity this, void (*callback)(Entity))
{
	GUI
		gui = component_getData (entity_getAs (this, "gui"));
	if (!gui)
		return;
	gui->cancelCallback = callback;
}

void gui_defaultCallback (Entity gui)
{
	entity_message (gui, NULL, "loseFocus", NULL);
	entity_destroy (gui);
}

void gui_defaultHover (Entity this, GUITarget hit)
{
	GUI
		gui = component_getData (entity_getAs (this, "gui"));
	GUITarget
		target;
	int
		i = 0;
	if (gui->lastHover == hit)
		return;
	while ((target = *(GUITarget *)dynarr_at (gui->targets, i++)))
	{
		if (target == hit)
			target->hasHover = true;
		else
			target->hasHover = false;
	}
	gui->lastHover = hit;
}

bool gui_xy (Entity e, int * x, int * y)
{
	GUI
		gui = component_getData (entity_getAs (e, "gui"));
	assert (x && y);
	if (!gui)
		return false;
	*x = gui->x;
	*y = gui->y;
	return true;
}

bool gui_wh (Entity e, int * w, int * h)
{
	GUI
		gui = component_getData (entity_getAs (e, "gui"));
	assert (w && h);
	if (!gui)
		return false;
	*w = gui->w;
	*h = gui->h;
	return true;
}

bool gui_vhMargin (Entity e, int * v, int * h)
{
	GUI
		gui = component_getData (entity_getAs (e, "gui"));
	assert (v && h);
	if (!gui)
		return false;
	*v = gui->vertMargin;
	*h = gui->horizMargin;
	return true;
}


void gui_drawPane (Entity e)
{
	GUI
		gui = component_getData (entity_getAs (e, "gui"));
	float
		top, bottom,
		left, right,
		zNear;
	if (!gui)
		return;
	top = video_yMap (gui->y);
	bottom = video_yMap (gui->y + gui->h);
	left = video_xMap (gui->x);
	right = video_xMap (gui->x + gui->w);
	zNear = video_getZnear () - 0.001;

	glBindTexture (GL_TEXTURE_2D, 0);

	glBegin (GL_TRIANGLE_FAN);
	glVertex3f (left, top, zNear);
	glVertex3f (left, bottom, zNear);
	glVertex3f (right, bottom, zNear);
	glVertex3f (right, top, zNear);
	glEnd ();

	return;
}

void gui_drawTargetPane (GUITarget target)
{
	float
		top, bottom,
		left, right,
		zNear;
	assert (target);
	top = video_yMap (target->y);
	bottom = video_yMap (target->y + target->h);
	left = video_xMap (target->x);
	right = video_xMap (target->x + target->w);
	zNear = video_getZnear () - 0.001;

	glBindTexture (GL_TEXTURE_2D, 0);

	glBegin (GL_TRIANGLE_FAN);
	glVertex3f (left, top, zNear);
	glVertex3f (left, bottom, zNear);
	glVertex3f (right, bottom, zNear);
	glVertex3f (right, top, zNear);
	glEnd ();

	return;
}

void gui_drawPaneCoords (int x, int y, int width, int height)
{
	float
		left = video_xMap (x),
		right = video_xMap (x + width),
		top = video_yMap (y),
		bottom = video_yMap (y + height),
		zNear = video_getZnear () - 0.001;
	glBindTexture (GL_TEXTURE_2D, 0);

	glBegin (GL_TRIANGLE_FAN);
	glVertex3f (left, top, zNear);
	glVertex3f (left, bottom, zNear);
	glVertex3f (right, bottom, zNear);
	glVertex3f (right, top, zNear);
	glEnd ();
}
