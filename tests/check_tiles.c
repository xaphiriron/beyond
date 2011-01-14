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

Suite * make_tile_suite (void) {
  Suite * s = suite_create ("Tile");
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

int main (void) {
  int number_failed = 0;
  SRunner * sr = srunner_create (make_tile_suite ());

  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0)
    ? EXIT_SUCCESS
    : EXIT_FAILURE;
}