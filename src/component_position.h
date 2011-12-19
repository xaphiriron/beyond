#ifndef XPH_COMPONENT_POSITION_H
#define XPH_COMPONENT_POSITION_H

#include "xph_memory.h"
#include "vector.h"
#include "entity.h"
#include "quaternion.h"
#include "hex_utility.h"

#include "map.h"

typedef struct position_data * POSITION;

typedef struct position_update
{
	SUBHEX
		oldGround,
		newGround;
	RELATIVEHEX
		relPosition;
	VECTOR3
		difference;
} * POSITIONUPDATE;

typedef struct axes
{
	VECTOR3
		front,
		side,
		up;
} AXES;

#include "component_input.h"

void position_define (EntComponent position, EntSpeech speech);

void position_set (Entity e, hexPos pos);
hexPos position_get (Entity e);



void position_unset (Entity e);
void position_setDirect (Entity e, VECTOR3 pos, SUBHEX ground);
bool position_move (Entity e, VECTOR3 move);
// moves target to source
void position_copy (Entity target, const Entity source);

void position_setOrientation (Entity e, const QUAT q);

void position_rotateOnMouseInput (Entity e, const struct input_event * ie);

float position_getHeading (const Entity e);
float position_getPitch (const Entity e);
float position_getRoll (const Entity e);
float position_getHeadingR (const POSITION p);
float position_getPitchR (const POSITION p);
float position_getRollR (const POSITION p);

bool position_getCoordOffset (const Entity e, signed int * xp, signed int * yp);
bool position_getCoordOffsetR (const POSITION p, signed int * xp, signed int * yp);
VECTOR3 position_getLocalOffset (const Entity e);
VECTOR3 position_getLocalOffsetR (const POSITION p);
QUAT position_getOrientation (const Entity e);
QUAT position_getOrientationR (const POSITION p);
SUBHEX position_getGround (const Entity e);
SUBHEX position_getGroundR (const POSITION p);
const AXES * const position_getViewAxes (const Entity e);
const AXES * const position_getViewAxesR (const POSITION p);
const AXES * const position_getMoveAxes (const Entity e);
const AXES * const position_getMoveAxesR (const POSITION p);


void position_getHex (EntComponent position, EntSpeech speech);
void position_getHexAngle (EntComponent position, EntSpeech speech);


#endif /* XPH_COMPONENT_POSITION_H */