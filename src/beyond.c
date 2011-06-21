#include "video.h"
#include "object.h"
#include "entity.h"
#include "system.h"

#include "font.h"
#include "path.h"

#include <time.h>

int main (int argc, char * argv[])
{
	logSetLevel (E_ALL & ~(E_FUNCLABEL | E_DEBUG));

	setSystemPath (argv[0]);
	objPassEnable (FALSE);
	system_message (OM_CLSINIT, NULL, NULL);
	systemStart ();
	srand (time (NULL));
	while (!System->quit)
	{
		systemUpdate ();
		systemRender ();
	}
	system_message (OM_DESTROY, NULL, NULL);
	system_message (OM_CLSFREE, NULL, NULL);
	objects_destroyEverything ();
	return 0;
}
