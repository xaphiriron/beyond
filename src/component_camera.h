/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

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

void camera_define (EntComponent camera, EntSpeech speech);

void cameraRender_system (Dynarr entities);

#endif /* XPH_COMPONENT_CAMERA_H */
