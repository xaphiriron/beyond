#include "system.h"

Object * SystemObject = NULL;

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
  s->state = STATE_INIT;
	s->updateFuncs = dynarr_create (2, sizeof (void (*)(TIMER)));
  return s;
}

void system_destroy (SYSTEM * s)
{
	dynarr_destroy (s->updateFuncs);
	accumulator_destroy (s->acc);
	xtimerDestroyRegistry ();
	clock_destroy (s->clock);
	xph_free (s);
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

enum system_states system_getState (const SYSTEM * s)
{
	if (s == NULL)
		return 0;
	return s->state;
}

bool system_setState (enum system_states state)
{
	SYSTEM
		* s;
	if (SystemObject == NULL)
		return FALSE;
	s = obj_getClassData (SystemObject, "SYSTEM");
	s->state = state;
	return TRUE;
}

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

int system_handler (Object * o, objMsg msg, void * a, void * b)
{
	SYSTEM
		* s = NULL;
	//printf ("IN SYSTEM HANDLER W\\ MESSAGE %d\n", msg);
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
			entity_registerComponentAndSystem (component_pattern);
			entity_registerComponentAndSystem (component_ground);
//			entity_registerComponentAndSystem (component_integrate);
			entity_registerComponentAndSystem (component_camera);
//			entity_registerComponentAndSystem (component_collide);
			entity_registerComponentAndSystem (component_walking);
			entity_registerComponentAndSystem (component_input);
			entity_registerComponentAndSystem (component_plant);
      // this order DOES matter, since this is the order they're updated later.
      entitySubsystem_store ("position");
      entitySubsystem_store ("pattern");
      entitySubsystem_store ("ground");
      entitySubsystem_store ("plant");
      entitySubsystem_store ("walking");
//      entitySubsystem_store ("integrate");
      entitySubsystem_store ("camera");
//      entitySubsystem_store ("collide");
      entitySubsystem_store ("input");

	// not really sure where these should go; they're going here for now.
	system_registerTimedFunction (entity_purgeDestroyed, 0x7f);
	system_registerTimedFunction (cameraCache_update, 0x7f);
	system_registerTimedFunction (component_runLoader, 0x7f);

			//printf ("creating additional objects\n");
			//printf ("\tvideo:\n");
      obj_create ("video", SystemObject,
        NULL, NULL);
			//printf ("\tphysics:\n");
/*
      obj_create ("physics", SystemObject,
        accumulator_create (xtimer_create (s->clock, 1.0), 0.03), NULL);
*/
			//printf ("\tworld:\n");
/*
      obj_create ("world", SystemObject,
        NULL, NULL);
*/

#ifdef MEM_DEBUG
			atexit (xph_audit);
#endif /* MEM_DEBUG */
			printf ("DONE WITH SYSTEM CLASS INIT\n");
			return EXIT_SUCCESS;
    case OM_CLSFREE:
      //printf ("%s[OM_CLSFREE]: calling entity_destroyEverything\n", __FUNCTION__);
      entity_destroyEverything ();
      //printf ("...done\n");
      SDL_Quit ();
      return EXIT_SUCCESS;
    case OM_CLSVARS:
      return EXIT_FAILURE;
    case OM_CREATE:
      if (SystemObject != NULL) {
        obj_destroy (o);
        return EXIT_FAILURE;
      }
      s = system_create ();
      obj_addClassData (o, "SYSTEM", s);
      SystemObject = o;
      return EXIT_SUCCESS;

    default:
      break;
  }
  s = obj_getClassData (o, "SYSTEM");
  switch (msg) {
    case OM_SHUTDOWN:
      s->quit = TRUE;
      s->state = STATE_QUIT;
      return EXIT_SUCCESS;
    case OM_DESTROY:
      // this is post-order because if it was pre-order it wouldn't hit all its children due to an annoying issue I can't fix easily. When an entity is destroyed it detatches itself from the entity hierarchy, and since the *Entity globals are at the top of the entity tree, destroying them makes their children no longer siblings and thus unable to message each other. most likely I'll need to rework how messaging works internally, so I can ensure a message sent pre/post/in/whatever will hit all of the entities it should, at the moment the message was first sent (messages that change the entity tree that use the entity tree to flow are kind of a challenge, as you might imagine :/)
      //printf ("%s[OM_DESTROY]\n", __FUNCTION__);
      //obj_messagePost (WorldObject, OM_DESTROY, NULL, NULL);
      obj_messagePost (VideoObject, OM_DESTROY, NULL, NULL);
      //obj_messagePost (PhysicsObject, OM_DESTROY, NULL, NULL);

      //printf ("%s[OM_DESTROY]: destroying system data\n", __FUNCTION__);
      system_destroy (s);
      //printf ("%s[OM_DESTROY]: removing null system object data\n", __FUNCTION__);
      obj_rmClassData (o, "SYSTEM");
      //printf ("%s[OM_DESTROY]: destroying system object\n", __FUNCTION__);
      obj_destroy (o);
      SystemObject = NULL;
      return EXIT_SUCCESS;

		case OM_START:
			// for the record, the VideoEntity calls SDL_Init; everything else calls SDL_InitSubSystem. so the video entity has to start first. If this becomes a problem, feel free to switch the SDL_Init call to somewhere where it will /really/ always be called first (the system entity seems like a good place) and make everything else use SDL_InitSubSystem.
			printf ("SYSTEM START:\n");
			obj_message (VideoObject, OM_START, NULL, NULL);
			//obj_message (PhysicsObject, OM_START, NULL, NULL);
			//obj_message (WorldObject, OM_START, NULL, NULL);
			// this next line "generates" the "world" - xph 2011-01-11
			entitySubsystem_message ("ground", OM_START, NULL, NULL);
			printf ("DONE W/ SYSTEM START:\n");
			printf ("ARTIFICIALLY TRIGGERING WORLDGEN:\n");
			worldgenAbsHocNihilo ();
			system_setState (STATE_WORLDGEN);
			return EXIT_SUCCESS;

		case OM_UPDATE:
			system_update ();

/*
			clock_update (s->clock);
			xtimer_updateAll ();
			accumulator_update (s->acc);
			entity_purgeDestroyed ();
			while (accumulator_withdrawlTime (s->acc))
			{
				cameraCache_update (system_getTimer ());
				component_runLoader (system_getTimer ());
				//obj_messagePre (WorldObject, OM_UPDATE, NULL, NULL);
				entitySubsystem_runOnStored (OM_UPDATE);
				//obj_messagePre (WorldObject, OM_POSTUPDATE, NULL, NULL);
				entitySubsystem_runOnStored (OM_POSTUPDATE);
			}
*/
			obj_halt ();
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
			// FIXME: this is the amount of time in seconds to give each function. it ought to 1) be related to the accumulator delta and 2) be divided between update funcs by their priority
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
