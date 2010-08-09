#include "world.h"

Object * WorldObject = NULL;

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

int world_handler (Object * o, objMsg msg, void * a, void * b) {
  WORLD * w = NULL;
  switch (msg) {
    case OM_CLSNAME:
      strncpy (a, "world", 32);
      return EXIT_SUCCESS;
    case OM_CLSINIT:
    case OM_CLSFREE:
      return EXIT_FAILURE;
    case OM_CLSVARS:
      return EXIT_FAILURE;

    case OM_CREATE:
      if (WorldObject != NULL) {
        obj_destroy (o);
        return EXIT_FAILURE;
      }
      w = world_create ();
      obj_addClassData (o, "world", w);
      WorldObject = o;
      return EXIT_SUCCESS;

    default:
      break;
  }
  w = obj_getClassData (o, "world");
  switch (msg) {
    case OM_SHUTDOWN:
    case OM_DESTROY:
      WorldObject = NULL;
      world_destroy (w);
      obj_rmClassData (o, "world");
      obj_destroy (o);
      return EXIT_SUCCESS;

    case OM_UPDATE:
      // integrate kids or prepare kids to be integrated
      return EXIT_FAILURE;

    case OM_POSTUPDATE:
      // do integration things which depend on all world objects having their
      // new position and momentum and the like
      return EXIT_FAILURE;

    case OM_PRERENDER:
      // called after video:prerender and before this:render
      glPushMatrix ();
      //glLoadMatrixf (w->c->viewMatrix);
      return EXIT_SUCCESS;

    case OM_RENDER:
      // DRAW THINGS
      //triplane_render (w->tp, &w->origin);
      return EXIT_SUCCESS;

    case OM_POSTRENDER:
      // called after this:render. do we really need this?
      glPopMatrix ();
      return EXIT_SUCCESS;

    default:
      return obj_pass ();
  }
  return EXIT_FAILURE;
}

