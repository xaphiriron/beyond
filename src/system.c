#include "system.h"

#include "object.h"
#include "video.h"
#include "timer.h"
#include "ui.h"

#include "worldgen.h"

#include "component_plant.h"
#include "component_input.h"
#include "component_camera.h"
#include "component_walking.h"

static void systemInitialize (void);

SYSTEM * System = NULL;

SYSTEM * system_create () {
	SYSTEM
		* s = xph_alloc (sizeof (SYSTEM));
	TIMER
		t = timerCreate ();

	s->quit = FALSE;

	s->clock = clock_create ();
	s->timer_mult = 1.0;
	s->timestep = 0.03;
	timerSetClock (t, s->clock);
	timerSetScale (t, s->timer_mult);
	s->acc = accumulator_create (t, s->timestep);

	s->state = dynarr_create (4, sizeof (enum system_states));
	dynarr_push (s->state, STATE_LOADING);
	// register system loading %/note callback here

	s->updateFuncs = dynarr_create (2, sizeof (void (*)(TIMER)));
	s->uiPanels = dynarr_create (2, sizeof (UIPANEL));
	return s;
}

void system_destroy (SYSTEM * s)
{
	FUNCOPEN ();
	dynarr_wipe (s->uiPanels, (void (*)(void *))uiDestroyPanel);
	dynarr_destroy (s->uiPanels);
	dynarr_destroy (s->updateFuncs);
	dynarr_destroy (s->state);
	accumulator_destroy (s->acc);
	xtimerDestroyRegistry ();
	clock_destroy (s->clock);
	xph_free (s);
	FUNCCLOSE ();
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

enum system_states systemGetState ()
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
		return FALSE;
	dynarr_push (System->state, state);
	return TRUE;
}

enum system_states systemPopState ()
{
	enum system_states
		state;
	if (System == NULL)
		return STATE_ERROR;
	dynarr_pop (System->state);
	state = *(enum system_states *)dynarr_back (System->state);
	return state;
}

bool systemClearStates ()
{
	if (System == NULL)
		return FALSE;
	dynarr_wipe (System->state, NULL);
	return TRUE;
}

/***
 * UI CONTROL FUNCTIONS
 */

UIPANEL systemPopUI ()
{
	if (System == NULL)
		return NULL;
	return *(UIPANEL *)dynarr_pop (System->uiPanels);
}

void systemPushUI (UIPANEL p)
{
	if (System == NULL)
	{
		ERROR ("Trying to push UI (%p) without a system", p);
		return;
	}
	dynarr_push (System->uiPanels, p);
}

enum uiPanelTypes systemTopUIPanelType ()
{
	if (System == NULL)
		return UI_NONE;
	return uiGetPanelType (*(UIPANEL *)dynarr_back (System->uiPanels));
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

void systemCreatePlayer ()
{
	Entity
		player,
		camera;
	FUNCOPEN ();

	player = entity_create ();
	if (component_instantiateOnEntity ("input", player)) {
		input_addEntity (player, INPUT_CONTROLLED);
	}
	if (component_instantiateOnEntity ("position", player))
	{
		systemPlacePlayerAt (mapPole ('r'));
	}
	component_instantiateOnEntity ("walking", player);

	camera = entity_create ();
	component_instantiateOnEntity ("position", camera);
	if (component_instantiateOnEntity ("camera", camera))
	{
		camera_attachToTarget (camera, player);
		camera_setAsActive (camera);
	}
	if (component_instantiateOnEntity ("input", camera))
	{
		input_addEntity (camera, INPUT_CONTROLLED);
	}
	FUNCCLOSE ();
}

void systemPlacePlayerAt (const SUBHEX subhex)
{
	FUNCOPEN ();
	Entity
		player = input_getPlayerEntity ();
	VECTOR3
		pos = vectorCreate (0.0, 0.0, 0.0);
	position_set (player, pos, subhex);
	worldSetRenderCacheCentre (subhex);
	FUNCCLOSE ();
}

/***
 * INITIALIZATION
 */

/* all these functions -- initialize, start, and update -- are relics from the awful object code; they used to be the code called by obj_message (SystemObject, (OM_CLSINIT|OM_START|OM_UPDATE), NULL, NULL), and so their structure is pointless. i ought to rewrite and clarify them at some point
 * - xph 2011-05-28
 */
static void systemInitialize (void)
{
	printf ("STARTING CLASS INIT\n");
	if (System != NULL)
		return;
	System = system_create ();

	//printf ("initializing other objects\n");
	objClass_init (video_handler, NULL, NULL, NULL);
	//objClass_init (physics_handler, NULL, NULL, NULL);
	//objClass_init (world_handler, NULL, NULL, NULL);

	//printf ("registering components\n");
	entity_registerComponentAndSystem (component_position);
//	entity_registerComponentAndSystem (component_ground);
//	entity_registerComponentAndSystem (component_integrate);
	entity_registerComponentAndSystem (component_camera);
//	entity_registerComponentAndSystem (component_collide);
	entity_registerComponentAndSystem (component_walking);
	entity_registerComponentAndSystem (component_input);
	entity_registerComponentAndSystem (component_plant);
	// this order DOES matter, since this is the order they're updated later.
	entitySubsystem_store ("position");
	entitySubsystem_store ("ground");
	entitySubsystem_store ("plant");
	entitySubsystem_store ("walking");
//	entitySubsystem_store ("integrate");
	entitySubsystem_store ("camera");
//	entitySubsystem_store ("collide");
	entitySubsystem_store ("input");

	// not really sure where these should go; they're going here for now.
	system_registerTimedFunction (entity_purgeDestroyed, 0x7f);
//	system_registerTimedFunction (cameraCache_update, 0x7f);
	system_registerTimedFunction (component_runLoader, 0x7f);

	obj_create ("video", NULL, NULL, NULL);
/*
	obj_create ("physics", SystemObject,
		accumulator_create (xtimer_create (s->clock, 1.0), 0.03), NULL
	);
*/
#ifdef MEM_DEBUG
	atexit (xph_audit);
#endif /* MEM_DEBUG */
	printf ("DONE WITH SYSTEM CLASS INIT\n");
	return;
}

void systemStart (void)
{
	/* for the record, the VideoEntity calls SDL_Init; everything else
	 * calls SDL_InitSubSystem. so the video entity has to start first.
	 * If this becomes a problem, feel free to switch the SDL_Init call
	 * to somewhere where it will /really/ always be called first (the
	 * system entity seems like a good place) and make everything else
	 * use SDL_InitSubSystem.
	 */
	LOG (E_FUNCLABEL, "%s...", __FUNCTION__);
	obj_message (VideoObject, OM_START, NULL, NULL);
	//obj_message (PhysicsObject, OM_START, NULL, NULL);
	//obj_message (WorldObject, OM_START, NULL, NULL);
	entitySubsystem_message ("ground", OM_START, NULL, NULL);
	printf ("DONE W/ SYSTEM START:\n");
	printf ("ARTIFICIALLY TRIGGERING WORLDGEN:\n");
	systemClearStates ();
	systemPushState (STATE_LOADING);
	// register worldgen loading %/note callback here
	worldgenAbsHocNihilo ();
	LOG (E_FUNCLABEL, "...%s", __FUNCTION__);
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
		entitySubsystem_runOnStored (OM_UPDATE);
		entitySubsystem_runOnStored (OM_POSTUPDATE);
	}
	FUNCCLOSE ();
}

/***
 * RENDERING
 */

void systemRender (void)
{
	Entity
		camera;
	const float
		* matrix;
	int
		i;

	if (System == NULL)
		return;
	obj_messagePre (VideoObject, OM_PRERENDER, NULL, NULL);
	if (systemGetState (System) == STATE_LOADING)
	{
		// presumably draw loading screen + loading text
	}

	if (worldExists ())
	{
		camera = camera_getActiveCamera ();
		matrix = camera_getMatrix (camera);
		if (matrix == NULL)
			glLoadIdentity ();
		else
			glLoadMatrixf (matrix);
		mapDraw ();
		if (systemGetState (System) == STATE_FREEVIEW)
			camera_drawCursor ();
	}

	if (dynarr_size (System->uiPanels))
	{
		glLoadIdentity ();
		glDisable (GL_DEPTH_TEST);
		i = 0;
		while (i < dynarr_size (System->uiPanels))
		{
			// draw ui stuff
			uiDrawPanel (*(UIPANEL *)dynarr_at (System->uiPanels, i));
			i++;
		}
		glEnable (GL_DEPTH_TEST);
	}
	obj_messagePre (VideoObject, OM_POSTRENDER, NULL, NULL);
}

/***
 * "MESSAGER"
 */

int system_message (objMsg msg, void * a, void * b)
{
	switch (msg)
	{
		case OM_CLSINIT:
			systemInitialize ();
			return EXIT_SUCCESS;

		case OM_CLSFREE:
			LOG (E_FUNCLABEL, "%s[OM_CLSFREE]...", __FUNCTION__);
			entity_destroyEverything ();
			SDL_Quit ();
			LOG (E_FUNCLABEL, "...%s[OM_CLSFREE]", __FUNCTION__);
			return EXIT_SUCCESS;

		case OM_SHUTDOWN:
			System->quit = TRUE;
			systemPushState (STATE_QUIT);
			return EXIT_SUCCESS;

		case OM_DESTROY:
			LOG (E_FUNCLABEL, "%s[OM_DESTROY]...", __FUNCTION__);
			/* this is post-order because if it was pre-order it wouldn't hit
			 * all its children due to an annoying issue I can't fix easily.
			 * When an entity is destroyed it detatches itself from the entity
			 * hierarchy, and since the *Entity globals are at the top of the
			 * entity tree, destroying them makes their children no longer
			 * siblings and thus unable to message each other. most likely
			 * I'll need to rework how messaging works internally, so I can
			 * ensure a message sent pre/post/in/whatever will hit all of the
			 * entities it should, at the moment the message was first sent
			 * (messages that change the entity tree that use the entity tree
			 * to flow are kind of a challenge, as you might imagine :/)
			 */
			/* i think the preceeding comment is outdated -- the object
			 * messaging functions traverse the tree prior to sending the
			 * message now. also, all the object code is terrible and ought to
			 * be removed as soon as is feasible.
			 *  - xph 2011-05-22
			 */
			obj_messagePost (VideoObject, OM_DESTROY, NULL, NULL);
//			obj_messagePost (PhysicsObject, OM_DESTROY, NULL, NULL);
			system_destroy (System);
			System = NULL;
			LOG (E_FUNCLABEL, "...%s[OM_DESTROY]", __FUNCTION__);
			return EXIT_SUCCESS;


		default:
			// no passing. no reason to.
			break;
	}
	return EXIT_FAILURE;
}