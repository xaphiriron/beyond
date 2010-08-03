#ifndef PHYSICS_H
#define PHYSICS_H

#include "bool.h"
#include "entity.h"
#include "accumulator.h"

extern ENTITY * PhysicsEntity;

typedef struct physics {
  ACCUMULATOR * acc;
} PHYSICS;

PHYSICS * physics_create (ACCUMULATOR *);
void physics_destroy (PHYSICS *);

bool physics_hasTime (ENTITY * e);

int physics_handler (ENTITY * e, eMessage msg, void *, void *);

#endif /* PHYSICS_H */
