/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#include "comp_gui.h"

#include "video.h"

static Dynarr
	GUIDepthStack = NULL;

void gui_defaultClose (Entity this, const inputEvent const * event)
{
	entity_message (this, NULL, "loseFocus", NULL);
	entity_destroy (this);
}



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

void gui_setFrame (Entity this, Entity frame)
{
	GUI
		gui = component_getData (entity_getAs (this, "gui")),
		oldFrame,
		newFrame;
	if (!gui)
		return;
	if (gui->frame)
	{
		if (gui->frame == frame)
			return;
		oldFrame = component_getData (entity_getAs (gui->frame, "gui"));
		assert (oldFrame);
		dynarr_remove_condense (oldFrame->subs, this);
	}
	gui->frame = frame;
	newFrame = component_getData (entity_getAs (gui->frame, "gui"));
	assert (newFrame);
	dynarr_push (newFrame->subs, this);
}

static void gui_classDestroy (EntComponent comp, EntSpeech speech);
static void gui_create (EntComponent comp, EntSpeech speech);
static void gui_destroy (EntComponent comp, EntSpeech speech);

static void gui_gainFocus (EntComponent comp, EntSpeech speech);
static void gui_loseFocus (EntComponent comp, EntSpeech speech);

void gui_define (EntComponent comp, EntSpeech speech)
{
	component_registerResponse ("gui", "__classDestroy", gui_classDestroy);

	component_registerResponse ("gui", "__create", gui_create);
	component_registerResponse ("gui", "__destroy", gui_destroy);

	component_registerResponse ("gui", "gainFocus", gui_gainFocus);
	component_registerResponse ("gui", "loseFocus", gui_loseFocus);

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
	gui->subs = dynarr_create (2, sizeof (Entity));

	component_setData (comp, gui);
}

static void gui_destroy (EntComponent comp, EntSpeech speech)
{
	Entity
		this = component_entityAttached (comp),
		nextHighest;
	GUI
		gui = component_getData (comp);
	// TODO: should this: update all its subs to have no frame/update all its subs to have its frame if it has one/destroy all subs/do nothing??? (currently doing nothing)
	xph_free (gui->subs);
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

void gui_gainFocus (EntComponent comp, EntSpeech speech)
{
	Entity
		this = component_entityAttached (comp);
	GUI
		gui = component_getData (comp);
	if (in_dynarr (GUIDepthStack, this) != -1)
		gui_placeOnStack (this);

	entity_messageDynarr (gui->subs, this, "gainFocus", NULL);
}

void gui_loseFocus (EntComponent comp, EntSpeech speech)
{
	Entity
		this = component_entityAttached (comp);
	GUI
		gui = component_getData (comp);
	entity_messageDynarr (gui->subs, this, "loseFocus", NULL);
}


void gui_update (Dynarr entities)
{
	EntSpeech
		message;

	while ((message = entitySystem_dequeueMessage ("gui")))
	{
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

bool gui_inside (Entity this, int x, int y)
{
	GUI
		gui = component_getData (entity_getAs (this, "gui"));
	if (!gui)
		return false;
	if (x >= gui->x && x < gui->x + gui->w
	 && y >= gui->y && y < gui->y + gui->h)
		return true;
	return false;
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

Entity gui_getFrame (Entity this)
{
	GUI
		gui = component_getData (entity_getAs (this, "gui"));
	if (!gui)
		return NULL;
	return gui->frame;
}

Dynarr gui_getSubs (Entity this)
{
	GUI
		gui = component_getData (entity_getAs (this, "gui"));
	if (!gui)
		return NULL;
	return gui->subs;
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
