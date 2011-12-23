#include <time.h>

#include "video.h"
#include "entity.h"
#include "system.h"

#include "font.h"
#include "path.h"

#include "component_input.h"
#include "component_ui.h"
#include "component_camera.h"
#include "component_walking.h"

#include "comp_arch.h"
#include "comp_body.h"

static void bootstrap (void);
static void load (TIMER t);
static void finalize (void);


int main (int argc, char * argv[])
{
	int
		r;
	logSetLevel (E_ALL & ~(E_FUNCLABEL | E_DEBUG | E_INFO));

	setSystemPath (argv[0]);
	systemInit ();
	systemLoad (bootstrap, load, finalize);

	r = systemLoop ();

	systemDestroy ();
	return r;
}


void bootstrap (void)
{
	/* this is where any loading that's absolutely needed from the first frame
	 * onwards should go -- sdl initialization, font loading, loading screen
	 * effects, etc
	 *  - xph 2011 09 26
	 */

	component_register ("position", position_define);
	component_register ("camera", camera_define);
	component_register ("walking", walking_define);
	component_register ("input", input_define);
	component_register ("ui", ui_define);
	component_register ("worldArch", worldarch_define);
	component_register ("pattern", pattern_define);
	component_register ("body", pattern_define);

	entitySystem_register ("input", input_system, 1, "input");
	entitySystem_register ("walking", walking_system, 1, "walking");
	entitySystem_register ("ui", ui_system, 1, "ui");

	entitySystem_register ("uiRender", uiRender_system, 1, "ui");
	entitySystem_disableMessages ("uiRender");
	entitySystem_register ("bodyRender", bodyRender_system, 2, "body", "position");
	entitySystem_disableMessages ("bodyRender");
	entitySystem_register ("cameraRender", cameraRender_system, 1, "camera");
	entitySystem_disableMessages ("cameraRender");

	entitySystem_register ("TEMPsystemRender", systemRender, 0);
	entitySystem_disableMessages ("TEMPsystemRender");

	loadFont ("../img/default.png");
	uiLoadPanelTexture ("../img/frame.png");
	loadSetText ("Loading...");
}

void finalize (void)
{
	Entity
		titleScreenMenu;
	unsigned int
		height,
		width;

	/* this is just a filler that ought to be set elsewhere and remembered */
	srand (time (NULL));

	video_getDimensions (&height, &width);
	titleScreenMenu = entity_create ();
	component_instantiate ("ui", titleScreenMenu);
	entity_message (titleScreenMenu, NULL, "setType", (void *)UI_MENU);
	component_instantiate ("input", titleScreenMenu);
	input_addEntity (titleScreenMenu, INPUT_FOCUSED);
	entity_refresh (titleScreenMenu);

	entity_message (titleScreenMenu, NULL, "addValue", "New Game");
	entity_message (titleScreenMenu, NULL, "setAction", (void *)IR_WORLDGEN);
	entity_message (titleScreenMenu, NULL, "addValue", "this is a test of the menu code and also the ui code and also aaaaah :(");
	entity_message (titleScreenMenu, NULL, "addValue", "second option");
	entity_message (titleScreenMenu, NULL, "addValue", "third option");
	entity_message (titleScreenMenu, NULL, "addValue", "blah blah blah etc");
	entity_message (titleScreenMenu, NULL, "addValue", "lorem ipsum dolor sit amet");
	entity_message (titleScreenMenu, NULL, "addValue", "Quit");
	entity_message (titleScreenMenu, NULL, "setAction", (void *)IR_QUIT);

	entity_message (titleScreenMenu, NULL, "setWidth", (void *)(int)(width / 4));
	entity_message (titleScreenMenu, NULL, "setPosType", (void *)(PANEL_X_CENTER | PANEL_Y_ALIGN_23));
	entity_message (titleScreenMenu, NULL, "setFrameBG", (void *)FRAMEBG_SOLID);
	entity_message (titleScreenMenu, NULL, "setBorder", (void *)6);
	entity_message (titleScreenMenu, NULL, "setLineSpacing", (void *)4);

	systemClearStates();
	systemPushState (STATE_UI);
}

void load (TIMER t)
{
	/* this is where any loading that's needed for the system ui or for title
	 * screen effects should go
	 *  - xph 2011 09 26
	 */

	loadSetGoal (1);
	loadSetLoaded (1);
}