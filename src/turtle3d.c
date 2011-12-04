#include "turtle3d.h"

/*
 the turtle needs to keep an internal coordinate system -- in two dimensions you can get away with having only a forward vector and constructing an orthagonal left/right vector as needed, but in three dimensions you have to have at least two and since I know the math less well I might as well store the entire local coordinate system
 */

struct xph_turtle
{
	VECTOR3
		position;
	QUAT
		orientation;
	float
		matrix[16];
	Dynarr
		stack,
		path;
	TURTLEPENTYPE
		pen;
	bool
		penUpSinceLastMove;
};

struct xph_turtle_stack
{
	VECTOR3
		position;
	QUAT
		orientation;
};

static void turtleAddPoint (Turtle t);

Turtle turtleCreate (enum turtle_types type)
{
	Turtle
		t;
	switch (type)
	{
		case TURTLE_2D:
		case TURTLE_3D:
		case TURTLE_HEXGRID:
			break;
		case TURTLE_INVALID:
		default:
			return NULL;
	}
	t = xph_alloc (sizeof (struct xph_turtle));
	t->position = vectorCreate (0.0, 0.0, 0.0);
	t->orientation = quat_create (1.0, 0.0, 0.0, 0.0);
	quat_quatToMatrixf (&t->orientation, t->matrix);
	t->stack = dynarr_create (2, sizeof (struct xph_turtle_stack *));
	t->path = dynarr_create (2, sizeof (VECTOR3 *));
	t->pen = TURTLE_PENDOWN;
	t->penUpSinceLastMove = false;
/*
	printf ("\33[35;1mMATRIX\n");
	printf ("%4.2f %4.2f %4.2f %4.2f\n", t->matrix[0], t->matrix[4], t->matrix[8], t->matrix[12]);
	printf ("%4.2f %4.2f %4.2f %4.2f\n", t->matrix[1], t->matrix[5], t->matrix[9], t->matrix[13]);
	printf ("%4.2f %4.2f %4.2f %4.2f\n", t->matrix[2], t->matrix[6], t->matrix[10], t->matrix[14]);
	printf ("%4.2f %4.2f %4.2f %4.2f\33[0m\n\n", t->matrix[3], t->matrix[7], t->matrix[11], t->matrix[15]);
*/
	return t;
}

void turtleDestroy (Turtle t)
{
	if (t->stack != NULL)
	{
		dynarr_wipe (t->stack, xph_free);
		dynarr_destroy (t->stack);
	}
	dynarr_wipe (t->path, xph_free);
	dynarr_destroy (t->path);
	xph_free (t);
}

VECTOR3 turtleGetPosition (const Turtle t)
{
	return t->position;
}

VECTOR3 turtleGetHeadingVector (const Turtle t)
{
	return vectorCreate (t->matrix[0], t->matrix[4], t->matrix[8]);
}

VECTOR3 turtleGetPitchVector (const Turtle t)
{
	return vectorCreate (t->matrix[1], t->matrix[5], t->matrix[9]);
}

VECTOR3 turtleGetRollVector (const Turtle t)
{
	return vectorCreate (t->matrix[2], t->matrix[6], t->matrix[10]);
}

TURTLEPENTYPE turtleGetPen (const Turtle t)
{
	return t->pen;
}

Dynarr turtleGetPath (const Turtle t)
{
	return t->path;
}



void turtleMoveForward (Turtle t, float m)
{
	VECTOR3
		forward = turtleGetHeadingVector (t),
		move = vectorMultiplyByScalar (&forward, m);
	t->position = vectorAdd (&t->position, &move);
	if (t->pen == TURTLE_PENDOWN)
		turtleAddPoint (t);
}

void turtleMoveUp (Turtle t, float m)
{
	VECTOR3
		up = turtleGetPitchVector (t),
		move = vectorMultiplyByScalar (&up, m);
	t->position = vectorAdd (&t->position, &move);
	if (t->pen == TURTLE_PENDOWN)
		turtleAddPoint (t);
}

void turtleMoveRight (Turtle t, float m)
{
	VECTOR3
		right = turtleGetRollVector (t),
		move = vectorMultiplyByScalar (&right, m);
	t->position = vectorAdd (&t->position, &move);
	if (t->pen == TURTLE_PENDOWN)
		turtleAddPoint (t);
}

void turtleLookDown (Turtle t, float m)
{
	QUAT
		q = quat_eulerToQuat (
			0, 0, m
		);
	t->orientation = quat_multiply (&q, &t->orientation);
	quat_quatToMatrixf (&t->orientation, t->matrix);
}

void turtleTurnRight (Turtle t, float m)
{
	QUAT
		q = quat_eulerToQuat (
			0, m, 0
		);
	t->orientation = quat_multiply (&q, &t->orientation);
	quat_quatToMatrixf (&t->orientation, t->matrix);
}

void turtleTwistCounterclockwise (Turtle t, float m)
{
	QUAT
		q = quat_eulerToQuat (
			m, 0, 0
		);
	t->orientation = quat_multiply (&q, &t->orientation);
	quat_quatToMatrixf (&t->orientation, t->matrix);
}

void turtlePushStack (Turtle t)
{
	struct xph_turtle_stack
		* s = xph_alloc (sizeof (struct xph_turtle_stack));
	s->position = t->position;
	s->orientation = t->orientation;
	dynarr_push (t->stack, s);
}

void turtlePopStack (Turtle t)
{
	struct xph_turtle_stack
		* s = *(struct xph_turtle_stack **)dynarr_pop (t->stack);
	if (s == NULL)
		return;
	t->position = s->position;
	t->orientation = s->orientation;
	quat_quatToMatrixf (&t->orientation, t->matrix);
	xph_free (s);
}


void turtlePenDown (Turtle t)
{
	t->pen = TURTLE_PENDOWN;
}

void turtlePenUp (Turtle t)
{
	t->pen = TURTLE_PENUP;
	t->penUpSinceLastMove = true;
}



static void turtleAddPoint (Turtle t)
{
	TurtlePath
		p = xph_alloc (sizeof (struct xph_turtlePath));
	p->broken = t->penUpSinceLastMove;
	t->penUpSinceLastMove = false;
	p->position = t->position;
	dynarr_push (t->path, p);
}
