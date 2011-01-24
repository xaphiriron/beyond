#ifndef XPH_COMPONENT_GROUND_H
#define XPH_COMPONENT_GROUND_H

#include "xph_memory.h"
#include "vector.h"

#include "object.h"
#include "entity.h"

#include "hex.h"
#include "hex_utility.h"

#include "world_position.h"

// the argument in a "GROUND_EDGE_TRAVERSAL" entity message is a pointer to
// this struct.
struct ground_edge_traversal
{
	Entity
		oldGroundEntity,
		newGroundEntity;
	unsigned short
		directionOfMovement;	// the direction moved from the old ground map
};

#include "component_position.h"
#include "component_plant.h"

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

void ground_setWorldPos (GroundMap g, worldPosition wp);

/***
 *  given an entity with a ground component and a position from the
 * groundmap's origin that is outside the bounds of the groundmap, updates the
 * entity's position component.
 *  returns TRUE if entity was updated successfully or didn't need updating,
 * or FALSE if there is no valid update possible (no adjacent ground)
 */
bool ground_bridgeConnections (const Entity groundEntity, Entity e);
bool ground_placeOnTile (Entity groundEntity, short r, short k, short i, Entity e);

void ground_bakeInternalTiles (Entity g_entity);
void ground_bakeEdgeTiles (Entity g_entity, unsigned int edge, Entity adj_entity);
//void ground_bakeTiles (Entity g_entity);
struct hex * ground_getHexatCoord (GroundMap g, short r, short k, short i);

void ground_initSize (GroundMap g, int size);
void ground_fillFlat (GroundMap g, float height);

bool ground_isInitialized (const GroundMap g);
bool ground_isValidRKI (const GroundMap g, short r, short k, short i);


unsigned int ground_entDistance (const Entity a, const Entity b);


void groundWorld_patternLoad (Component c);
void groundWorld_groundLoad (Component c);
unsigned char groundWorld_patternWeigh (Component c);
unsigned char groundWorld_groundWeigh (Component c);

bool groundWorld_groundFileExists (const worldPosition wp);
Entity groundWorld_queueLoad (const worldPosition wp);
Entity groundWorld_queueGeneration (const worldPosition wp);

int component_ground (Object * obj, objMsg msg, void * a, void * b);
// this is a placeholder:
int component_pattern (Object * obj, objMsg msg, void * a, void * b);

#endif
