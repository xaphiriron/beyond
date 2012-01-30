#include "comp_gui.h"

#include "video.h"
#include "component_input.h"

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

static void gui_create (EntComponent comp, EntSpeech speech);
static void gui_destroy (EntComponent comp, EntSpeech speech);
static void gui_input (EntComponent comp, EntSpeech speech);

void gui_define (EntComponent comp, EntSpeech speech)
{
	component_registerResponse ("gui", "__create", gui_create);
	component_registerResponse ("gui", "__destroy", gui_destroy);

	component_registerResponse ("gui", "FOCUS_INPUT", gui_input);
}

static void gui_create (EntComponent comp, EntSpeech speech)
{
	GUI
		gui = xph_alloc (sizeof (struct xph_gui));
	gui->confirmCallback = gui_defaultCallback;
	gui->cancelCallback = gui_defaultCallback;

	component_setData (comp, gui);
}

static void gui_destroy (EntComponent comp, EntSpeech speech)
{
	GUI
		gui = component_getData (comp);

	xph_free (gui);

	component_clearData (comp);
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
		default:
			break;
	}
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

	while ((active = *(Entity *)dynarr_at (entities, i++)))
	{
		entity_message (active, NULL, "guiDraw", NULL);
	}

	glEnable (GL_DEPTH_TEST);
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
