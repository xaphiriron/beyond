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
#include "component_plant.h"

#include "ground_draw.h"
#include "camera_draw.h"
#include "system.h"

extern Object * WorldObject;

typedef struct world
{
	unsigned int
		poleRadius,
		groundRadius;

	// the hypothetical max size of this array is hex (poleRadius)
	Dynarr
		loadedGrounds;

/*
	(other data that has to do with map generation or storage should go here, but nothing else. most specifically, this is not the place for video options or references to the player-controlled entities)
*/

	Entity
		groundOrigin,
		camera;		// this is dumb, in the end we don't want a "camera" here. probably.

// these are video options and shouldn't be in the world object:
	int
		groundDistanceDraw;
	bool
		renderWireframe;

} WORLD;

WORLD * world_create ();
void world_destroy (WORLD *);

void world_update ();
void world_postupdate ();

unsigned int world_getLoadedGroundCount ();
unsigned int world_getPoleRadius ();
Entity world_loadGroundAt (const worldPosition wp);

/*
void integrate (WORLD * w, float delta);
int interpenetrating (const WORLD * w);
int colliding (const WORLD * w);
void commitState (WORLD * w);
 */

int world_handler (Object * o, objMsg msg, void * a, void * b);

#endif /* WORLD_H */