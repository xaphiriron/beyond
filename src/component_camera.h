#ifndef XPH_COMPONENT_CAMERA_H
#define XPH_COMPONENT_CAMERA_H

#include "entity.h"

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


void camera_classInit (EntComponent camera, EntSpeech speech);
void camera_classDestroy (EntComponent camrea, EntSpeech speech);

void component_cameraInitialize (EntComponent camera, EntSpeech speech);
void component_cameraDestroy (EntComponent camera, EntSpeech speech);

void component_cameraActivate (EntComponent camera, EntSpeech speech);
void component_cameraSetTarget (EntComponent camera, EntSpeech speech);

void component_cameraControlResponse (EntComponent camera, EntSpeech speech);
void component_cameraOrientResponse (EntComponent camera, EntSpeech speech);
void component_cameraPositionResponse (EntComponent camera, EntSpeech speech);

void component_cameraGetMatrix (EntComponent camera, EntSpeech speech);
void component_cameraGetTargetMatrix (EntComponent camera, EntSpeech speech);

#endif /* XPH_COMPONENT_CAMERA_H */
