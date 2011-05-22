#ifndef XPH_COMPONENT_CAMERA_H
#define XPH_COMPONENT_CAMERA_H

#include "quaternion.h"

#include "entity.h"
#include "object.h"
#include "video.h"

#include "component_position.h"

enum camera_modes
{
	CAMERA_INVALID,
	CAMERA_FIRST_PERSON,
	CAMERA_THIRD_PERSON,
	CAMERA_ISOMETRIC
};

typedef struct camera_data * cameraComponent;

const float * camera_getMatrix (Entity e);
enum camera_modes camera_getMode (Entity camera);
float camera_getHeading (Entity e);
float camera_getPitch (Entity e);
float camera_getRoll (Entity e);

const Entity camera_getActiveCamera ();

void camera_attachToTarget (Entity camera, Entity target);
void camera_setAsActive (Entity e);
void camera_switchMode (Entity camera, enum camera_modes mode);

void camera_updateTargetPositionData (Entity camera);
// azimuth is pitch from horizon; 0 means perpendicular with up vector, 90 means the opposite of the up vector, -90 means equal to the up vector. rotation is relative to the object's forward vector; 0 means equal to the object's forward vector, 180 means the opposite of it, and 90 and -90 are perpendicular.
// in first-person mode distance and rotation are clamped to 0 (and the target's orientation is used to calculate azimuth)
// in third-person mode rotation is clamped to 0 (and the target's orientation is used to calculate azimuth??)
// in isometric mode azimuth is clamped to 45 (or maybe 30-60)
void camera_setCameraOffset (Entity camera, float azimuth, float rotation, float distance);

void camera_update (Entity e);
void camera_updatePosition (Entity camera);
int component_camera (Object * obj, objMsg msg, void * a, void * b);


void camera_drawCursor ();

#endif /* XPH_COMPONENT_CAMERA_H */
