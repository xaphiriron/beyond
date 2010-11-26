#include "physics.h"

Object * PhysicsObject = NULL;

PHYSICS * physics_create (ACCUMULATOR * acc) {
  PHYSICS * p = xph_alloc (sizeof (PHYSICS));
  p->acc = acc;
  p->timestep = p->acc->delta;
  return p;
}

void physics_destroy (PHYSICS * p) {
  accumulator_destroy (p->acc);
  xph_free (p);
}

bool physics_hasTime (Object * o) {
  //bool time = FALSE;
  PHYSICS * p = obj_getClassData (o, "physics");
  //time = accumulator_withdrawlTime (p->acc);
  //printf ("%s: %s\n", __FUNCTION__, time ? "TRUE" : "FALSE");
  //printf ("  time passed: %f\n", p->acc->timer->elapsed);
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

    // the physics entity is just an accumulator. it doesn't do any actual physics. why are we doing things this way, anyway?
    case OM_UPDATE:
      // the important thing to remember here is that the accumulator does not update the timer it is using, which means this is safe to call anywhere between timesteps, since the timer will return the same value.
      accumulator_update (p->acc);
      //printf ("have %f seconds in the accumulator; enough for %d timesteps\n", p->acc->accumulated, (int)(p->acc->accumulated / p->acc->delta));
      return EXIT_SUCCESS;

    default:
      return obj_pass ();
  }
  return EXIT_FAILURE;
}
