#ifndef XPH_TURTLE3D_H
#define XPH_TURTLE3D_H

#include <stdio.h>

#include "xph_memory.h"
#include "xph_log.h"
#include "vector.h"
#include "quaternion.h"
#include "dynarr.h"

typedef struct xph_turtle * Turtle;

typedef struct xph_turtlePath
{
	VECTOR3
		position;
	bool
		broken;
} * TurtlePath;

enum turtle_types {
	TURTLE_INVALID = 0,
	TURTLE_2D,
	TURTLE_3D,
	TURTLE_HEXGRID,
};

typedef enum turtle_pentypes {
	TURTLE_PENUP,
	TURTLE_PENDOWN
} TURTLEPENTYPE;

Turtle turtleCreate (enum turtle_types type);
void turtleDestroy (Turtle t);

VECTOR3 turtleGetPosition (const Turtle t);
VECTOR3 turtleGetHeadingVector (const Turtle t);
VECTOR3 turtleGetPitchVector (const Turtle t);
VECTOR3 turtleGetRollVector (const Turtle t);

TURTLEPENTYPE turtleGetPen (const Turtle t);
Dynarr turtleGetPath (const Turtle t);



void turtleMoveForward (Turtle t, float m);
void turtleMoveUp (Turtle t, float m);
void turtleMoveRight (Turtle t, float m);
#define turtleMoveBackward(t,m)		turtleMoveForward(t,(m)*-1)
#define turtleMoveDown(t,m)			turtleMoveUp(t,(m)*-1)
#define turtleMoveLeft(t,m)			turtleMoveRight(t,(m)*-1)

void turtleLookDown (Turtle t, float m);
void turtleTurnRight (Turtle t, float m);
void turtleTwistCounterclockwise (Turtle t, float m);
#define turtleLookUp(t,m)			turtleLookDown(t,(m)*-1)
#define turtleTurnLeft(t,m)			turtleTurnRight(t,(m)*-1)
#define turtleTwistClockwise(t,m)	turtleTwistCounterclockwise(t,(m)*-1)

void turtlePushStack (Turtle t);
void turtlePopStack (Turtle t);

void turtlePenDown (Turtle t);
void turtlePenUp (Turtle t);

#endif /* XPH_TURTLE3D_H */