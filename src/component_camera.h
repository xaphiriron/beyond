#ifndef XPH_COMPONENT_CAMERA_H
#define XPH_COMPONENT_CAMERA_H

#include "entity.h"
#include "object.h"

enum camera_modes
{
	CAMERA_INVALID,
	CAMERA_FIRST_PERSON,
	CAMERA_THIRD_PERSON,
	CAMERA_ISOMETRIC
};

typedef struct camera_data * cameraComponent;

enum camera_modes camera_getMode (Entity camera);
float camera_getHeading (Entity e);
float camera_getPitch (Entity e);
float camera_getRoll (Entity e);

const Entity camera_getActiveCamera ();


void camera_classInit (EntComponent camera, void * arg);
void camera_classDestroy (EntComponent camrea, void * arg);

void component_cameraInitialize (EntComponent camera, void * arg);
void component_cameraDestroy (EntComponent camera, void * arg);

void component_cameraActivate (EntComponent camera, void * arg);
void component_cameraSetTarget (EntComponent camera, void * arg);

void component_cameraControlResponse (EntComponent camera, void * arg);
void component_cameraOrientResponse (EntComponent camera, void * arg);
void component_cameraPositionResponse (EntComponent camera, void * arg);

void component_cameraGetMatrix (EntComponent camera, void * arg);

#endif /* XPH_COMPONENT_CAMERA_H */
