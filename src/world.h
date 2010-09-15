#ifndef WORLD_H
#define WORLD_H

#include <SDL/SDL_opengl.h>
#include "xph_memory.h"
#include "object.h"
#include "vector.h"
#include "hex.h"
#include "physics.h"

#include "entity.h"

#include "component_position.h"
#include "component_integrate.h"
#include "component_camera.h"
#include "component_collide.h"
#include "component_input.h"
#include "component_walking.h"

extern Object * WorldObject;

typedef struct world {
  VECTOR3 origin;
  MAP * map;

  // this is not how we should do things, but it's good enough for now.
  Entity * camera;
} WORLD;

WORLD * world_create ();
void world_destroy (WORLD *);

/*
void integrate (WORLD * w, float delta);
int interpenetrating (const WORLD * w);
int colliding (const WORLD * w);
void commitState (WORLD * w);
 */

int world_handler (Object * o, objMsg msg, void * a, void * b);

#endif /* WORLD_H */