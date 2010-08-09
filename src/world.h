#ifndef WORLD_H
#define WORLD_H

#include <SDL/SDL_opengl.h>
#include "xph_memory.h"
#include "object.h"
#include "vector.h"

extern Object * WorldObject;

typedef struct world {
  VECTOR3 origin;
} WORLD;

WORLD * world_create ();
void world_destroy (WORLD *);

int world_handler (Object * o, objMsg msg, void * a, void * b);

#endif /* WORLD_H */