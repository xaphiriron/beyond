#include <check.h>
#include <stdlib.h>
#include "../src/hex.h"

START_TEST (test_tile_hex_coords) {
  unsigned int
    r = 0, k = 0, i = 0,
    sr = 0, sk = 0, si = 0;
  signed int
    x = 0, y = 0;
  hex_rki2xy (r, k, i, &x, &y);
  fail_unless (x == 0 && y == 0);
  mark_point ();
  hex_xy2rki (x, y, &r, &k, &i);
  fail_unless (r == 0, k == 0, i == 0);
  mark_point ();
  while (r < 5) {
    k = 0;
    while (k < 6) {
      i = 0;
      while (i < r) {
        hex_rki2xy (r, k, i, &x, &y);
        hex_xy2rki (x, y, &sr, &sk, &si);
        fail_unless (
          r == sr && k == sk && i == si,
          "inversion failed on {%d %d %d}/%d,%d/{%d %d %d}",
          r, k, i, x, y, sr, sk, si
        );
        // printf (
        //   "{%d %d %d}/%d,%d/{%d %d %d}\n",
        //   r, k, i, x, y, sr, sk, si);
        i++;
      }
      k++;
    }
    r++;
  }
}
END_TEST

/*
START_TEST (test_tile_hex_inside) {
  VECTOR3 v = vectorCreate (0, 0, 0);
  fail_unless (
    hex_point_inside (&v) == TRUE,
    "point (%.2f, %.2f) should be considered inside the hex",
    v.x, v.z
  );
  v = vectorCreate (14.99, 0, 25.99);
  fail_unless (
    hex_point_inside (&v) == TRUE,
    "point (%.2f, %.2f) should be considered inside the hex",
    v.x, v.z
  );
  v = vectorCreate (-14.99, 0, 25.99);
  fail_unless (
    hex_point_inside (&v) == TRUE,
    "point (%.2f, %.2f) should be considered inside the hex",
    v.x, v.z
  );
  v = vectorCreate (14.99, 0, -25.99);
  fail_unless (
    hex_point_inside (&v) == TRUE,
    "point (%.2f, %.2f) should be considered inside the hex",
    v.x, v.z
  );
  v = vectorCreate (-14.99, 0, -25.99);
  fail_unless (
    hex_point_inside (&v) == TRUE,
    "point (%.2f, %.2f) should be considered inside the hex",
    v.x, v.z
  );
  v = vectorCreate (-29.99, 0, 0);
  fail_unless (
    hex_point_inside (&v) == TRUE,
    "point (%.2f, %.2f) should be considered inside the hex",
    v.x, v.z
  );
  v = vectorCreate (29.99, 0, 0);
  fail_unless (
    hex_point_inside (&v) == TRUE,
    "point (%.2f, %.2f) should be considered inside the hex",
    v.x, v.z
  );
}
END_TEST

START_TEST (test_tile_hex_outside) {
  VECTOR3 v = vectorCreate (0, 0, -30.0);
  fail_unless (
    hex_point_inside (&v) == FALSE,
    "point (%.2f, %.2f) should be considered outside the hex",
    v.x, v.z
  );
  v = vectorCreate (0, 0, 30.0);
  fail_unless (
    hex_point_inside (&v) == FALSE,
    "point (%.2f, %.2f) should be considered outside the hex",
    v.x, v.z
  );
  v = vectorCreate (30.01, 0, 0);
  fail_unless (
    hex_point_inside (&v) == FALSE,
    "point (%.2f, %.2f) should be considered outside the hex",
    v.x, v.z);
  v = vectorCreate (15.01, 0, 26.01);
  fail_unless (
    hex_point_inside (&v) == FALSE,
    "point (%.2f, %.2f) should be considered outside the hex",
    v.x, v.z
  );
  v = vectorCreate (-15.01, 0, 26.01);
  fail_unless (
    hex_point_inside (&v) == FALSE,
    "point (%.2f, %.2f) should be considered outside the hex",
    v.x, v.z
  );
  v = vectorCreate (-15.01, 0, -26.01);
  fail_unless (
    hex_point_inside (&v) == FALSE,
    "point (%.2f, %.2f) should be considered outside the hex",
    v.x, v.z
  );
  v = vectorCreate (15.01, 0, -26.01);
  fail_unless (
    hex_point_inside (&v) == FALSE,
    "point (%.2f, %.2f) should be considered outside the hex",
    v.x, v.z
  );
  v = vectorCreate (-30.01, 0, 0);
  fail_unless (
    hex_point_inside (&v) == FALSE,
    "point (%.2f, %.2f) should be considered outside the hex",
    v.x, v.z
  );
}
END_TEST
*/

HEX hex = NULL;
void hexSlopeSetup (void)
{
	hex = hex_create (0, 0, 0);
	hexSetHeight (hex, 32);
	hexSetCorners (hex, 0, 0, 0, 0, 0, 0);
	hexSetCorners (hex, -1, -1, 0, 1, 1, 0);
}

void hexSlopeTeardown (void)
{
	hex_destroy (hex);
}

/*
START_TEST (hexSlopeInitTest)
{
	signed char
		height;
	int
		i = 0;
	while (i < 6)
	{
		height = hexGetCornerHeight (hex, i);
		fail_unless (height == 0);
		i++;
	}
}
END_TEST
*/

START_TEST (hexSlopeChangeTest0)
{
	signed char
		height;
	height = hexGetCornerHeight (hex, 0);
	fail_unless (
		height == -1,
		"Expected height of %d-th corner to be %d, got %d instead",
		0, -1, height
	);
}
END_TEST

START_TEST (hexSlopeChangeTest1)
{
	signed char
		height;
	height = hexGetCornerHeight (hex, 1);
	fail_unless (
		height == -1,
		"Expected height of %d-th corner to be %d, got %d instead",
		1, -1, height
	);
}
END_TEST

START_TEST (hexSlopeChangeTest2)
{
	signed char
		height;
	height = hexGetCornerHeight (hex, 2);
	fail_unless (
		height == 0,
		"Expected height of %d-th corner to be %d, got %d instead",
		2, 0, height
	);
}
END_TEST

START_TEST (hexSlopeChangeTest3)
{
	signed char
		height;
	height = hexGetCornerHeight (hex, 3);
	fail_unless (
		height == 1,
		"Expected height of %d-th corner to be %d, got %d instead",
		3, 1, height
	);
}
END_TEST

START_TEST (hexSlopeChangeTest4)
{
	signed char
		height;
	height = hexGetCornerHeight (hex, 4);
	fail_unless (
		height == 1,
		"Expected height of %d-th corner to be %d, got %d instead",
		4, 1, height
	);
}
END_TEST

START_TEST (hexSlopeChangeTest5)
{
	signed char
		height;
	height = hexGetCornerHeight (hex, 5);
	fail_unless (
		height == 0,
		"Expected height of %d-th corner to be %d, got %d instead",
		5, 0, height
	);
}
END_TEST

/*
START_TEST (hexSlopeMinLimitTest)
{
	signed char
		height;
	hexSetHeight (hex, 0);
	hexSetCorners (hex, -1, -2, -3, -4, -5, -6);
}
END_TEST
*/

Suite * make_tile_suite (void)
{
	Suite
		* s = suite_create ("Tile");
	TCase
		* tc_coord = tcase_create ("Coordinates");

  tcase_add_test (tc_coord, test_tile_hex_coords);
/*
  tcase_add_test (tc_coord, test_tile_hex_inside);
  tcase_add_test (tc_coord, test_tile_hex_outside);
 */
  suite_add_tcase (s, tc_coord);
  return s;
}

Suite * makeHexSlopeSuite (void)
{
	Suite
		* s = suite_create ("Hex slopes");
	TCase
		* tcSetSlope = tcase_create ("Setting slope");

	tcase_add_checked_fixture (tcSetSlope, hexSlopeSetup, hexSlopeTeardown);
	tcase_add_test (tcSetSlope, hexSlopeChangeTest0);
	tcase_add_test (tcSetSlope, hexSlopeChangeTest1);
	tcase_add_test (tcSetSlope, hexSlopeChangeTest2);
	tcase_add_test (tcSetSlope, hexSlopeChangeTest3);
	tcase_add_test (tcSetSlope, hexSlopeChangeTest4);
	tcase_add_test (tcSetSlope, hexSlopeChangeTest5);

	suite_add_tcase (s, tcSetSlope);
	return s;
}

int main (void)
{
	int
		number_failed = 0;
	SRunner
		* sr = srunner_create (make_tile_suite ());

	srunner_add_suite (sr, makeHexSlopeSuite ());

	srunner_run_all (sr, CK_NORMAL);
	number_failed = srunner_ntests_failed (sr);
	srunner_free (sr);
	return (number_failed == 0)
		? EXIT_SUCCESS
		: EXIT_FAILURE;
}