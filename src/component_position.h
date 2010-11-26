#ifndef XPH_COMPONENT_POSITION_H
#define XPH_COMPONENT_POSITION_H

#include "xph_memory.h"
#include "vector.h"
#include "world.h"
#include "object.h"
#include "entity.h"

typedef struct position_data * positionComponent;

#include "component_ground.h"

typedef struct axes {
  VECTOR3
    forward,
    side,
    up;
} AXES;

struct position_data {
  AXES
    orient;
  VECTOR3 pos;		// distance from the center of the given map
  Entity mapEntity;
};


void position_set (Entity e, VECTOR3 pos, Entity mapEntity);
bool position_move (Entity e, VECTOR3 move);


// (rotation around world y axis, which is "up")
bool position_rotateAroundGround (Entity e, float rotation);
void position_updateOnEdgeTraversal (Entity e, struct ground_edge_traversal * t);

VECTOR3 position_getLocalOffset (const positionComponent p);
Entity position_getGroundEntity (const positionComponent p);

int component_position (Object * obj, objMsg msg, void * a, void * b);


#endif /* XPH_COMPONENT_POSITION_H */