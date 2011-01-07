#include "beyond.h"

int main (int argc, char * argv[])
{
	SYSTEM
		* sys = NULL;

	objClass_init (system_handler, NULL, NULL, NULL);
	sys = obj_getClassData (SystemObject, "SYSTEM");

	obj_message (SystemObject, OM_START, NULL, NULL);

	while (!sys->quit)
	{
		obj_message (SystemObject, OM_UPDATE, NULL, NULL);

    obj_messagePre (VideoObject, OM_PRERENDER, NULL, NULL);
    obj_messagePre (WorldObject, OM_PRERENDER, NULL, NULL);
    obj_messagePre (WorldObject, OM_RENDER, NULL, NULL);
    obj_messagePre (WorldObject, OM_POSTRENDER, NULL, NULL);
    obj_messagePre (VideoObject, OM_POSTRENDER, NULL, NULL);
  }

  obj_message (SystemObject, OM_DESTROY, NULL, NULL);
  objClass_destroy ("SYSTEM");
  objects_destroyEverything ();
  return 0;
}