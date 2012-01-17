#include "comp_gui.h"

#include "video.h"

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


static void gui_create (EntComponent comp, EntSpeech speech)
{
	GUI
		gui = xph_alloc (sizeof (struct xph_gui));

	component_setData (comp, gui);
}

static void gui_destroy (EntComponent comp, EntSpeech speech)
{
	GUI
		gui = component_getData (comp);
	xph_free (gui);

	component_clearData (comp);
}

void gui_define (EntComponent comp, EntSpeech speech)
{
	component_registerResponse ("gui", "__create", gui_create);
	component_registerResponse ("gui", "__destroy", gui_destroy);

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
