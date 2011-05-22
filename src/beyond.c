#include "video.h"
#include "object.h"
#include "entity.h"
#include "system.h"

int main (int argc, char * argv[])
{
	SYSTEM
		* sys = NULL;
	logSetLevel (E_ALL & ~E_FUNCLABEL);
	objPassEnable (FALSE);
	objClass_init (system_handler, NULL, NULL, NULL);
	sys = obj_getClassData (SystemObject, "SYSTEM");
	obj_message (SystemObject, OM_START, NULL, NULL);
	while (!sys->quit)
	{
		obj_message (SystemObject, OM_UPDATE, NULL, NULL);
		obj_messagePre (VideoObject, OM_PRERENDER, NULL, NULL);
		systemRender ();
		obj_messagePre (VideoObject, OM_POSTRENDER, NULL, NULL);
	}
	obj_message (SystemObject, OM_DESTROY, NULL, NULL);
	objClass_destroy ("SYSTEM");
	objects_destroyEverything ();
	return 0;
}
