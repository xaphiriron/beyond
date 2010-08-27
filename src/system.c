#include "system.h"

Object * SystemObject = NULL;

SYSTEM * system_create () {
  SYSTEM * s = xph_alloc (sizeof (SYSTEM), "SYSTEM");
  s->quit = FALSE;
  s->clock = clock_create ();
  s->timer_mult = 1.0;
  s->state = STATE_INIT;
  return s;
}

void system_destroy (SYSTEM * s) {
  timer_destroyTimerRegistry ();
  clock_destroy (s->clock);
  xph_free (s);
}

int system_handler (Object * o, objMsg msg, void * a, void * b) {
  SYSTEM * s = NULL;
  //printf ("IN SYSTEM HANDLER W\\ MESSAGE %d\n", msg);
  switch (msg) {
    case OM_CLSNAME:
      strncpy (a, "SYSTEM", 32);
      return EXIT_SUCCESS;
    case OM_CLSINIT:
      obj_create ("SYSTEM", NULL, NULL, NULL);
      s = obj_getClassData (SystemObject, "SYSTEM");

      objClass_init (video_handler, NULL, NULL, NULL);
      objClass_init (control_handler, NULL, NULL, NULL);
      objClass_init (physics_handler, NULL, NULL, NULL);
      objClass_init (world_handler, NULL, NULL, NULL);
      obj_create ("video", SystemObject,
        NULL, NULL);
      obj_create ("control", SystemObject,
        NULL, NULL);
      obj_create ("physics", SystemObject,
        accumulator_create (timer_create (s->clock, 1.0), 0.03), NULL);
      obj_create ("world", SystemObject,
        NULL, NULL);

#ifdef MOM_DEBUG
      atexit (xph_audit);
#endif /* MOM_DEBUG */
      return EXIT_SUCCESS;
    case OM_CLSFREE:
      //entclass_destroyAll ();
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
      obj_messagePost (WorldObject, OM_DESTROY, NULL, NULL);
      obj_messagePost (ControlObject, OM_DESTROY, NULL, NULL);
      obj_messagePost (VideoObject, OM_DESTROY, NULL, NULL);
      obj_messagePost (PhysicsObject, OM_DESTROY, NULL, NULL);

      system_destroy (s);
      obj_rmClassData (o, "SYSTEM");
      obj_destroy (o);
      SystemObject = NULL;
      return EXIT_SUCCESS;

    case OM_START:
      // for the record, the VideoEntity calls SDL_Init; everything else calls SDL_InitSubSystem. so the video entity has to start first. If this becomes a problem, feel free to switch the SDL_Init call to somewhere where it will /really/ always be called first (the system entity seems like a good place) and make everything else use SDL_InitSubSystem.
      obj_message (VideoObject, OM_START, NULL, NULL);
      obj_message (ControlObject, OM_START, NULL, NULL);
      obj_message (PhysicsObject, OM_START, NULL, NULL);
      obj_message (WorldObject, OM_START, NULL, NULL);
      return EXIT_SUCCESS;

    case OM_UPDATE:
      clock_update (s->clock);
      timer_updateAll ();
      obj_halt ();
      return EXIT_SUCCESS;

    default:
      // no passing. no reason to.
      break;
  }
  return EXIT_FAILURE;
}
