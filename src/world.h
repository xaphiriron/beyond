#ifndef WORLD_H
#define WORLD_H

#include <SDL/SDL_opengl.h>
#include "xph_memory.h"
#include "entity.h"
#include "vector.h"

extern ENTITY * WorldEntity;

typedef struct world {
  VECTOR3 origin;
} WORLD;

WORLD * world_create ();
void world_destroy (WORLD *);

int world_handler (ENTITY * e, eMessage msg, void * a, void * b);

#endif /* WORLD_H */