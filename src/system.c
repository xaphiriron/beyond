#include "system.h"

ENTITY * SystemEntity = NULL;

SYSTEM * system_create () {
  SYSTEM * s = xph_alloc (sizeof (SYSTEM), "SYSTEM");
  s->quit = FALSE;
  s->clock = clock_create ();
  s->state = STATE_INIT;
  return s;
}

void system_destroy (SYSTEM * s) {
  timer_destroyTimerRegistry ();
  clock_destroy (s->clock);
  xph_free (s);
}

int system_handler (ENTITY * e, eMessage msg, void * a, void * b) {
  SYSTEM * s = NULL;
  //printf ("IN SYSTEM HANDLER W\\ MESSAGE %d\n", msg);
  switch (msg) {
    case EM_CLSNAME:
      strncpy (a, "SYSTEM", 32);
      return EXIT_SUCCESS;
    case EM_CLSINIT:
      entity_create ("SYSTEM", NULL, NULL, NULL);
      s = entity_getClassData (SystemEntity, "SYSTEM");

      entclass_init (video_handler, NULL, NULL, NULL);
      entclass_init (control_handler, NULL, NULL, NULL);
      entclass_init (physics_handler, NULL, NULL, NULL);
      entclass_init (world_handler, NULL, NULL, NULL);
      entity_create ("video", SystemEntity,
        NULL, NULL);
      entity_create ("control", SystemEntity,
        NULL, NULL);
      entity_create ("physics", SystemEntity,
        accumulator_create (timer_create (s->clock, 1.0), 0.05), NULL);
      entity_create ("world", SystemEntity,
        NULL, NULL);

#ifdef MEM_DEBUG
      atexit (xph_audit);
#endif /* MEM_DEBUG */
      return EXIT_SUCCESS;
    case EM_CLSFREE:
      //entclass_destroyAll ();
      SDL_Quit ();
      return EXIT_SUCCESS;
    case EM_CLSVARS:
      return EXIT_FAILURE;
    case EM_CREATE:
      if (SystemEntity != NULL) {
        entity_destroy (e);
        return EXIT_FAILURE;
      }
      s = system_create ();
      entity_addClassData (e, "SYSTEM", s);
      SystemEntity = e;
      return EXIT_SUCCESS;

    default:
      break;
  }
  s = entity_getClassData (e, "SYSTEM");
  switch (msg) {
    case EM_SHUTDOWN:
      s->quit = TRUE;
      s->state = STATE_QUIT;
      return EXIT_SUCCESS;
    case EM_DESTROY:
      // this is post-order because if it was pre-order it wouldn't hit all its children due to an annoying issue I can't fix easily. When an entity is destroyed it detatches itself from the entity hierarchy, and since the *Entity globals are at the top of the entity tree, destroying them makes their children no longer siblings and thus unable to message each other. most likely I'll need to rework how messaging works internally, so I can ensure a message sent pre/post/in/whatever will hit all of the entities it should, at the moment the message was first sent (messages that change the entity tree that use the entity tree to flow are kind of a challenge, as you might imagine :/)
      entity_messagePost (WorldEntity, EM_DESTROY, NULL, NULL);
      entity_messagePost (ControlEntity, EM_DESTROY, NULL, NULL);
      entity_messagePost (VideoEntity, EM_DESTROY, NULL, NULL);
      entity_messagePost (PhysicsEntity, EM_DESTROY, NULL, NULL);

      system_destroy (s);
      entity_rmClassData (e, "SYSTEM");
      entity_destroy (e);
      SystemEntity = NULL;
      return EXIT_SUCCESS;

    case EM_START:
      // for the record, the VideoEntity calls SDL_Init; everything else calls SDL_InitSubSystem. so the video entity has to start first. If this becomes a problem, feel free to switch the SDL_Init call to somewhere where it will /really/ always be called first (the system entity seems like a good place) and make everything else use SDL_InitSubSystem.
      entity_message (VideoEntity, EM_START, NULL, NULL);
      entity_message (ControlEntity, EM_START, NULL, NULL);
      entity_message (PhysicsEntity, EM_START, NULL, NULL);
      entity_message (WorldEntity, EM_START, NULL, NULL);
      return EXIT_SUCCESS;

    case EM_UPDATE:
      clock_update (s->clock);
      timer_updateAll ();
      entity_halt ();
      return EXIT_SUCCESS;

    default:
      // no passing. no reason to.
      break;
  }
  return EXIT_FAILURE;
}
