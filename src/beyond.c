#include "beyond.h"

void render ();

int main (int argc, char * argv[])
{
	SYSTEM
		* sys = NULL;
	logSetLevel (E_ALL);
	objPassEnable (FALSE);
	objClass_init (system_handler, NULL, NULL, NULL);
	sys = obj_getClassData (SystemObject, "SYSTEM");
	obj_message (SystemObject, OM_START, NULL, NULL);

	while (!sys->quit)
	{
		obj_message (SystemObject, OM_UPDATE, NULL, NULL);

		obj_messagePre (VideoObject, OM_PRERENDER, NULL, NULL);
		render ();
		obj_messagePre (VideoObject, OM_POSTRENDER, NULL, NULL);
	}
	obj_message (SystemObject, OM_DESTROY, NULL, NULL);
	objClass_destroy ("SYSTEM");
	objects_destroyEverything ();
	return 0;
}

void render ()
{
	SYSTEM
		* sys = obj_getClassData (SystemObject, "SYSTEM");
	Entity
		camera;
	const float
		* matrix;
	camera = camera_getActiveCamera ();
	// THIS IS FROM WORLD:PRERENDER
	matrix = camera_getMatrix (camera);
	//glPushMatrix ();
	if (matrix == NULL)
		glLoadIdentity ();
	else
		glLoadMatrixf (matrix);

	// THIS IS FROM WORLD:RENDER
	ground_draw_fill (camera);
	if (system_getState (sys) == STATE_FIRSTPERSONVIEW)
		camera_drawCursor ();

	// THIS IS FROM WORLD:POSTRENDER
	//glPopMatrix ();
}
