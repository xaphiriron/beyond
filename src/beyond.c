#include <time.h>

#include "video.h"
#include "entity.h"
#include "system.h"

#include "font.h"
#include "xph_path.h"

#include "component_input.h"
#include "component_camera.h"
#include "component_walking.h"

#include "comp_arch.h"
#include "comp_body.h"
#include "comp_plant.h"
#include "comp_chaser.h"
#include "comp_gui.h"
#include "comp_worldmap.h"
#include "comp_player.h"
#include "comp_optlayout.h"
#include "comp_clickable.h"
#include "comp_textlabel.h"
#include "comp_menu.h"

static void bootstrap (void);
static void load (TIMER * t);
static void finalize (void);


int main (int argc, char * argv[])
{
	int
		r;
	logSetLevel (E_ALL & ~(E_FUNCLABEL | E_DEBUG | E_INFO));

	setSystemPath (argv[0]);
	systemInit ();
	// this is after the system init because it depends on opengl being started to make the textures it needs
	fontLoad ("../data/FreeSans.ttf", 16);
	systemLoad (bootstrap, load, finalize);

	r = systemLoop ();

	systemDestroy ();
	fontUnload ();

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
	component_register ("arch", arch_define);
	component_register ("body", body_define);
	component_register ("builder", builder_define);
	component_register ("plant", plant_define);
	component_register ("chaser", chaser_define);
	component_register ("gui", gui_define);
	component_register ("worldmap", worldmap_define);
	component_register ("player", player_define);
	component_register ("optlayout", optlayout_define);
	component_register ("clickable", clickable_define);
	component_register ("textlabel", textlabel_define);
	component_register ("menu", menu_define);

	entitySystem_register ("input", input_system, 1, "input");
	entitySystem_register ("walking", walking_system, 1, "walking");
	entitySystem_register ("builder", builder_system, 1, "builder");
	entitySystem_register ("plantUpdate", plantUpdate_system, 1, "plant");
	entitySystem_register ("chaser", chaser_update, 1, "chaser");
	entitySystem_register ("gui", gui_update, 1, "gui");

	entitySystem_register ("mapLoad", mapLoad_system, 0);

	entitySystem_register ("guiRender", gui_render, 1, "gui");
	entitySystem_disableMessages ("guiRender");
	entitySystem_register ("bodyRender", bodyRender_system, 2, "body", "position");
	entitySystem_disableMessages ("bodyRender");
	entitySystem_register ("cameraRender", cameraRender_system, 1, "camera");
	entitySystem_disableMessages ("cameraRender");
	entitySystem_register ("plantRender", plantRender_system, 1, "plant");
	entitySystem_disableMessages ("plantRender");

	entitySystem_register ("TEMPsystemRender", systemRender, 0);
	entitySystem_disableMessages ("TEMPsystemRender");

	loadSetText ("Loading...");
}

void finalize (void)
{
	Entity
		titleScreenMenu;
	unsigned int
		height,
		width;

	video_getDimensions (&width, &height);
	titleScreenMenu = entity_create ();
	component_instantiate ("gui", titleScreenMenu);
	component_instantiate ("input", titleScreenMenu);
	component_instantiate ("menu", titleScreenMenu);
	entity_refresh (titleScreenMenu);
	gui_placeOnStack (titleScreenMenu);
	gui_place (titleScreenMenu, width / 4, height / 2, width / 2, height / 4);
	gui_setMargin (titleScreenMenu, 4, 4);
	menu_addItem (titleScreenMenu, "New Game", IR_WORLDGEN);
	// options currently can't be altered at realtime due to 1. the lack of usable UI interface; 2. the lack of option-saving code; 3. probably some other things too - xph 02 14 2012
	//menu_addItem (titleScreenMenu, "Options", IR_OPTIONS);
	menu_addItem (titleScreenMenu, "Quit", IR_QUIT);

	entity_message (titleScreenMenu, NULL, "gainFocus", NULL);

	systemClearStates();
	systemPushState (STATE_UI);
}

void load (TIMER * t)
{
	/* this is where any loading that's needed for the system ui or for title
	 * screen effects should go
	 *  - xph 2011 09 26
	 */

	loadSetGoal (1);
	loadSetLoaded (1);
}