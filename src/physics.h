#ifndef PHYSICS_H
#define PHYSICS_H

#include "bool.h"
#include "object.h"
#include "accumulator.h"

extern Object * PhysicsObject;

typedef struct physics {
  ACCUMULATOR * acc;

  float timestep;
} PHYSICS;

PHYSICS * physics_create (ACCUMULATOR *);
void physics_destroy (PHYSICS *);

bool physics_hasTime (Object * o);

int physics_handler (Object * o, objMsg msg, void *, void *);

#endif /* PHYSICS_H */
