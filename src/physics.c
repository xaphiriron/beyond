#include "physics.h"

ENTITY * PhysicsEntity = NULL;

PHYSICS * physics_create (ACCUMULATOR * acc) {
  PHYSICS * p = xph_alloc (sizeof (PHYSICS), "PHYSICS");
  p->acc = acc;
  return p;
}

void physics_destroy (PHYSICS * p) {
  accumulator_destroy (p->acc);
  xph_free (p);
}

bool physics_hasTime (ENTITY * e) {
  PHYSICS * p = entity_getClassData (e, "physics");
  return accumulator_withdrawlTime (p->acc);
}

int physics_handler (ENTITY * e, eMessage msg, void * a, void * b) {
  PHYSICS * p = NULL;
  switch (msg) {
    case EM_CLSNAME:
      strncpy (a, "physics", 32);
      return EXIT_SUCCESS;

    case EM_CLSINIT:
    case EM_CLSFREE:
    case EM_CLSVARS:
      return EXIT_FAILURE;

    case EM_CREATE:
      if (PhysicsEntity != NULL) {
        entity_destroy (e);
        return EXIT_FAILURE;
      }
      if (a == NULL) {
        fprintf (stderr, "Physics entity initialized with no accumulator: this is going to cause a crash unless you fix it.\n");
      }
      p = physics_create (a);
      entity_addClassData (e, "physics", p);
      PhysicsEntity = e;
      return EXIT_SUCCESS;

    default:
      break;
  }
  p = entity_getClassData (e, "physics");
  switch (msg) {
    case EM_SHUTDOWN:
    case EM_DESTROY:
      PhysicsEntity = NULL;
      physics_destroy (p);
      entity_rmClassData (e, "physics");
      entity_destroy (e);
      return EXIT_SUCCESS;

    case EM_UPDATE:
      accumulator_update (p->acc);
      return EXIT_SUCCESS;

    default:
      return entity_pass ();
  }
  return EXIT_FAILURE;
}
