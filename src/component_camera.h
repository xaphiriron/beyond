#ifndef XPH_COMPONENT_CAMERA_H
#define XPH_COMPONENT_CAMERA_H

#include "quaternion.h"

#include "entity.h"
#include "object.h"

#include "component_position.h"
#include "component_ground.h"
#include "ground_draw.h"

typedef struct camera_data * cameraComponent;

const float * camera_getMatrix (Entity e);
float camera_getHeading (Entity e);
float camera_getPitch (Entity e);
float camera_getRoll (Entity e);

void camera_setAsActive (Entity e);

void camera_updateLabelsFromEdgeTraversal (Entity e, struct ground_edge_traversal * t);

void camera_update (Entity e);
int component_camera (Object * obj, objMsg msg, void * a, void * b);

#endif /* XPH_COMPONENT_CAMERA_H */
