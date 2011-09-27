#include "video.h"
#include "object.h"
#include "entity.h"
#include "system.h"

#include "font.h"
#include "path.h"

int main (int argc, char * argv[])
{
	logSetLevel (E_ALL & ~(E_FUNCLABEL | E_DEBUG | E_INFO));

	setSystemPath (argv[0]);
	objPassEnable (FALSE);
	system_message (OM_CLSINIT, NULL, NULL);
	systemLoad (systemBootstrapInit, systemBootstrap, systemBootstrapFinalize);
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
