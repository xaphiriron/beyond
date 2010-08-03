#include "world.h"

ENTITY * WorldEntity = NULL;

WORLD * world_create () {
  WORLD * w = xph_alloc (sizeof (WORLD), "WORLD");
  //w->tp = triplane_create (512.0, 7);
  w->origin = vectorCreate (0.0, 0.0, 0.0);
  //w->c = camera_create (CAM_MODE_FIRST);
  return w;
}

void world_destroy (WORLD * w) {
  //triplane_destroy (w->tp);
  //camera_destroy (w->c);
  xph_free (w);
}

int world_handler (ENTITY * e, eMessage msg, void * a, void * b) {
  WORLD * w = NULL;
  switch (msg) {
    case EM_CLSNAME:
      strncpy (a, "world", 32);
      return EXIT_SUCCESS;
    case EM_CLSINIT:
    case EM_CLSFREE:
      return EXIT_FAILURE;
    case EM_CLSVARS:
      return EXIT_FAILURE;

    case EM_CREATE:
      if (WorldEntity != NULL) {
        entity_destroy (e);
        return EXIT_FAILURE;
      }
      w = world_create ();
      entity_addClassData (e, "world", w);
      WorldEntity = e;
      return EXIT_SUCCESS;

    default:
      break;
  }
  w = entity_getClassData (e, "world");
  switch (msg) {
    case EM_SHUTDOWN:
    case EM_DESTROY:
      WorldEntity = NULL;
      world_destroy (w);
      entity_rmClassData (e, "world");
      entity_destroy (e);
      return EXIT_SUCCESS;

    case EM_UPDATE:
      // integrate kids or prepare kids to be integrated
      return EXIT_FAILURE;

    case EM_POSTUPDATE:
      // do integration things which depend on all world objects having their
      // new position and momentum and the like
      return EXIT_FAILURE;

    case EM_PRERENDER:
      // called after video:prerender and before this:render
      glPushMatrix ();
      //glLoadMatrixf (w->c->viewMatrix);
      return EXIT_SUCCESS;

    case EM_RENDER:
      // DRAW THINGS
      //triplane_render (w->tp, &w->origin);
      return EXIT_SUCCESS;

    case EM_POSTRENDER:
      // called after this:render. do we really need this?
      glPopMatrix ();
      return EXIT_SUCCESS;

    default:
      return entity_pass ();
  }
  return EXIT_FAILURE;
}

