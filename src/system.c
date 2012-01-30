#include "system.h"

#include <ctype.h>

#include "video.h"
#include "xph_timer.h"

#include <GL/glpng.h>
#include "texture.h"
#include "font.h"

#include "worldgen.h"

#include "component_input.h"
#include "component_ui.h"
#include "component_camera.h"
#include "component_position.h"

#define CONFIGPATH "../data/settings"

#define LOADERTEXTBUFFERSIZE 128
struct loadingdata
{
	void (*finishCallback)(void);
	void (*loaderFunc)(TIMER *);
	unsigned int
		goal,
		loaded;
	float
		percentage;
	char
		displayText[LOADERTEXTBUFFERSIZE];
};

#define DEBUGLEN	256
char
	debugDisplay[DEBUGLEN];

static struct loadingdata
	SysLoader;

SYSTEM
	* System = NULL;

void systemUpdate (void);

void systemInit ()
{
	TIMER
		* t;
	if (System != NULL)
		return;

	System = xph_alloc (sizeof (SYSTEM));

	System->quit = false;

	System->timer_mult = 1.0;
	System->timestep = 30;
	t = timerCreate ();
	timerSetScale (t, System->timer_mult);
	System->acc = accumulator_create (t, System->timestep);
	// this ought to force the update code to run once the first time through - xph 2011 06 17
	System->acc->accumulated = System->timestep + 1;

	System->state = dynarr_create (4, sizeof (enum system_states));
	dynarr_push (System->state, STATE_LOADING);
	// register system loading %/note callback here

	System->updateFuncs = dynarr_create (2, sizeof (void (*)(TIMER *)));

	// not really sure where these should go; they're going here for now.
	system_registerTimedFunction (entity_purgeDestroyed, 0x7f);

	System->config = Ogdl_load (absolutePath (CONFIGPATH));

#ifdef MEM_DEBUG
	atexit (xph_audit);
#endif /* MEM_DEBUG */

	videoInit ();

	return;
}

void systemDestroy ()
{
	videoDestroy ();

	Graph_free (System->config);

	dynarr_destroy (System->updateFuncs);
	dynarr_destroy (System->state);
	accumulator_destroy (System->acc);
	timerDestroyRegistry ();
	xph_free (System);

	System = NULL;

	entity_destroyEverything ();
}

int systemLoop ()
{
	while (!System->quit)
	{
		systemUpdate ();
	}
	return 0;
}

static unsigned long
	startTime,
	finishTime;
void tickBegin ()
{
	startTime = SDL_GetTicks ();
}

void tickSleep ()
{
	unsigned long
		timeTaken;
	finishTime = SDL_GetTicks ();
	timeTaken = finishTime - startTime;
	if (timeTaken <= System->timestep)
		SDL_Delay (System->timestep - timeTaken);
	else
	{
		//WARNING ("processing tick took %lu milliseconds; %lu more than the tick guideline (%lu)", timeTaken, timeTaken - System->timestep, System->timestep);
		SDL_Delay (0);
	}
}

void systemUpdate (void)
{
	static TIMER
		* t = NULL;
	void (*func)(TIMER *);
	int
		i;
	FUNCOPEN ();

	if (System == NULL)
		return;
	timerUpdateRegistry ();
	accumulator_update (System->acc);

	if (t == NULL)
		t = timerCreate ();

	tickBegin ();
	while (accumulator_withdrawlTime (System->acc))
	{
		i = 0;
		while ((func = *(void (**)(TIMER *))dynarr_at (System->updateFuncs, i)) != NULL)
		{
			// FIXME: this is the amount of time in seconds to give each function. it ought to 1) be related to the accumulator delta and 2) be divided between update funcs by their priority (which is the second arg of the register function, but it's ignored completely right now)
			timerSetGoal (t, 0.05);
			timerUpdate (t);
			func (t);
			i++;
		}
		entitySystem_update ("mapLoad");
		entitySystem_update ("input");

		entitySystem_update ("walking");
		entitySystem_update ("builder");
		entitySystem_update ("plantUpdate");
		entitySystem_update ("chaser");

		entitySystem_update ("ui");
		entitySystem_update ("gui");
	}

	videoPrerender ();
	entitySystem_update ("TEMPsystemRender");
	entitySystem_update ("bodyRender");
	entitySystem_update ("plantRender");

	// NOTE: this system can in some cases reset the viewmatrix; if you want to draw something in 3d space then have it run before this system. also given the complexity of rendering systems and highlighting and overlays and all that it might be a good idea to generalize the "set the viewmatrix to the in-the-world value" code so it doesn't have to depend on execution order any more than absolutely necessary (e.g., when doing blending) - xph 2011 12 22
	entitySystem_update ("cameraRender");

	entitySystem_update ("uiRender");
	entitySystem_update ("guiRender");
	videoPostrender ();
	tickSleep ();

	FUNCCLOSE ();
}


const TIMER * system_getTimer ()
{
	if (System == NULL)
		return NULL;
	return System->acc->timer;
}

/***
 * SYSTEM STATE
 */

enum system_states systemState ()
{
	enum system_states
		state;
	if (System == NULL)
		return STATE_ERROR;
	state = *(enum system_states *)dynarr_back (System->state);
	return state;
}

bool systemPushState (enum system_states state)
{
	if (System == NULL)
		return false;
	if (*(enum system_states *)dynarr_back (System->state) == STATE_FREEVIEW)
		SDL_ShowCursor (SDL_ENABLE);
	dynarr_push (System->state, state);
	if (state == STATE_FREEVIEW)
		SDL_ShowCursor (SDL_DISABLE);
	return true;
}

enum system_states systemPopState ()
{
	enum system_states
		state;
	if (System == NULL)
		return STATE_ERROR;
	dynarr_pop (System->state);
	state = *(enum system_states *)dynarr_back (System->state);
	if (state == STATE_FREEVIEW)
		SDL_ShowCursor (SDL_DISABLE);
	else
		SDL_ShowCursor (SDL_ENABLE);
	return state;
}

bool systemClearStates ()
{
	if (System == NULL)
		return false;
	dynarr_wipe (System->state, NULL);
	return true;
}

/***
 * SYSTEM LOADING
 */

static void systemCheckLoadState (TIMER * t);

void systemLoad (void (*initialize)(void), void (*loader)(TIMER *), void (*finishCallback)(void))
{
	systemClearStates ();
	systemPushState (STATE_LOADING);
	memset (&SysLoader, 0, sizeof (struct loadingdata));

	system_registerTimedFunction (loader, 255);
	system_registerTimedFunction (systemCheckLoadState, 1);
	SysLoader.finishCallback = finishCallback;
	SysLoader.loaderFunc = loader;
	initialize ();
}

static void systemCheckLoadState (TIMER * t)
{
	FUNCOPEN ();
	if (SysLoader.loaded < SysLoader.goal)
		return;

	system_removeTimedFunction (SysLoader.loaderFunc);
	system_removeTimedFunction (systemCheckLoadState);

	systemClearStates ();
	SysLoader.finishCallback ();
	FUNCCLOSE ();
}

void loadSetGoal (unsigned int goal)
{
	if (systemState () != STATE_LOADING)
	{
		WARNING ("Loader function called outside of loading context?!");
		return;
	}
	SysLoader.goal = goal;
}

void loadSetLoaded (unsigned int loaded)
{
	if (systemState () != STATE_LOADING)
	{
		WARNING ("Loader function called outside of loading context?!");
		return;
	}
	SysLoader.loaded = loaded;
	SysLoader.percentage = loaded / SysLoader.goal * 100;
}

void loadSetText (char * displayText)
{
	if (systemState () != STATE_LOADING)
	{
		WARNING ("Loader function called outside of loading context?!");
		return;
	}
	strncpy (SysLoader.displayText, displayText, LOADERTEXTBUFFERSIZE);
}


/***
 * TIMED FUNCTIONS
 */

void system_registerTimedFunction (void (*func)(TIMER *), unsigned char weight)
{
	if (System == NULL)
		return;
	dynarr_push (System->updateFuncs, func);
}

void system_removeTimedFunction (void (*func)(TIMER *))
{
	if (System == NULL)
		return;
	dynarr_remove_condense (System->updateFuncs, func);
}



#include "map_internal.h"
void systemCreatePlayer (Entity base)
{
	Entity
		player,
		camera,
		worldmap;
	SUBHEX
		place;

	FUNCOPEN ();

	player = entity_create ();
	if (component_instantiate ("input", player)) {
		input_addEntity (player, INPUT_CONTROLLED);
		entity_message (player, NULL, "gainFocus", NULL);
	}
	if (component_instantiate ("position", player))
	{
		/* NOTE: this is a placeholder; player positioning in the generated world is a worldgen thing, not a system thing. maybe this entire function is misguided, idk.
		 *  - xph 2011 06 09
		 */
		place = map_posFocusedPlatter (position_get (base));
		worldSetRenderCacheCentre (place);

		place = subhexData (place, 0, 0);
		assert (place && subhexSpanLevel (place) == 0);
		position_placeOnHexStep (player, &place->hex, hexGroundStepNear (&place->hex, 0));
	}
	component_instantiate ("walking", player);
	component_instantiate ("body", player);
	component_instantiate ("builder", player);
	component_instantiate ("player", player);
	entity_refresh (player);
	entity_name (player, "PLAYER");

	camera = entity_create ();
	component_instantiate ("position", camera);
	if (component_instantiate ("camera", camera))
	{
		entity_message (camera, NULL, "setTarget", player);
		entity_message (camera, NULL, "activate", NULL);
	}
	if (component_instantiate ("input", camera))
	{
		input_addEntity (camera, INPUT_CONTROLLED);
		entity_message (camera, NULL, "gainFocus", NULL);
	}

	entity_refresh (camera);
	entity_name (camera, "CAMERA");

	entity_addToGroup (player, "PlayerAvatarEntities");
	entity_addToGroup (camera, "PlayerAvatarEntities");

	worldmap = entity_create ();
	if (component_instantiate ("input", worldmap))
	{
		input_addEntity (worldmap, INPUT_CONTROLLED);
	}
	component_instantiate ("gui", worldmap);
	component_instantiate ("worldmap", worldmap);
	entity_refresh (worldmap);
	entity_name (worldmap, "WORLDMAP");

	entity_addToGroup (worldmap, "GUI");

	FUNCCLOSE ();
}

/***
 * RENDERING
 */

static void renderSkyCube ();

void systemRender (Dynarr entities)
{
	Entity
		camera;
	const float
		* matrix;
	float
		fakeMatrix[16];
	unsigned int
		width,
		height;
	char
		loadStr[64];
	float
		zNear = video_getZnear () - 0.001;

	if (System == NULL)
		return;
	video_getDimensions (&width, &height);
	if (systemState (System) == STATE_LOADING)
	{
		glDisable (GL_DEPTH_TEST);
		glColor3f (0.0, 0.0, 0.0);

		glBegin (GL_QUADS);
		glVertex3f (video_pixelXMap (0), video_pixelYMap (0), zNear);
		glVertex3f (video_pixelXMap (0), video_pixelYMap (height), zNear);
		glVertex3f (video_pixelXMap (width), video_pixelYMap (height), zNear);
		glVertex3f (video_pixelXMap (width), video_pixelYMap (0), zNear);
		glEnd ();
		glColor3f (1.0, 1.0, 1.0);
		fontPrint ("loading...", width/2, height/2);

		snprintf (loadStr, 64, "(%d / %d, %.2f%%)", SysLoader.loaded, SysLoader.goal, SysLoader.percentage);

		fontPrint (loadStr, width/2, height/2 + 32);
		fontPrint (SysLoader.displayText, width/2, height/1.5);

		glEnable (GL_DEPTH_TEST);
		return;
	}

	if (worldExists ())
	{
		camera = entity_getByName ("CAMERA");
		entity_message (camera, NULL, "getMatrix", &matrix);
		if (matrix != NULL)
		{
			memcpy (fakeMatrix, matrix, sizeof (float) * 16);
			fakeMatrix[12] = fakeMatrix[13] = fakeMatrix[14] = 0.0;
			glLoadMatrixf (fakeMatrix);
		}

		renderSkyCube ();

		if (matrix != NULL)
			glLoadMatrixf (matrix);

		mapDraw (matrix);

	}

}

static void renderSkyCube ()
{
	float
		zNear = video_getZnear () * 2.0;

	glBindTexture (GL_TEXTURE_2D, 0);
	glDisable (GL_DEPTH_TEST);
	glBegin (GL_QUADS);

	glColor3ub (0x1f, 0x0f, 0x2f);
	glVertex3f ( 1.0 * zNear,  1.0 * zNear,  1.0 * zNear);
	glVertex3f ( 1.0 * zNear, -1.0 * zNear,  1.0 * zNear);
	glVertex3f ( 1.0 * zNear, -1.0 * zNear, -1.0 * zNear);
	glVertex3f ( 1.0 * zNear,  1.0 * zNear, -1.0 * zNear);

	glVertex3f (-1.0 * zNear,  1.0 * zNear, -1.0 * zNear);
	glVertex3f (-1.0 * zNear,  1.0 * zNear,  1.0 * zNear);
	glVertex3f ( 1.0 * zNear,  1.0 * zNear,  1.0 * zNear);
	glVertex3f ( 1.0 * zNear,  1.0 * zNear, -1.0 * zNear);

	glVertex3f (-1.0 * zNear,  1.0 * zNear,  1.0 * zNear);
	glVertex3f (-1.0 * zNear, -1.0 * zNear,  1.0 * zNear);
	glVertex3f ( 1.0 * zNear, -1.0 * zNear,  1.0 * zNear);
	glVertex3f ( 1.0 * zNear,  1.0 * zNear,  1.0 * zNear);

	glVertex3f (-1.0 * zNear,  1.0 * zNear, -1.0 * zNear);
	glVertex3f (-1.0 * zNear, -1.0 * zNear, -1.0 * zNear);
	glVertex3f (-1.0 * zNear, -1.0 * zNear,  1.0 * zNear);
	glVertex3f (-1.0 * zNear,  1.0 * zNear,  1.0 * zNear);

	glVertex3f ( 1.0 * zNear, -1.0 * zNear, -1.0 * zNear);
	glVertex3f ( 1.0 * zNear, -1.0 * zNear,  1.0 * zNear);
	glVertex3f (-1.0 * zNear, -1.0 * zNear,  1.0 * zNear);
	glVertex3f (-1.0 * zNear, -1.0 * zNear, -1.0 * zNear);

	glVertex3f ( 1.0 * zNear,  1.0 * zNear, -1.0 * zNear);
	glVertex3f ( 1.0 * zNear, -1.0 * zNear, -1.0 * zNear);
	glVertex3f (-1.0 * zNear, -1.0 * zNear, -1.0 * zNear);
	glVertex3f (-1.0 * zNear,  1.0 * zNear, -1.0 * zNear);
	glEnd ();
	glEnable (GL_DEPTH_TEST);
	glClear (GL_DEPTH_BUFFER_BIT);
}


char * systemGenDebugStr ()
{
	signed int
		x = 0,
		y = 0,
		len = 0,
		span = mapGetSpan (),
		radius = mapGetRadius ();
	unsigned int
		height = 0;
	Entity
		player = entity_getByName ("PLAYER"),
		camera = entity_getByName ("CAMERA");
	hexPos
		position = position_get (player);
	SUBHEX
		platter;
	unsigned char
		focus = hexPos_focus (position),
		i = 0;
	char
		buffer[64];
	
	memset (debugDisplay, 0, DEBUGLEN);
	len = snprintf (buffer, 63, "Player Entity: #%d\nCamera Entity: #%d\n\n", entity_GUID (player), entity_GUID (camera));
	strncpy (debugDisplay, buffer, len);


	len += snprintf (buffer, 63, "scale: %d,%d\n\ton %c:\n", span, radius, toupper (subhexPoleName (hexPos_platter (position, span))));
	strncat (debugDisplay, buffer, DEBUGLEN - len);

	i = span;
	while (i > 0)
	{
		i--;
		if (i < focus)
		{
			len += snprintf (buffer, 63, "\t%d: **, **\n", i);
			strncat (debugDisplay, buffer, DEBUGLEN - len);
			continue;
		}
		platter = hexPos_platter (position, i);
		subhexLocalCoordinates (platter, &x, &y);
		len += snprintf (buffer, 63, "\t%d: %d, %d\n", i, x, y);
		strncat (debugDisplay, buffer, DEBUGLEN - len);
	}
	height = position_height (player);
	len += snprintf (buffer, 63, "\nheight: %u\n", height);
	strncat (debugDisplay, buffer, DEBUGLEN - len);

	return debugDisplay;
}

/***
 * "MESSAGER"
 */

// look let's just awknowledge this function is an abomination that should not exist and realize that certain workarounds are required until the code is updated to note require it at all :( - xph 2012 01 27
#include "comp_optlayout.h"
#include "comp_gui.h"

int system_message (objMsg msg, void * a, void * b)
{
	Dynarr
		arr;

	Entity
		worldOptions;
	unsigned int
		width, height;

	switch (msg)
	{
		case OM_SHUTDOWN:
			System->quit = true;
			systemPushState (STATE_QUIT);
			return EXIT_SUCCESS;

		case OM_FORCEWORLDGEN:

			worldOptions = entity_create ();
			component_instantiate ("gui", worldOptions);
			component_instantiate ("optlayout", worldOptions);
			component_instantiate ("input", worldOptions);
			entity_refresh (worldOptions);

			video_getDimensions (&width, &height);
			gui_place (worldOptions, width/4, 0, width/2, height);
			gui_setMargin (worldOptions, 8, 8);
			gui_confirmCallback (worldOptions, worldConfig);

			input_addEntity (worldOptions, INPUT_FOCUSED);
			entity_message (worldOptions, NULL, "gainFocus", NULL);

			optlayout_addOption (worldOptions, "World Size", OPT_NUM, "4", NULL);
			optlayout_addOption (worldOptions, "Seed", OPT_STRING, "", NULL);
			optlayout_addOption (worldOptions, "Pattern Data", OPT_STRING, "data/patterns", "Generation rules to apply");

			return EXIT_SUCCESS;

			arr = entity_getWith (1, "ui");
			dynarr_map (arr, (void (*)(void *))entity_destroy);
			dynarr_destroy (arr);

			return EXIT_SUCCESS;

		default:
			// no passing. no reason to.
			break;
	}
	return EXIT_FAILURE;
}