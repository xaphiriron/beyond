#include "system.h"

#include "video.h"
#include "timer.h"

#include <GL/glpng.h>
#include "texture.h"
#include "font.h"

#include "worldgen.h"

#include "component_input.h"
#include "component_ui.h"
#include "component_camera.h"
#include "component_position.h"


#define LOADERTEXTBUFFERSIZE 128
struct loadingdata
{
	void (*finishCallback)(void);
	void (*loaderFunc)(TIMER);
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
void systemRender (void);

void systemInit ()
{
	TIMER
		t;
	if (System != NULL)
		return;

	System = xph_alloc (sizeof (SYSTEM));

	System->quit = false;
	System->debug = false;

	System->clock = clock_create ();
	System->timer_mult = 1.0;
	System->timestep = 0.03;
 	t = timerCreate ();
	timerSetClock (t, System->clock);
	timerSetScale (t, System->timer_mult);
	System->acc = accumulator_create (t, System->timestep);
	// this ought to force the update code to run once the first time through - xph 2011 06 17
	System->acc->accumulated = System->timestep * 1.01;

	System->state = dynarr_create (4, sizeof (enum system_states));
	dynarr_push (System->state, STATE_LOADING);
	// register system loading %/note callback here

	System->updateFuncs = dynarr_create (2, sizeof (void (*)(TIMER)));
	System->uiPanels = dynarr_create (2, sizeof (Entity));

	// not really sure where these should go; they're going here for now.
	system_registerTimedFunction (entity_purgeDestroyed, 0x7f);

#ifdef MEM_DEBUG
	atexit (xph_audit);
#endif /* MEM_DEBUG */

	videoInit ();

	return;
}

void systemDestroy ()
{
	videoDestroy ();

	dynarr_map (System->uiPanels, (void (*)(void *))entity_destroy);
	dynarr_destroy (System->uiPanels);
	dynarr_destroy (System->updateFuncs);
	dynarr_destroy (System->state);
	accumulator_destroy (System->acc);
	xtimerDestroyRegistry ();
	clock_destroy (System->clock);
	xph_free (System);

	System = NULL;

	entity_destroyEverything ();
}

int systemLoop ()
{
	while (!System->quit)
	{
		systemUpdate ();
		systemRender ();
	}
	return 0;
}


const TIMER system_getTimer ()
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

static void systemCheckLoadState (const TIMER t);

void systemLoad (void (*initialize)(void), void (*loader)(TIMER), void (*finishCallback)(void))
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

static void systemCheckLoadState (const TIMER t)
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
 * UI CONTROL FUNCTIONS
 */

Entity systemPopUI ()
{
	if (System == NULL)
		return NULL;
	return *(Entity *)dynarr_pop (System->uiPanels);
}

void systemPushUI (Entity p)
{
	if (System == NULL)
	{
		ERROR ("Trying to push UI (#%d) without a system", entity_GUID (p));
		return;
	}
	dynarr_push (System->uiPanels, p);
}

enum uiPanelTypes systemTopUIPanelType ()
{
	enum uiPanelTypes
		type = UI_NONE;
	if (System == NULL)
		return type;
	entity_message (*(Entity *)dynarr_back (System->uiPanels), NULL, "getType", &type);
	return type;
}

/***
 * TIMED FUNCTIONS
 */

void system_registerTimedFunction (void (*func)(TIMER), unsigned char weight)
{
	if (System == NULL)
		return;
	dynarr_push (System->updateFuncs, func);
}

void system_removeTimedFunction (void (*func)(TIMER))
{
	if (System == NULL)
		return;
	dynarr_remove_condense (System->updateFuncs, func);
}



void systemCreatePlayer (Entity base)
{
	Entity
		player,
		camera;

	FUNCOPEN ();

	player = entity_create ();
	if (component_instantiate ("input", player)) {
		input_addEntity (player, INPUT_CONTROLLED);
	}
	if (component_instantiate ("position", player))
	{
		/* NOTE: this is a placeholder; player positioning in the generated world is a worldgen thing, not a system thing. maybe this entire function is misguided, idk.
		 *  - xph 2011 06 09
		 */
		systemPlacePlayerAt (map_posFocusedPlatter (position_get (base)));
	}
	component_instantiate ("walking", player);
	entity_refresh (player);

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
	}
	entity_refresh (camera);

	FUNCCLOSE ();
}

void systemPlacePlayerAt (const SUBHEX subhex)
{
	FUNCOPEN ();
	Entity
		player = input_getPlayerEntity ();
	VECTOR3
		pos;
	worldSetRenderCacheCentre (subhex);
	pos = vectorCreate (0.0, subhexGetHeight (subhex) + 90.0, 0.0);
	position_setDirect (player, pos, subhex);
	FUNCCLOSE ();
}

/***
 * UPDATING
 */

void systemUpdate (void)
{
	static TIMER
		t = NULL;
	void (*func)(TIMER);
	int
		i;
	FUNCOPEN ();
	if (System == NULL)
		return;
	clock_update (System->clock);
	xtimerUpdateAll ();
	accumulator_update (System->acc);

	if (t == NULL)
		t = timerCreate ();

	timerSetClock (t, System->clock);
	while (accumulator_withdrawlTime (System->acc))
	{
		i = 0;
		while ((func = *(void (**)(TIMER))dynarr_at (System->updateFuncs, i)) != NULL)
		{
			//printf ("got func %p\n", func);
			// FIXME: this is the amount of time in seconds to give each function. it ought to 1) be related to the accumulator delta and 2) be divided between update funcs by their priority (which is the second arg of the register function, but it's ignored completely right now)
			timerSetGoal (t, 0.05);
			timerUpdate (t);
			func (t);
			i++;
		}
		entitySystem_update ("walking");
		entitySystem_updateAll ();
	}
	FUNCCLOSE ();
}

/***
 * RENDERING
 */

static void renderSkyCube ();

void systemRender (void)
{
	Entity
		camera;
	const float
		* matrix;
	float
		fakeMatrix[16];
	int
		i;
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
	videoPrerender ();
	if (systemState (System) == STATE_LOADING)
	{
		glDisable (GL_DEPTH_TEST);
		glColor3f (0.0, 0.0, 0.0);
		glBegin (GL_QUADS);
		glVertex3f (video_pixelXMap (0), video_pixelYMap (0), video_getZnear ());
		glVertex3f (video_pixelXMap (0), video_pixelYMap (height), video_getZnear ());
		glVertex3f (video_pixelXMap (width), video_pixelYMap (height), video_getZnear ());
		glVertex3f (video_pixelXMap (width), video_pixelYMap (0), video_getZnear ());

		glColor3f (0.0, 0.0, 1.0);
		glVertex3f (0, 0, zNear);
		glVertex3f (5, 0, zNear);
		glVertex3f (5, 5, zNear);
		glVertex3f (0, 5, zNear);

		glEnd ();
		glColor3f (1.0, 1.0, 1.0);
		drawLine ("loading...", width/2, height/2);

		
		snprintf (loadStr, 64, "(%d / %d, %.2f%%)", SysLoader.loaded, SysLoader.goal, SysLoader.percentage);

		drawLine (loadStr, width/2, height/2 + 32);
		drawLine (SysLoader.displayText, width/2, height/1.5);

		glEnable (GL_DEPTH_TEST);
		videoPostrender ();
		return;
	}

	if (worldExists ())
	{
		camera = camera_getActiveCamera ();
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


		if (systemState (System) == STATE_FREEVIEW && camera_getMode (camera) == CAMERA_FIRST_PERSON)
		{
			glLoadIdentity ();
			uiDrawCursor ();
		}
	}

	if (dynarr_size (System->uiPanels))
	{
		glLoadIdentity ();
		glDisable (GL_DEPTH_TEST);
		i = 0;
		while (i < dynarr_size (System->uiPanels))
		{
			// draw ui stuff
			entity_message (*(Entity *)dynarr_at (System->uiPanels, i), NULL, "draw", NULL);
			i++;
		}
		glEnable (GL_DEPTH_TEST);
	}
	glEnable (GL_DEPTH_TEST);
	videoPostrender ();
}

static void renderSkyCube ()
{
	float
		zNear = video_getZnear () * 2.0;

	glBindTexture (GL_TEXTURE_2D, 0);
	glDisable (GL_DEPTH_TEST);
	glBegin (GL_QUADS);

	glColor3ub (0xff, 0x00, 0x00);
	glVertex3f ( 1.0 * zNear,  1.0 * zNear,  1.0 * zNear);
	glVertex3f ( 1.0 * zNear, -1.0 * zNear,  1.0 * zNear);
	glVertex3f ( 1.0 * zNear, -1.0 * zNear, -1.0 * zNear);
	glVertex3f ( 1.0 * zNear,  1.0 * zNear, -1.0 * zNear);

	glColor3ub (0x00, 0xff, 0x00);
	glVertex3f (-1.0 * zNear,  1.0 * zNear, -1.0 * zNear);
	glVertex3f (-1.0 * zNear,  1.0 * zNear,  1.0 * zNear);
	glVertex3f ( 1.0 * zNear,  1.0 * zNear,  1.0 * zNear);
	glVertex3f ( 1.0 * zNear,  1.0 * zNear, -1.0 * zNear);

	glColor3ub (0x00, 0x00, 0xff);
	glVertex3f (-1.0 * zNear,  1.0 * zNear,  1.0 * zNear);
	glVertex3f (-1.0 * zNear, -1.0 * zNear,  1.0 * zNear);
	glVertex3f ( 1.0 * zNear, -1.0 * zNear,  1.0 * zNear);
	glVertex3f ( 1.0 * zNear,  1.0 * zNear,  1.0 * zNear);

	glColor3ub (0x00, 0xff, 0xff);
	glVertex3f (-1.0 * zNear,  1.0 * zNear, -1.0 * zNear);
	glVertex3f (-1.0 * zNear, -1.0 * zNear, -1.0 * zNear);
	glVertex3f (-1.0 * zNear, -1.0 * zNear,  1.0 * zNear);
	glVertex3f (-1.0 * zNear,  1.0 * zNear,  1.0 * zNear);

	glColor3ub (0xff, 0x00, 0xff);
	glVertex3f ( 1.0 * zNear, -1.0 * zNear, -1.0 * zNear);
	glVertex3f ( 1.0 * zNear, -1.0 * zNear,  1.0 * zNear);
	glVertex3f (-1.0 * zNear, -1.0 * zNear,  1.0 * zNear);
	glVertex3f (-1.0 * zNear, -1.0 * zNear, -1.0 * zNear);

	glColor3ub (0xff, 0xff, 0x00);
	glVertex3f ( 1.0 * zNear,  1.0 * zNear, -1.0 * zNear);
	glVertex3f ( 1.0 * zNear, -1.0 * zNear, -1.0 * zNear);
	glVertex3f (-1.0 * zNear, -1.0 * zNear, -1.0 * zNear);
	glVertex3f (-1.0 * zNear,  1.0 * zNear, -1.0 * zNear);
	glEnd ();
	glEnable (GL_DEPTH_TEST);
	glClear (GL_DEPTH_BUFFER_BIT);
}


bool systemToggleAttr (enum system_toggle_states toggle)
{
	switch (toggle)
	{
		case SYS_DEBUG:
			System->debug ^= 1;
			return System->debug;
		default:
			break;
	}
	return false;
}

bool systemAttr (enum system_toggle_states state)
{
	switch (state)
	{
		case SYS_DEBUG:
			return System->debug;
		default:
			break;
	}
	return false;
}

char * systemGenDebugStr ()
{

	signed int
		x = 0,
		y = 0,
		len = 0;
	unsigned int
		height = 0;
	Entity
		player = input_getPlayerEntity ();
	SUBHEX
		trav;
	char
		buffer[64];
	
	memset (debugDisplay, 0, DEBUGLEN);
	len = snprintf (buffer, 63, "Player Entity: #%d\nCamera Entity: #%d\n\nPosition\n", entity_GUID (player), entity_GUID (camera_getActiveCamera ()));
	strncpy (debugDisplay, buffer, len);

	entity_message (player, NULL, "getHex", &trav);
	height = subhexGetRawHeight (trav);

	len += snprintf (buffer, 63, "World\n\tSpan: %d\n\tRadius: %d\n\tPole: '%c'\n", mapGetSpan (), mapGetRadius (), subhexPoleName (trav));
	strncat (debugDisplay, buffer, DEBUGLEN - len);

	/* the poles have local coordinates technically, but they're always 0,0
	 * since there's no parent scope so we're skipping them */
	while (subhexSpanLevel (trav) < mapGetSpan ())
	{
		subhexLocalCoordinates (trav, &x, &y);
		len += snprintf (buffer, 63, "\t%d: %d, %d\n", subhexSpanLevel (trav), x, y);
		strncat (debugDisplay, buffer, DEBUGLEN - len);
		trav = subhexParent (trav);
	}

	len += snprintf (buffer, 63, "\nheight: %u\n", height);
	strncat (debugDisplay, buffer, DEBUGLEN - len);

	return debugDisplay;
}

/***
 * "MESSAGER"
 */

int system_message (objMsg msg, void * a, void * b)
{
	switch (msg)
	{
		case OM_SHUTDOWN:
			System->quit = true;
			systemPushState (STATE_QUIT);
			return EXIT_SUCCESS;

		case OM_FORCEWORLDGEN:
			dynarr_map (System->uiPanels, (void (*)(void *))entity_destroy);
			dynarr_clear (System->uiPanels);

			printf ("TRIGGERING WORLDGEN:\n");
			systemLoad (worldInit, worldGenerate, worldFinalize);
			return EXIT_SUCCESS;

		default:
			// no passing. no reason to.
			break;
	}
	return EXIT_FAILURE;
}