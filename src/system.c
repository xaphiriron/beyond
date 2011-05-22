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


Object * SystemObject = NULL;
SYSTEM * System = NULL;

static void system_update ();

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
	SYSTEM
		* s;
	if (SystemObject == NULL)
		return NULL;
	s = obj_getClassData (SystemObject, "SYSTEM");
	return s->acc->timer;
}

/***
 * SYSTEM STATE
 */

enum system_states systemGetState ()
{
	SYSTEM
		* s;
	enum system_states
		state;
	if (SystemObject == NULL)
		return STATE_ERROR;
	s = obj_getClassData (SystemObject, "SYSTEM");
	state = *(enum system_states *)dynarr_back (s->state);
	return state;
}

bool systemPushState (enum system_states state)
{
	SYSTEM
		* s;
	if (SystemObject == NULL)
		return FALSE;
	s = obj_getClassData (SystemObject, "SYSTEM");
	dynarr_push (s->state, state);
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
	SYSTEM
		* s;
	if (SystemObject == NULL)
		return FALSE;
	s = obj_getClassData (SystemObject, "SYSTEM");
	dynarr_wipe (s->state, NULL);
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
	SYSTEM
		* s;
	if (SystemObject == NULL)
		return;
	s = obj_getClassData (SystemObject, "SYSTEM");
	dynarr_push (s->updateFuncs, func);
}

void system_removeTimedFunction (void (*func)(TIMER))
{
	SYSTEM
		* s;
	if (SystemObject == NULL)
		return;
	s = obj_getClassData (SystemObject, "SYSTEM");
	dynarr_remove_condense (s->updateFuncs, func);
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
 * RENDERING
 */

void systemRender ()
{
	Entity
		camera;
	const float
		* matrix;
	int
		i;

	if (System == NULL)
		return;
	if (systemGetState (System) == STATE_LOADING)
	{
		// presumably draw loading screen + loading text
		return;
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
}

/***
 * HANDLER
 */

int system_handler (Object * o, objMsg msg, void * a, void * b)
{
	SYSTEM
		* s = NULL;
	switch (msg)
	{
		case OM_CLSNAME:
			strncpy (a, "SYSTEM", 32);
			return EXIT_SUCCESS;
		case OM_CLSINIT:
			printf ("STARTING CLASS INIT\n");
			obj_create ("SYSTEM", NULL, NULL, NULL);
			s = obj_getClassData (SystemObject, "SYSTEM");
			//printf ("initializing other objects\n");
			objClass_init (video_handler, NULL, NULL, NULL);
			//objClass_init (physics_handler, NULL, NULL, NULL);
			//objClass_init (world_handler, NULL, NULL, NULL);
			//printf ("registering components\n");
			entity_registerComponentAndSystem (component_position);
//			entity_registerComponentAndSystem (component_ground);
//			entity_registerComponentAndSystem (component_integrate);
			entity_registerComponentAndSystem (component_camera);
//			entity_registerComponentAndSystem (component_collide);
			entity_registerComponentAndSystem (component_walking);
			entity_registerComponentAndSystem (component_input);
			entity_registerComponentAndSystem (component_plant);
			// this order DOES matter, since this is the order they're updated later.
			entitySubsystem_store ("position");
			entitySubsystem_store ("ground");
			entitySubsystem_store ("plant");
			entitySubsystem_store ("walking");
//			entitySubsystem_store ("integrate");
			entitySubsystem_store ("camera");
//			entitySubsystem_store ("collide");
			entitySubsystem_store ("input");

			// not really sure where these should go; they're going here for now.
			system_registerTimedFunction (entity_purgeDestroyed, 0x7f);
//			system_registerTimedFunction (cameraCache_update, 0x7f);
			system_registerTimedFunction (component_runLoader, 0x7f);

			obj_create ("video", SystemObject, NULL, NULL);
/*
			obj_create ("physics", SystemObject,
				accumulator_create (xtimer_create (s->clock, 1.0), 0.03), NULL
			);
*/
#ifdef MEM_DEBUG
			atexit (xph_audit);
#endif /* MEM_DEBUG */
			printf ("DONE WITH SYSTEM CLASS INIT\n");
			return EXIT_SUCCESS;
		case OM_CLSFREE:
			LOG (E_FUNCLABEL, "%s[OM_CLSFREE]...", __FUNCTION__);
			entity_destroyEverything ();
			SDL_Quit ();
			LOG (E_FUNCLABEL, "...%s[OM_CLSFREE]", __FUNCTION__);
			return EXIT_SUCCESS;

		case OM_CLSVARS:
			return EXIT_FAILURE;

		case OM_CREATE:
			if (SystemObject != NULL)
			{
				obj_destroy (o);
				return EXIT_FAILURE;
			}
			s = system_create ();
			obj_addClassData (o, "SYSTEM", s);
			SystemObject = o;
			System = s;
			return EXIT_SUCCESS;

		default:
			break;
	}
	s = obj_getClassData (o, "SYSTEM");
	switch (msg)
	{
		case OM_SHUTDOWN:
			s->quit = TRUE;
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
			system_destroy (s);
			obj_rmClassData (o, "SYSTEM");
			obj_destroy (o);
			SystemObject = NULL;
			LOG (E_FUNCLABEL, "...%s[OM_DESTROY]", __FUNCTION__);
			return EXIT_SUCCESS;

		case OM_START:
			/* for the record, the VideoEntity calls SDL_Init; everything else
			 * calls SDL_InitSubSystem. so the video entity has to start first.
			 * If this becomes a problem, feel free to switch the SDL_Init call
			 * to somewhere where it will /really/ always be called first (the
			 * system entity seems like a good place) and make everything else
			 * use SDL_InitSubSystem.
			 */
			LOG (E_FUNCLABEL, "%s[OM_START]...", __FUNCTION__);
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
			LOG (E_FUNCLABEL, "...%s[OM_START]", __FUNCTION__);
			return EXIT_SUCCESS;

		case OM_UPDATE:
			LOG (E_FUNCLABEL, "%s[OM_UPDATE]...", __FUNCTION__);
			system_update ();
			obj_halt ();
			LOG (E_FUNCLABEL, "...%s[OM_UPDATE]", __FUNCTION__);
			return EXIT_SUCCESS;

		default:
			// no passing. no reason to.
			break;
	}
	return EXIT_FAILURE;
}


void system_update ()
{
	SYSTEM
		* s;
	int
		i;
	TIMER
		t;
	void (*func)(TIMER);
	if (SystemObject == NULL)
		return;
	s = obj_getClassData (SystemObject, "SYSTEM");
	clock_update (s->clock);
	xtimerUpdateAll ();
	accumulator_update (s->acc);

	t = timerCreate ();
	timerSetClock (t, s->clock);
	while (accumulator_withdrawlTime (s->acc))
	{
		i = 0;
		while ((func = *(void (**)(TIMER))dynarr_at (s->updateFuncs, i)) != NULL)
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
	timerDestroy (t);
}
