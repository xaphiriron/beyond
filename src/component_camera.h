#ifndef XPH_COMPONENT_CAMERA_H
#define XPH_COMPONENT_CAMERA_H

#include "entity.h"
#include "object.h"

#include "component_position.h"

struct camera_data {
  float m[16];
/* right now the view matrix is based entirely on the orientation and position of the "position" component. So it's impossible to do something like LOOK UP or roll the camera. if we store view axes then we still need a way for cameras to get input messages in a way that doesn't tether the camera to the input system or vice/versa. (MESSAGES are obviously the way to do it, but etc etc that's its own whole thing that I don't know how to work yet)
 *
  AXES
    view;
 */
};

void updateCamera (Entity * e);
int component_camera (Object * obj, objMsg msg, void * a, void * b);

#endif /* XPH_COMPONENT_CAMERA_H */
