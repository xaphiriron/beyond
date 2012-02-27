/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#include "system.h"

#include "video.h"
#include "xph_timer.h"

#include "texture.h"
#include "font.h"

#include "component_input.h"
#include "component_camera.h"
#include "component_position.h"

#define CONFIGPATH "data/settings"

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

static struct loadingdata
	SysLoader;

SYSTEM
	* System = NULL;
Entity
	SystemEntity = NULL;

static void system_define (EntComponent component, EntSpeech speech);
static void system_shutdown (Entity this, const inputEvent * const event);

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

	System->config = Ogdl_load (xph_canonCachePath (CONFIGPATH));

#ifdef MEM_DEBUG
	atexit (xph_audit);
#endif /* MEM_DEBUG */

	videoInit ();

	component_register ("SYSTEM", system_define);
	component_register ("input", input_define);
	SystemEntity = entity_create ();
	component_instantiate ("SYSTEM", SystemEntity);
	component_instantiate ("input", SystemEntity);
	entity_refresh (SystemEntity);
	entity_message (SystemEntity, NULL, "gainFocus", NULL);
	input_addAction (SystemEntity, IR_QUIT, system_shutdown);

	return;
}

static void system_define (EntComponent component, EntSpeech speech)
{
}

static void system_shutdown (Entity this, const inputEvent * const event)
{
	System->quit = true;
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
	// FIXME: for some reason that i don't fully understand, destroying the system entity early (it's automatically destroyed with the other entities) makes entity_destroyEverything incapable of actually purging all other entities. this is probably a really dumb bug somewhere, but it's not really a major concern so i'm not going to hunt it down - xph 02 27 2012
	//entity_destroy (SystemEntity);
	SystemEntity = NULL;

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
	while (!System->quit && accumulator_withdrawlTime (System->acc))
	{
		i = 0;
		while ((func = *(void (**)(TIMER *))dynarr_at (System->updateFuncs, i)) != NULL)
		{
			// FIXME: this is the amount of time in seconds to give each function. it ought to 1) be related to the accumulator delta and 2) be divided between update funcs by their priority (which is the second arg of the register function, but it's ignored completely right now)
			timerSetGoal (t, 50);
			timerUpdate (t);
			func (t);
			i++;
		}
		entitySystem_update ("input");

		entitySystem_update ("ui");
		entitySystem_update ("gui");

		entitySystem_update ("walking");
		entitySystem_update ("builder");
		entitySystem_update ("chaser");

		entitySystem_update ("debug");
		entitySystem_update ("mapLoad");
	}

	videoPrerender ();
	entitySystem_update ("TEMPsystemRender");
	entitySystem_update ("bodyRender");

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
