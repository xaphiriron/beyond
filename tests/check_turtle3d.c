#include "../src/turtle3d.h"
#include "../src/vector.h"
#include <check.h>
#include <stdlib.h>

START_TEST (turtleCreateTest)
{
	Turtle t = turtleCreate (TURTLE_3D);
	fail_unless (t != NULL);
	turtleDestroy (t);
}
END_TEST


Turtle t = NULL;
void turtleBasicSetup (void)
{
	t = turtleCreate (TURTLE_3D);
}

void turtleBasicTeardown (void)
{
	turtleDestroy (t);
}

START_TEST (turtleDefaultPositionTest)
{
	VECTOR3
		dflt = vectorCreate (0.0, 0.0, 0.0),
		pos = turtleGetPosition (t);
	fail_unless (
		vector_cmp (&dflt, &pos),
		"Default position should be 0,0,0; got %f.0,%f.0,%f.0 instead",
		pos.x, pos.y, pos.z
	);
}
END_TEST

START_TEST (turtleDefaultHeadingTest)
{
	VECTOR3
		dflt = vectorCreate (1.0, 0.0, 0.0),
		heading = turtleGetHeadingVector (t);
	fail_unless (
		vector_cmp (&dflt, &heading),
		"Default heading should be 1,0,0; got %f.0,%f.0,%f.0 instead",
		heading.x, heading.y, heading.z
	);
}
END_TEST

START_TEST (turtleDefaultPitchTest)
{
	VECTOR3
		dflt = vectorCreate (0.0, 1.0, 0.0),
		pitch = turtleGetPitchVector (t);
	fail_unless (
		vector_cmp (&dflt, &pitch),
		"Default pitch should be 0,1,0; got %f.0,%f.0,%f.0 instead",
		pitch.x, pitch.y, pitch.z
	);
}
END_TEST

START_TEST (turtleDefaultRollTest)
{
	VECTOR3
		dflt = vectorCreate (0.0, 0.0, 1.0),
		roll = turtleGetRollVector (t);
	fail_unless (
		vector_cmp (&dflt, &roll),
		"Default roll should be 0,0,1; got %4.2f, %4.2f, %4.2f instead",
		roll.x, roll.y, roll.z
	);
}
END_TEST

START_TEST (turtleMoveForwardTest)
{
	VECTOR3
		move = vectorCreate (5.0, 0.0, 0.0),
		pos;
	turtleMoveForward (t, 5.0);
	pos = turtleGetPosition (t);
	fail_unless (
		vector_cmp (&move, &pos),
		"From default position expected movement forward to end with turtle at 5,0,0; instead was at %4.2f, %4.2f, %4.2f",
		pos.x, pos.y, pos.z
	);
}
END_TEST

START_TEST (turtleMoveBackwardTest)
{
	VECTOR3
		move = vectorCreate (-5.0, 0.0, 0.0),
		pos;
	turtleMoveBackward (t, 5.0);
	pos = turtleGetPosition (t);
	fail_unless (
		vector_cmp (&move, &pos),
		"From default position expected movement backward to end with turtle at -5,0,0; instead was at %4.2f, %4.2f, %4.2f",
		pos.x, pos.y, pos.z
	);
}
END_TEST

START_TEST (turtleMoveUpTest)
{
	VECTOR3
		move = vectorCreate (0.0, 5.0, 0.0),
		pos;
	turtleMoveUp (t, 5.0);
	pos = turtleGetPosition (t);
	fail_unless (
		vector_cmp (&move, &pos),
		"From default position expected movement up to end with turtle at 0,5,0; instead was at %4.2f, %4.2f, %4.2f",
		pos.x, pos.y, pos.z
	);
}
END_TEST

START_TEST (turtleMoveDownTest)
{
	VECTOR3
		move = vectorCreate (0.0, -5.0, 0.0),
		pos;
	turtleMoveDown (t, 5.0);
	pos = turtleGetPosition (t);
	fail_unless (
		vector_cmp (&move, &pos),
		"From default position expected movement down to end with turtle at 0,-5,0; instead was at %4.2f, %4.2f, %4.2f",
		pos.x, pos.y, pos.z
	);
}
END_TEST

START_TEST (turtleMoveRightTest)
{
	VECTOR3
		move = vectorCreate (0.0, 0.0, 5.0),
		pos;
	turtleMoveRight (t, 5.0);
	pos = turtleGetPosition (t);
	fail_unless (
		vector_cmp (&move, &pos),
		"From default position expected movement right to end with turtle at 0,0,5; instead was at %4.2f, %4.2f, %4.2f",
		pos.x, pos.y, pos.z
	);
}
END_TEST

START_TEST (turtleMoveLeftTest)
{
	VECTOR3
		move = vectorCreate (0.0, 0.0, -5.0),
		pos;
	turtleMoveLeft (t, 5.0);
	pos = turtleGetPosition (t);
	fail_unless (
		vector_cmp (&move, &pos),
		"From default position expected movement left to end with turtle at 0,0,-5; instead was at %4.2f, %4.2f, %4.2f",
		pos.x, pos.y, pos.z
	);
}
END_TEST

START_TEST (turtleMoveMultipleTest)
{
	VECTOR3
		finalPos = vectorCreate (10.0, 10.0, 10.0),
		pos;
	turtleMoveUp (t, 10.0);
	turtleMoveRight (t, 10.0);
	turtleMoveForward (t, 10.0);
	pos = turtleGetPosition (t);
	fail_unless (
		vector_cmp (&finalPos, &pos),
		"Multiple movement commands should move the turtle from its prior last position; expected final position at \33[32;1m%5.2f, %5.2f, %5.2f\33[0m, instead got \33[31;1m%5.2f, %5.2f, %5.2f\33[0m",
		finalPos.x, finalPos.y, finalPos.z,
		pos.x, pos.y, pos.z
	);
}
END_TEST

START_TEST (turtleTurnLeftTest)
{
	VECTOR3
		newForward = vectorCreate (0.0, 0.0, -1.0),
		newUp = vectorCreate (0.0, 1.0, 0.0),
		newRight = vectorCreate (1.0, 0.0, 0.0),
		heading,
		pitch,
		roll;
	turtleTurnLeft (t, 90.0);
	heading = turtleGetHeadingVector (t);
	pitch = turtleGetPitchVector (t);
	roll = turtleGetRollVector (t);
/*
	printf ("\33[32;1mEXPECTED\t\t\33[33;1mGOT\n");
	printf ("\33[32;1m%5.2f %5.2f %5.2f\t\33[33;1m%5.2f %5.2f %5.2f\n", newForward.x, newForward.y, newForward.z, heading.x, heading.y, heading.z);
	printf ("\33[32;1m%5.2f %5.2f %5.2f\t\33[33;1m%5.2f %5.2f %5.2f\n", newUp.x, newUp.y, newUp.z, pitch.x, pitch.y, pitch.z);
	printf ("\33[32;1m%5.2f %5.2f %5.2f\t\33[33;1m%5.2f %5.2f %5.2f\33[0m\n\n", newRight.x, newRight.y, newRight.z, roll.x, roll.y, roll.z);
*/
	fail_unless (
		vector_cmp (&newForward, &heading),
		"From default heading, turning left 90 degrees ought to end with turtle facing 0,0,-1; instead got %4.2f, %4.2f, %4.2f",
		heading.x, heading.y, heading.z
	);
	fail_unless (
		vector_cmp (&newUp, &pitch),
		"From default heading, turning left 90 degress ought to leave pitch unchanged at 0,1,0; instead got %4.2f, %4.2f, %4.2f",
		pitch.x, pitch.y, pitch.z
	);
	fail_unless (
		vector_cmp (&newRight, &roll),
		"From default heading, turning left 90 degrees ought to end with turtle right at 1,0,0; instead got %4.2f, %4.2f, %4.2f",
		roll.x, roll.y, roll.z
	);
}
END_TEST

START_TEST (turtleTurnRightTest)
{
	VECTOR3
		newForward = vectorCreate (0.0, 0.0, 1.0),
		newUp = vectorCreate (0.0, 1.0, 0.0),
		newRight = vectorCreate (-1.0, 0.0, 0.0),
		heading,
		pitch,
		roll;
	turtleTurnRight (t, 90.0);
	heading = turtleGetHeadingVector (t);
	pitch = turtleGetPitchVector (t);
	roll = turtleGetRollVector (t);
/*
	printf ("\33[32;1mEXPECTED\t\t\33[33;1mGOT\n");
	printf ("\33[32;1m%5.2f %5.2f %5.2f\t\33[33;1m%5.2f %5.2f %5.2f\n", newForward.x, newForward.y, newForward.z, heading.x, heading.y, heading.z);
	printf ("\33[32;1m%5.2f %5.2f %5.2f\t\33[33;1m%5.2f %5.2f %5.2f\n", newUp.x, newUp.y, newUp.z, pitch.x, pitch.y, pitch.z);
	printf ("\33[32;1m%5.2f %5.2f %5.2f\t\33[33;1m%5.2f %5.2f %5.2f\33[0m\n\n", newRight.x, newRight.y, newRight.z, roll.x, roll.y, roll.z);
*/
	fail_unless (
		vector_cmp (&newForward, &heading),
		"From default angles, turning right 90 degrees ought to end with turtle forward facing %4.2f, %4.2f, %4.2f; instead got %4.2f, %4.2f, %4.2f",
		newForward.x, newForward.y, newForward.z,
		heading.x, heading.y, heading.z
	);
	fail_unless (
		vector_cmp (&newUp, &pitch),
		"From default angles, turning right 90 degress ought to end with turtle up unchanged at %4.2f, %4.2f, %4.2f; instead got %4.2f, %4.2f, %4.2f",
		newUp.x, newUp.y, newUp.z,
		pitch.x, pitch.y, pitch.z
	);
	fail_unless (
		vector_cmp (&newRight, &roll),
		"From default angles, turning right 90 degrees ought to end with turtle right facing %4.2f, %4.2f, %4.2f; instead got %4.2f, %4.2f, %4.2f",
		newRight.x, newRight.y, newRight.z,
		roll.x, roll.y, roll.z
	);
}
END_TEST

START_TEST (turtleLookUpTest)
{
	VECTOR3
		newForward = vectorCreate (0.0, 1.0, 0.0),
		newUp = vectorCreate (-1.0, 0.0, 0.0),
		newRight = vectorCreate (0.0, 0.0, 1.0),
		heading,
		pitch,
		roll;
	turtleLookUp (t, 90.0);
	heading = turtleGetHeadingVector (t);
	pitch = turtleGetPitchVector (t);
	roll = turtleGetRollVector (t);
/*
	printf ("\33[32;1mEXPECTED\t\t\33[33;1mGOT\n");
	printf ("\33[32;1m%5.2f %5.2f %5.2f\t\33[33;1m%5.2f %5.2f %5.2f\n", newForward.x, newForward.y, newForward.z, heading.x, heading.y, heading.z);
	printf ("\33[32;1m%5.2f %5.2f %5.2f\t\33[33;1m%5.2f %5.2f %5.2f\n", newUp.x, newUp.y, newUp.z, pitch.x, pitch.y, pitch.z);
	printf ("\33[32;1m%5.2f %5.2f %5.2f\t\33[33;1m%5.2f %5.2f %5.2f\33[0m\n\n", newRight.x, newRight.y, newRight.z, roll.x, roll.y, roll.z);
*/
	fail_unless (
		vector_cmp (&newForward, &heading),
		"From default angles, looking up 90 degrees ought to end with turtle forward facing %4.2f, %4.2f, %4.2f; instead got %4.2f, %4.2f, %4.2f",
		newForward.x, newForward.y, newForward.z,
		heading.x, heading.y, heading.z
	);
	fail_unless (
		vector_cmp (&newUp, &pitch),
		"From default angles, looking up 90 degress ought to end with turtle up facing %4.2f, %4.2f, %4.2f; instead got %4.2f, %4.2f, %4.2f",
		newUp.x, newUp.y, newUp.z,
		pitch.x, pitch.y, pitch.z
	);
	fail_unless (
		vector_cmp (&newRight, &roll),
		"From default angles, looking up 90 degrees ought to end with turtle right unchanged at %4.2f, %4.2f, %4.2f; instead got %4.2f, %4.2f, %4.2f",
		newRight.x, newRight.y, newRight.z,
		roll.x, roll.y, roll.z
	);
}
END_TEST

START_TEST (turtleLookDownTest)
{
	VECTOR3
		newForward = vectorCreate (0.0, -1.0, 0.0),
		newUp = vectorCreate (1.0, 0.0, 0.0),
		newRight = vectorCreate (0.0, 0.0, 1.0),
		heading,
		pitch,
		roll;
	turtleLookDown (t, 90.0);
	heading = turtleGetHeadingVector (t);
	pitch = turtleGetPitchVector (t);
	roll = turtleGetRollVector (t);
/*
	printf ("\33[32;1mEXPECTED\t\t\33[33;1mGOT\n");
	printf ("\33[32;1m%5.2f %5.2f %5.2f\t\33[33;1m%5.2f %5.2f %5.2f\n", newForward.x, newForward.y, newForward.z, heading.x, heading.y, heading.z);
	printf ("\33[32;1m%5.2f %5.2f %5.2f\t\33[33;1m%5.2f %5.2f %5.2f\n", newUp.x, newUp.y, newUp.z, pitch.x, pitch.y, pitch.z);
	printf ("\33[32;1m%5.2f %5.2f %5.2f\t\33[33;1m%5.2f %5.2f %5.2f\33[0m\n\n", newRight.x, newRight.y, newRight.z, roll.x, roll.y, roll.z);
*/
	fail_unless (
		vector_cmp (&newForward, &heading),
		"From default angles, looking down 90 degrees ought to end with turtle forward facing %4.2f, %4.2f, %4.2f; instead got %4.2f, %4.2f, %4.2f",
		newForward.x, newForward.y, newForward.z,
		heading.x, heading.y, heading.z
	);
	fail_unless (
		vector_cmp (&newUp, &pitch),
		"From default angles, looking down 90 degress ought to end with turtle up facing %4.2f, %4.2f, %4.2f; instead got %4.2f, %4.2f, %4.2f",
		newUp.x, newUp.y, newUp.z,
		pitch.x, pitch.y, pitch.z
	);
	fail_unless (
		vector_cmp (&newRight, &roll),
		"From default angles, looking down 90 degrees ought to end with turtle right unchanged at %4.2f, %4.2f, %4.2f; instead got %4.2f, %4.2f, %4.2f",
		newRight.x, newRight.y, newRight.z,
		roll.x, roll.y, roll.z
	);
}
END_TEST

START_TEST (turtleTwistClockwiseTest)
{
	VECTOR3
		newForward = vectorCreate (1.0, 0.0, 0.0),
		newUp = vectorCreate (0.0, 0.0, 1.0),
		newRight = vectorCreate (0.0, -1.0, 0.0),
		heading,
		pitch,
		roll;
	turtleTwistClockwise (t, 90.0);
	heading = turtleGetHeadingVector (t);
	pitch = turtleGetPitchVector (t);
	roll = turtleGetRollVector (t);
/*
	printf ("\33[32;1mEXPECTED\t\t\33[33;1mGOT\n");
	printf ("\33[32;1m%5.2f %5.2f %5.2f\t\33[33;1m%5.2f %5.2f %5.2f\n", newForward.x, newForward.y, newForward.z, heading.x, heading.y, heading.z);
	printf ("\33[32;1m%5.2f %5.2f %5.2f\t\33[33;1m%5.2f %5.2f %5.2f\n", newUp.x, newUp.y, newUp.z, pitch.x, pitch.y, pitch.z);
	printf ("\33[32;1m%5.2f %5.2f %5.2f\t\33[33;1m%5.2f %5.2f %5.2f\33[0m\n\n", newRight.x, newRight.y, newRight.z, roll.x, roll.y, roll.z);
*/
	fail_unless (
		vector_cmp (&newForward, &heading),
		"From default angles, twisting clockwise 90 degrees ought to end with turtle forward unchanged at %4.2f, %4.2f, %4.2f; instead got %4.2f, %4.2f, %4.2f",
		newForward.x, newForward.y, newForward.z,
		heading.x, heading.y, heading.z
	);
	fail_unless (
		vector_cmp (&newUp, &pitch),
		"From default angles, twisting clockwise 90 degress ought to end with turtle up facing %4.2f, %4.2f, %4.2f; instead got %4.2f, %4.2f, %4.2f",
		newUp.x, newUp.y, newUp.z,
		pitch.x, pitch.y, pitch.z
	);
	fail_unless (
		vector_cmp (&newRight, &roll),
		"From default angles, twisting clockwise 90 degrees ought to end with turtle right facing %4.2f, %4.2f, %4.2f; instead got %4.2f, %4.2f, %4.2f",
		newRight.x, newRight.y, newRight.z,
		roll.x, roll.y, roll.z
	);
}
END_TEST

START_TEST (turtleTwistCounterclockwiseTest)
{
	VECTOR3
		newForward = vectorCreate (1.0, 0.0, 0.0),
		newUp = vectorCreate (0.0, 0.0, -1.0),
		newRight = vectorCreate (0.0, 1.0, 0.0),
		heading,
		pitch,
		roll;
	turtleTwistCounterclockwise (t, 90.0);
	heading = turtleGetHeadingVector (t);
	pitch = turtleGetPitchVector (t);
	roll = turtleGetRollVector (t);
/*
	printf ("\33[32;1mEXPECTED\t\t\33[33;1mGOT\n");
	printf ("\33[32;1m%5.2f %5.2f %5.2f\t\33[33;1m%5.2f %5.2f %5.2f\n", newForward.x, newForward.y, newForward.z, heading.x, heading.y, heading.z);
	printf ("\33[32;1m%5.2f %5.2f %5.2f\t\33[33;1m%5.2f %5.2f %5.2f\n", newUp.x, newUp.y, newUp.z, pitch.x, pitch.y, pitch.z);
	printf ("\33[32;1m%5.2f %5.2f %5.2f\t\33[33;1m%5.2f %5.2f %5.2f\33[0m\n\n", newRight.x, newRight.y, newRight.z, roll.x, roll.y, roll.z);
*/
	fail_unless (
		vector_cmp (&newForward, &heading),
		"From default angles, twisting counter-clockwise 90 degrees ought to end with turtle forward unchanged at %4.2f, %4.2f, %4.2f; instead got %4.2f, %4.2f, %4.2f",
		newForward.x, newForward.y, newForward.z,
		heading.x, heading.y, heading.z
	);
	fail_unless (
		vector_cmp (&newUp, &pitch),
		"From default angles, twisting counter-clockwise 90 degress ought to end with turtle up facing %4.2f, %4.2f, %4.2f; instead got %4.2f, %4.2f, %4.2f",
		newUp.x, newUp.y, newUp.z,
		pitch.x, pitch.y, pitch.z
	);
	fail_unless (
		vector_cmp (&newRight, &roll),
		"From default angles, twisting counter-clockwise 90 degrees ought to end with turtle right facing %4.2f, %4.2f, %4.2f; instead got %4.2f, %4.2f, %4.2f",
		newRight.x, newRight.y, newRight.z,
		roll.x, roll.y, roll.z
	);
}
END_TEST

/***
 * MOVEMENT & ROTATION TESTS
 */

START_TEST (turtleCompoundMoveTest)
{
	VECTOR3
		finalPos = vectorCreate (5.0, 5.0, 5.0),
		pos;
	turtleLookUp (t, 90);
	turtleMoveForward (t, 5.0);
	turtleLookDown (t, 90);
	turtleMoveForward (t, 5.0);
	turtleTurnRight (t, 90);
	turtleMoveForward (t, 5.0);
	pos = turtleGetPosition (t);
	fail_unless (
		vector_cmp (&finalPos, &pos),
		"After a move with multiple rotations, expected final position to be \33[32;1m%5.2f, %5.2f, %5.2f\33[0m, instead got \33[31;1m%5.2f, %5.2f, %5.2f\33[0m",
		finalPos.x, finalPos.y, finalPos.z,
		pos.x, pos.y, pos.z
	);
}
END_TEST

/***
 * STACK MANIPULATION TESTS
 */

START_TEST (turtleStackPushTest)
{
	VECTOR3
		finalPos = vectorCreate (30.0, 0.0, 0.0),
		finalHeading = vectorCreate (1.0, 0.0, 0.0),
		pos,
		heading;
	turtleMoveForward (t, 30.0);
	turtlePushStack (t);
	turtleTurnLeft (t, 90.0);
	turtleMoveForward (t, 30.0);
	turtlePopStack (t);
	pos = turtleGetPosition (t);
	heading = turtleGetHeadingVector (t);
	fail_unless
	(
		vector_cmp (&finalPos, &pos)
	);
	fail_unless
	(
		vector_cmp (&finalHeading, &heading)
	);
}
END_TEST

START_TEST (turtleInvalidStackPopTest)
{
	VECTOR3
		finalPos = vectorCreate (0.0, 0.0, -30.0),
		finalHeading = vectorCreate (0.0, 1.0, 0.0),
		pos,
		heading;
	turtleMoveLeft (t, 30.0);
	turtleLookUp (t, 90.0);
	turtlePopStack (t);
	pos = turtleGetPosition (t);
	heading = turtleGetHeadingVector (t);
	fail_unless
	(
		vector_cmp (&finalPos, &pos)
	);
	fail_unless
	(
		vector_cmp (&finalHeading, &heading)
	);
}
END_TEST

Suite * make_suite (void) {
	Suite * s = suite_create ("Turtle3D");
	TCase
		* tcCreate = tcase_create ("Creation"),
		* tcDefaults = tcase_create ("Default Values"),
		* tcMovement = tcase_create ("Basic Movement"),
		* tcRotation = tcase_create ("Basic Rotation"),
		* tcMoveRot = tcase_create ("Movement & Rotation"),
		* tcStack = tcase_create ("Stack Manipulation");

	tcase_add_test (tcCreate, turtleCreateTest);
	suite_add_tcase (s, tcCreate);

	tcase_add_checked_fixture (tcDefaults, turtleBasicSetup, turtleBasicTeardown);
	tcase_add_test (tcDefaults, turtleDefaultPositionTest);
	tcase_add_test (tcDefaults, turtleDefaultHeadingTest);
	tcase_add_test (tcDefaults, turtleDefaultPitchTest);
	tcase_add_test (tcDefaults, turtleDefaultRollTest);
	suite_add_tcase (s, tcDefaults);

	tcase_add_checked_fixture (tcMovement, turtleBasicSetup, turtleBasicTeardown);
	tcase_add_test (tcMovement, turtleMoveForwardTest);
	tcase_add_test (tcMovement, turtleMoveBackwardTest);
	tcase_add_test (tcMovement, turtleMoveUpTest);
	tcase_add_test (tcMovement, turtleMoveDownTest);
	tcase_add_test (tcMovement, turtleMoveRightTest);
	tcase_add_test (tcMovement, turtleMoveLeftTest);
	tcase_add_test (tcMovement, turtleMoveMultipleTest);
	suite_add_tcase (s, tcMovement);

	tcase_add_checked_fixture (tcRotation, turtleBasicSetup, turtleBasicTeardown);
	tcase_add_test (tcRotation, turtleTurnLeftTest);
	tcase_add_test (tcRotation, turtleTurnRightTest);
	tcase_add_test (tcRotation, turtleLookUpTest);
	tcase_add_test (tcRotation, turtleLookDownTest);
	tcase_add_test (tcRotation, turtleTwistClockwiseTest);
	tcase_add_test (tcRotation, turtleTwistCounterclockwiseTest);
	suite_add_tcase (s, tcRotation);

	tcase_add_checked_fixture (tcMoveRot, turtleBasicSetup, turtleBasicTeardown);
	tcase_add_test (tcMoveRot, turtleCompoundMoveTest);
	suite_add_tcase (s, tcMoveRot);

	tcase_add_checked_fixture (tcStack, turtleBasicSetup, turtleBasicTeardown);
	tcase_add_test (tcStack, turtleStackPushTest);
	tcase_add_test (tcStack, turtleInvalidStackPopTest);
	suite_add_tcase (s, tcStack);
	return s;
}

int main () {
  int number_failed = 0;
  SRunner * sr = srunner_create (make_suite ());

  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0)
    ? EXIT_SUCCESS
    : EXIT_FAILURE;
}
