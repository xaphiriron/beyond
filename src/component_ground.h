#ifndef XPH_COMPONENT_GROUND_H
#define XPH_COMPONENT_GROUND_H

#include "xph_memory.h"
#include "vector.h"

#include "object.h"
#include "entity.h"

#include "hex.h"

// the argument in a "GROUND_EDGE_TRAVERSAL" entity message is a pointer to
// this struct.
struct ground_edge_traversal
{
	Entity
		oldGroundEntity,
		newGroundEntity;
	unsigned short
		rotIndex,				// rotation relative to the old ground map
		directionOfMovement;	// the direction moved from the old ground map
};

#include "component_position.h"

typedef struct ground_comp * GroundMap;
typedef struct ground_location * GroundLoc;

// link a<->b with the given direction and rotation (relative to a)
bool ground_link (Entity a, Entity b, int direction, int rotation);
Entity ground_getEdgeConnection (const GroundMap m, short i);
unsigned short ground_getEdgeRotation (const GroundMap m, short i);

VECTOR3 ground_distanceBetweenAdjacentGrounds (int size, int direction);
GroundLoc ground_calculateLocationDistance (const GroundMap g, int edgesPassed, ...);
short ground_getMapSize (const GroundMap g);
struct hex * ground_getHexAtOffset (GroundMap g, int o);
short ground_getTileAdjacencyIndex (const Entity groundEntity, short r, short k, short i);

/***
 *  given an entity with a ground component and a position from the
 * groundmap's origin that is outside the bounds of the groundmap, updates the
 * entity's position component.
 *  returns TRUE if entity was updated successfully or didn't need updating,
 * or FALSE if there is no valid update possible (no adjacent ground)
 */
bool ground_bridgeConnections (const Entity groundEntity, Entity e);

void ground_bakeTiles (Entity g_entity);
struct hex * ground_getHexatCoord (GroundMap g, short r, short k, short i);

void ground_initSize (GroundMap g, int size);
void ground_fillFlat (GroundMap g, float height);

bool ground_isInitialized (const GroundMap g);
bool ground_isValidRKI (const GroundMap g, short r, short k, short i);

int component_ground (Object * obj, objMsg msg, void * a, void * b);

#endif