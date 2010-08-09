#include "physics.h"

Object * PhysicsObject = NULL;

PHYSICS * physics_create (ACCUMULATOR * acc) {
  PHYSICS * p = xph_alloc (sizeof (PHYSICS), "PHYSICS");
  p->acc = acc;
  return p;
}

void physics_destroy (PHYSICS * p) {
  accumulator_destroy (p->acc);
  xph_free (p);
}

bool physics_hasTime (Object * o) {
  PHYSICS * p = obj_getClassData (o, "physics");
  return accumulator_withdrawlTime (p->acc);
}

int physics_handler (Object * o, objMsg msg, void * a, void * b) {
  PHYSICS * p = NULL;
  switch (msg) {
    case OM_CLSNAME:
      strncpy (a, "physics", 32);
      return EXIT_SUCCESS;

    case OM_CLSINIT:
    case OM_CLSFREE:
    case OM_CLSVARS:
      return EXIT_FAILURE;

    case OM_CREATE:
      if (PhysicsObject != NULL) {
        obj_destroy (o);
        return EXIT_FAILURE;
      }
      if (a == NULL) {
        fprintf (stderr, "Physics entity initialized with no accumulator: this is going to cause a crash unless you fix it.\n");
      }
      p = physics_create (a);
      obj_addClassData (o, "physics", p);
      PhysicsObject = o;
      return EXIT_SUCCESS;

    default:
      break;
  }
  p = obj_getClassData (o, "physics");
  switch (msg) {
    case OM_SHUTDOWN:
    case OM_DESTROY:
      PhysicsObject = NULL;
      physics_destroy (p);
      obj_rmClassData (o, "physics");
      obj_destroy (o);
      return EXIT_SUCCESS;

    case OM_UPDATE:
      accumulator_update (p->acc);
      return EXIT_SUCCESS;

    default:
      return obj_pass ();
  }
  return EXIT_FAILURE;
}
