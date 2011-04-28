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


void position_destroy (Entity e);

void position_unset (Entity e);
void position_set (Entity e, VECTOR3 pos, Entity mapEntity);
bool position_move (Entity e, VECTOR3 move);
// moves target to source
void position_copy (Entity target, const Entity source);

void position_setOrientation (Entity e, const QUAT q);

void position_updateAxesFromOrientation (Entity e);
void position_rotateOnMouseInput (Entity e, const struct input_event * ie);

/* this isn't used anymore; it's a relic from the days when grounds were connected in a graph
void position_updateOnEdgeTraversal (Entity e, struct ground_edge_traversal * t);
*/

float position_getHeading (const Entity e);
float position_getPitch (const Entity e);
float position_getRoll (const Entity e);
float position_getHeadingR (const positionComponent p);
float position_getPitchR (const positionComponent p);
float position_getRollR (const positionComponent p);

VECTOR3 position_getLocalOffset (const Entity e);
QUAT position_getOrientation (const Entity e);
Entity position_getGroundEntity (const Entity e);
VECTOR3 position_getLocalOffsetR (const positionComponent p);
QUAT position_getOrientationR (const positionComponent p);
Entity position_getGroundEntityR (const positionComponent p);

int component_position (Object * obj, objMsg msg, void * a, void * b);


#endif /* XPH_COMPONENT_POSITION_H */