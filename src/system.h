#ifndef SYSTEM_H
#define SYSTEM_H

#include "object.h"
#include "video.h"
#include "world.h"
#include "timer.h"
#include "accumulator.h"

#include "component_ground.h"
#include "component_plant.h"
#include "component_input.h"
#include "component_camera.h"
#include "component_walking.h"

extern Object * SystemObject;

typedef struct system
{
	float
		timer_mult,
		timestep;
	CLOCK
		* clock;
	ACCUMULATOR
		* acc;
	enum system_states
	{
		STATE_ERROR				= 0x0000,
		STATE_INIT 				= 0x0001,
		STATE_TYPING			= 0x0002,
		STATE_FIRSTPERSONVIEW	= 0x0004,
		STATE_THIRDPERSONVIEW	= 0x0008,
		// ...
		STATE_QUIT				= 0x8000
	} state;

	bool
		quit;
} SYSTEM;

SYSTEM * system_create ();
void system_destroy (SYSTEM *);

enum system_states system_getState (const SYSTEM * s);
bool system_setState (SYSTEM * s, enum system_states state);

int system_handler (Object * o, objMsg msg, void * a, void * b);

#endif /* SYSTEM_H */