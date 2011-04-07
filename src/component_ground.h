#ifndef XPH_COMPONENT_GROUND_H
#define XPH_COMPONENT_GROUND_H

#include "xph_memory.h"
#include "vector.h"

#include "object.h"
#include "entity.h"

#include "hex.h"
#include "hex_utility.h"
#include "world_position.h"

#include <SDL/SDL_opengl.h>

// this is the dumbest naming scheme but 1) I don't remember the mapping from
// offsets to directions, 2) it doesn't make sense to call it "north",
// "northwest", etc anyway since there are actual poles, and 3) the only value
// of import here is the GROUND_FAR one, which would be used for teleportation
//  - xph 2011-03-16
enum ground_edges
{
	GROUND_EDGE0,
	GROUND_EDGE1,
	GROUND_EDGE2,
	GROUND_EDGE3,
	GROUND_EDGE4,
	GROUND_EDGE5,
	GROUND_FAR,
};

// the argument in a "GROUND_CHANGE" entity message is a pointer to
// this struct.
struct ground_change
{
	Entity
		oldGround,
		newGround;
	enum ground_edges
		dir;
};

#include "component_position.h"
#include "component_plant.h"
#include "worldgen.h"

typedef struct ground_comp * GroundMap;
/*
typedef struct ground_location * GroundLoc;
*/

struct ground_occupant
{
	Entity
		occupant;
	short
		r, k, i;
};

void groundWorld_placePlayer ();

unsigned int groundWorld_getPoleRadius ();
unsigned int groundWorld_getGroundRadius ();
unsigned int groundWorld_getDrawDistance ();

Entity groundWorld_getEntityOrigin (const Entity e);
void groundWorld_updateEntityOrigin (const Entity e, Entity newOrigin);

Entity groundWorld_loadGroundAt (const worldPosition wp);
Entity groundWorld_getGroundAt (const worldPosition wp);
void groundWorld_pruneDistantGrounds ();



Dynarr ground_getOccupants (GroundMap m);

short ground_getMapSize (const GroundMap g);
const worldPosition ground_getWorldPos (const GroundMap g);
struct hex * ground_getHexAtOffset (GroundMap g, int o);
short ground_getTileAdjacencyIndex (const Entity groundEntity, short r, short k, short i);
GLuint ground_getDisplayList (const GroundMap g);
void ground_setDisplayList (GroundMap g, GLuint list);

void ground_setWorldPos (GroundMap g, worldPosition wp);

/***
 * given an entity, [e], and the ground it's placed on, [groundEntity], calculates and returns a new ground if [e]'s position vector is out of bounds. returns groundEntity if the position vector isn't out of bounds
 */
Entity ground_bridgeConnections (const Entity groundEntity, Entity e);
bool ground_placeOnTile (Entity groundEntity, short r, short k, short i, Entity e);
bool ground_removeOccupant (Entity groundEntity, const Entity e);

void ground_bakeInternalTiles (Entity g_entity);
void ground_bakeEdgeTiles (Entity g_entity, unsigned int edge, Entity adj_entity);
//void ground_bakeTiles (Entity g_entity);
struct hex * ground_getHexatCoord (GroundMap g, short r, short k, short i);

void ground_initSize (GroundMap g);
void ground_fillFlat (GroundMap g);

bool ground_isInitialized (const GroundMap g);
bool ground_isValidRKI (const GroundMap g, short r, short k, short i);


unsigned int ground_entDistance (const Entity a, const Entity b);

void groundWorld_groundLoad (TIMER t, Component c);
unsigned char groundWorld_groundWeigh (Component c);

bool groundWorld_groundFileExists (const worldPosition wp);
Entity groundWorld_queueLoad (const worldPosition wp);
Entity groundWorld_queueGeneration (const worldPosition wp);

int component_ground (Object * obj, objMsg msg, void * a, void * b);

#endif
