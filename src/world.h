#ifndef WORLD_H
#define WORLD_H

#include <SDL/SDL_opengl.h>
#include "xph_memory.h"
#include "vector.h"
#include "hex.h"
#include "physics.h"

#include "object.h"
#include "entity.h"

#include "component_position.h"
#include "component_ground.h"
#include "component_integrate.h"
#include "component_camera.h"
#include "component_input.h"
#include "component_walking.h"

#include "ground_draw.h"
#include "camera_draw.h"
#include "system.h"

extern Object * WorldObject;

typedef struct world
{
	Entity
		groundOrigin,
		camera;		// this is dumb, in the end we don't want a "camera" here. probably.
	int groundDistanceDraw;
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