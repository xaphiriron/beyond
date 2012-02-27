/* beyond (or whatever it's going to eventually be called) game thing
 * copyright 2012 xax
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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

static void start_new_game (Entity menu_item);
static void create_options_menu (Entity menu_item);

static void blah_blah_GPL_whatever (Entity about);

int main (int argc, char * argv[])
{
	int
		r;
	logSetLevel (E_ALL & ~(E_FUNCLABEL | E_DEBUG | E_INFO));

	xph_findBaseDir (argv[0]);

	systemInit ();
	// this is after the system init because it depends on opengl being started to make the textures it needs
	fontLoad ("data/FreeSans.ttf", 16);
	systemLoad (bootstrap, load, finalize);

	r = systemLoop ();

	systemDestroy ();
	fontUnload ();

	return r;
}


static void bootstrap (void)
{
	/* this is where any loading that's absolutely needed from the first frame
	 * onwards should go -- sdl initialization, font loading, loading screen
	 * effects, etc
	 *  - xph 2011 09 26
	 */

	component_register ("position", position_define);
	component_register ("camera", camera_define);
	component_register ("walking", walking_define);
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

static void finalize (void)
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
	menu_addItem (titleScreenMenu, "New Game", IR_NOTHING, start_new_game);
	// options currently can't be altered at realtime due to 1. the lack of usable UI interface; 2. the lack of option-saving code; 3. probably some other things too - xph 02 14 2012
	menu_addItem (titleScreenMenu, "Options", IR_NOTHING, create_options_menu);
	menu_addItem (titleScreenMenu, "Quit", IR_QUIT, NULL);

	menu_addPositionedItem (titleScreenMenu, width - 32, height - 32, 32, 32, "?", IR_NOTHING, blah_blah_GPL_whatever);

	entity_message (titleScreenMenu, NULL, "gainFocus", NULL);

	systemClearStates();
	systemPushState (STATE_UI);
}

static void load (TIMER * t)
{
	/* this is where any loading that's needed for the system ui or for title
	 * screen effects should go
	 *  - xph 2011 09 26
	 */

	loadSetGoal (1);
	loadSetLoaded (1);
}

static void start_new_game (Entity menu_item)
{
	unsigned int
		width, height;
	Entity
		worldOptions;

	video_getDimensions (&width, &height);

	worldOptions = entity_create ();
	component_instantiate ("gui", worldOptions);
	gui_place (worldOptions, width/4, 0, width/2, height);
	gui_setMargin (worldOptions, 8, 8);
	component_instantiate ("optlayout", worldOptions);
	component_instantiate ("input", worldOptions);
	entity_refresh (worldOptions);

	optlayout_confirm (worldOptions, worldConfig);
	gui_placeOnStack (worldOptions);

	entity_message (worldOptions, NULL, "gainFocus", NULL);

	optlayout_addOption (worldOptions, "World Size", OPT_NUM, "3", NULL);
	optlayout_addOption (worldOptions, "Seed", OPT_STRING, "", NULL);
	optlayout_addOption (worldOptions, "Pattern Data", OPT_STRING, "data/patterns", "File to get generation rules from");
}

static void create_options_menu (Entity menu_item)
{
	unsigned int
		width, height;
	Entity
		gameOptions;

	video_getDimensions (&width, &height);

	gameOptions = entity_create ();
	component_instantiate ("gui", gameOptions);
	gui_place (gameOptions, width/4, 0, width/2, height);
	gui_setMargin (gameOptions, 8, 8);
	component_instantiate ("optlayout", gameOptions);
	component_instantiate ("input", gameOptions);
	entity_refresh (gameOptions);

	optlayout_confirm (gameOptions, NULL); // TODO: write the config-updating function and put it here
	gui_placeOnStack (gameOptions);

	entity_message (gameOptions, NULL, "gainFocus", NULL);

	// TODO: this needs a way of selecting the currently-active options as the default, presumably through reading System->config (so like make the code that establishes this entity its own function)
	// TODO: need optlayout dropdown box + "Custom..." w/ two numeric fields for this; that implies a lot of mucking about with optlayout code and the way options are defined within it
	optlayout_addOption (gameOptions, "Screen Resolution", OPT_STRING, "800x600", NULL);
	optlayout_addOption (gameOptions, "Fullscreen", OPT_FLAG, "off", NULL);

	// TODO: this also needs /panes/; so video options and key options and game options aren't all thrown together on the same screen :(
}


static void blah_blah_GPL_whatever (Entity about)
{
	Entity
		gpl;
	unsigned int
		height,
		width;

	video_getDimensions (&width, &height);
	gpl = entity_create ();
	component_instantiate ("gui", gpl);
	component_instantiate ("input", gpl);
	component_instantiate ("menu", gpl);
	component_instantiate ("textlabel", gpl);
	entity_refresh (gpl);

	gui_place (gpl, 16, 16, width - 32, height - 32);
	input_addAction (gpl, IR_UI_CONFIRM, gui_defaultClose);
	input_addAction (gpl, IR_UI_CANCEL, gui_defaultClose);
	input_addAction (gpl, IR_TEXT, gui_defaultClose);
	textlabel_set (gpl, "STANDARD LEGALLY-OBLIGATORY GPL BOILERPLATE\nThis program is free software: you can redistribute it and/or modify it under the terms of the GNU\nGeneral Public License as published by the Free Software Foundation, either version 3 of the\nLicense, or (at your option) any later version.\n\nThis program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;\nwithout even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR\n PURPOSE.  See the GNU General Public License for more details.\n\nYou should have received a copy of the GNU General Public License along with this program.  If\nnot, see <http://www.gnu.org/licenses/>.\n\ncopyright 2012 xaphiriron, gj, go me\n\nwow i hate licensing boilerplate i'm sorry you had to read that", ALIGN_LEFT, 16, 16, width - 32);

	gui_placeOnStack (gpl);
	
}
