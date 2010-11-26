#ifndef XPH_COMPONENT_CAMERA_H
#define XPH_COMPONENT_CAMERA_H

#include "entity.h"
#include "object.h"

#include "component_position.h"
#include "component_ground.h"
#include "ground_draw.h"

typedef struct camera_data * cameraComponent;

const float * camera_getMatrix (Entity e);

void camera_update (Entity e);
int component_camera (Object * obj, objMsg msg, void * a, void * b);

#endif /* XPH_COMPONENT_CAMERA_H */
