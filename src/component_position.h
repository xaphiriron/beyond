#ifndef XPH_COMPONENT_POSITION_H
#define XPH_COMPONENT_POSITION_H

#include "xph_memory.h"
#include "vector.h"
#include "object.h"
#include "entity.h"
#include "quaternion.h"
#include "hex_utility.h"

typedef struct position_data * positionComponent;

#include "component_ground.h"
#include "component_input.h"

typedef struct axes
{
	VECTOR3
		front,
		side,
		up;
} AXES;

struct position_data {
	AXES
		view,
		move;
	QUAT
		orientation;
	VECTOR3
		pos;		// distance from the center of the given map
	float
		sensitivity;
	Entity
		mapEntity;
	bool
		dirty;		// view axes don't match the orientation quaternion
};


void position_set (Entity e, VECTOR3 pos, Entity mapEntity);
bool position_move (Entity e, VECTOR3 move);

void position_updateAxesFromOrientation (Entity e);
void position_rotateOnMouseInput (Entity e, const struct input_event * ie);

// (rotation around world y axis, which is "up")
bool position_rotateAroundGround (Entity e, float rotation);
void position_updateOnEdgeTraversal (Entity e, struct ground_edge_traversal * t);

VECTOR3 position_getLocalOffset (const Entity e);
Entity position_getGroundEntity (const positionComponent p);

int component_position (Object * obj, objMsg msg, void * a, void * b);


#endif /* XPH_COMPONENT_POSITION_H */