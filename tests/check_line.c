#include <stdlib.h>
#include <check.h>
#include "../src/fcmp.h"
#include "../src/line.h"

LINE
 * l = NULL,
 * m = NULL;

void base_setup (void) {
  l = line_create (2401.63, -725.23, 0.0003, -12.6);
}

void base_teardown (void) {
  line_destroy (l);
  l = NULL;
}

START_TEST (test_line_create)
{
  fail_unless (
    l != NULL,
    "Line not created.");
}
END_TEST

START_TEST (test_line_distance)
{
  float
    x = 0,
    y = 0;

  fail_unless (
    line_coordsAtT (l, 0, &x, &y)
      && fcmp (x, 2401.63)
      && fcmp (y, -725.23),
    "Line's x0 and y0 value not set properly on creation");
  fail_unless (
    line_coordsAtT (l, 1, &x, &y)
      && fcmp (x, 2401.63 + 0.0003)
      && fcmp (y, -725.23 + -12.6),
    "Line's f and g value not set properly on creation");
  fail_unless (
    line_coordsAtT (l, -1, &x, &y)
      && fcmp (x, 2401.63 - 0.0003)
      && fcmp (y, -725.23 - -12.6),
    "T distance calculations incorrect");
}
END_TEST

START_TEST (test_line_tval_add)
{
  struct tval * t = NULL;
  fail_unless (
    line_mostRecentTval (l) == NULL,
    "Prior to any tvals being set on a line, line_mostRecentTval() should return NULL.");
  line_addTval (l, 1, LINE_ENDPOINT, NULL);
  t = line_addTval (l, 0, LINE_ENDPOINT, NULL);
  if (t == NULL || t->t != 0 || t->type != LINE_ENDPOINT || t->l != NULL) {
    fail ("T values not set properly on addition.");
  }
  fail_unless (
    line_countTvals (l) == 2,
    "Line t values not added");
  fail_unless (
    (t = line_mostRecentTval (l)) != NULL,
    "Most recently set T value ought to be returned by line_mostRecentTval()");
  t = line_nthRecentTval (l, 0);
  if (t == NULL || t->t != 0) {
    fail ("Recent T values not in proper order (most recent lowest)");
  }
  t = line_nthOrderedTval (l, 1);
  if (t == NULL || t->t != 1) {
    fail ("Ordered T values not in proper order (smallest lowest)");
  }
  fail_unless (
    line_nthOrderedTval (l, -1) == NULL,
    "Negative tval offsets should not be honored");
  fail_unless (
    line_nthOrderedTval (l, 2) == NULL,
    "Overrun tval offsets should not be honored");
}
END_TEST


void intersect_setup (void) {
  l = line_create (2401.63, -725.23, 0.0003, -12.6);
  m = line_create (-10, 0, 10, 0);
}

void intersect_teardown (void) {
  line_destroy (l);
  line_destroy (m);
  l = m = NULL;
}

START_TEST (test_line_intersection) {
  struct tval
    * t_l = NULL,
    * t_m = NULL;
  float
    t = line_findIntersection (m, l, LINE_SETINTERSECTION),
    x0 = 0,
    y0 = 0,
    x1 = 0,
    y1 = 0;
  fail_unless (
    line_countTvals (m) == 1,
    "Intersection t values not added to first line when requested");
  fail_unless (
    line_countTvals (l) == 1,
    "Intersection t values not added to second line when requested");
  t_l = line_mostRecentTval (l);
  t_m = line_mostRecentTval (m);
  fail_unless (
    fcmp (t_m->t, t),
    "T value returned from line_findIntersection must be the intersection point on the first line");
  fail_unless (
    t_l->l == m
      && t_m->l == l,
    "Intersection T values must reference to the line intersected with");
  line_coordsAtT (l, t_l->t, &x0, &y0);
  line_coordsAtT (m, t_m->t, &x1, &y1);
  fail_unless (
    fcmp (x0, x1) && fcmp (y0, y1),
    "Line intersection is not at the same location on both lines (l @ %f: %f, %f vs. m @ %f: %f, %f)", t_l->t, x0, y0, t_m->t, x1, y1);
}
END_TEST

START_TEST (test_line_parallel) {
  float nan = 0;
  line_destroy (m); // lol misusing test fixtures :(
  m = line_create (7234.0, 725.23, 0.0003, -12.6);
  nan = line_findIntersection (m, l, LINE_SETINTERSECTION);
  fail_unless (
    line_countTvals (m) == 0
      && line_countTvals (l) == 0,
    "Parallel lines do not intersect; they must not create intersection points on either line."
  );
  fail_unless (
    nan != nan,
    "Functions which return a t value for intersection must return NaN when lines do not intersect.");
}
END_TEST

START_TEST (test_line_throughpoint) {
  VECTOR3
   a = vectorCreate (100.0, 5.56, 0.0),
   b = vectorCreate (5012.0, -12.45, 0.0),
   m = vectorCreate (0.0, 0.0, 0.0);
  float
   x = 0, y = 0;
  LINE * q = line_createThroughPoints (&a, &b, LINE_DONOTSET);
  line_coordsAtT (q, 0, &x, &y);
  m = vectorCreate (x, y, 0.0);
  fail_unless (
    vector_cmp (&a, &m) == true,
    "When creating a line through two points, the first point must correspond to a T value of 0. (expecting %.3f,%.3f, got %.3f,%.3f)",
    a.x, a.y, x, y
  );
  line_coordsAtT (q, 1, &x, &y);
  m = vectorCreate (x, y, 0.0);
  fail_unless (
    vector_cmp (&b, &m) == true,
    "When creating a line through two points, the second point must correspond to a T value of 1. (expecting %.3f,%.3f, got %.3f,%.3f)",
    a.x, a.y, x, y
  );
}
END_TEST

LINE * j = NULL;
VECTOR3
  f, g,
  h, i;

void lengthen_setup (void) {
  j = line_create (100.0, 0.0, 100.0, 100.0);
  f = vectorCreate (0.0, 0.0, 0.0);
  g = vectorCreate (0.0, 0.0, 0.0);
  h = vectorCreate (0.0, 0.0, 0.0);
  i = vectorCreate (0.0, 0.0, 0.0);
}

void lengthen_teardown (void) {
  line_destroy (j);
  j = NULL;
}

START_TEST (test_line_lengthen) {
  line_coordsAtT (j, -1, &f.x, &f.y);
  line_coordsAtT (j, 2, &g.x, &g.y);
  fail_unless (
    line_resize (j, -1, 2) == true,
    "A successful resize should return true"
  );
  line_coordsAtT (j, 0, &h.x, &h.y);
  line_coordsAtT (j, 1, &i.x, &i.y);
  fail_unless (
    vector_cmp (&f, &h) == true,
    "A line lengthening must make the t-val given by the first argument into the new t-0 point."
  );
  fail_unless (
    vector_cmp (&g, &i) == true,
    "A line lengthening must make the t-val given by the second argument into the new t-1 point.");
}
END_TEST

START_TEST (test_line_flip) {
  line_coordsAtT (j, -1.1, &f.x, &f.y);
  line_coordsAtT (j, 4.3, &g.x, &g.y);
  line_resize (j, 4.3, -1.1);
  line_coordsAtT (j, 0, &h.x, &h.y);
  line_coordsAtT (j, 1, &i.x, &i.y);
  fail_unless (
    vector_cmp (&f, &i) == true &&
    vector_cmp (&g, &h) == true,
    "A new t-0 that is smaller than the t-1 is allowed, and should flip the line's direction as well as changing its length."
  );
}
END_TEST

START_TEST (test_line_shrink) {
  line_coordsAtT (j, 0, &f.x, &f.y);
  line_coordsAtT (j, 1, &g.x, &g.y);
  fail_unless (
    line_resize (j, 5, 5) == false,
    "The same t-value is not allowed as both the t-0 point and the t-1 point."
  );
  line_coordsAtT (j, 0, &h.x, &h.y);
  line_coordsAtT (j, 1, &i.x, &i.y);
  fail_unless (
    vector_cmp (&f, &h) == true &&
    vector_cmp (&g, &i) == true,
    "An invalid resize attempt cannot alter the line's dimensions"
  );
}
END_TEST

Suite * make_line_suite (void) {
  Suite * s = suite_create ("Line");
  TCase
    * tc_main = tcase_create ("Main"),
    * tc_intersect = tcase_create ("Intersection"),
    * tc_throughpoint = tcase_create ("Creation through two points"),
    * tc_lengthen = tcase_create ("Line resizing");
  tcase_add_checked_fixture (tc_main, base_setup, base_teardown);
  tcase_add_test (tc_main, test_line_create);
  tcase_add_test (tc_main, test_line_distance);
  tcase_add_test (tc_main, test_line_tval_add);
  suite_add_tcase (s, tc_main);

  tcase_add_checked_fixture (tc_intersect, intersect_setup, intersect_teardown);
  tcase_add_test (tc_intersect, test_line_intersection);
  tcase_add_test (tc_intersect, test_line_parallel);
  suite_add_tcase (s, tc_intersect);

  tcase_add_test (tc_throughpoint, test_line_throughpoint);
  suite_add_tcase (s, tc_throughpoint);

  tcase_add_checked_fixture (tc_lengthen, lengthen_setup, lengthen_teardown);
  tcase_add_test (tc_lengthen, test_line_lengthen);
  tcase_add_test (tc_lengthen, test_line_flip);
  tcase_add_test (tc_lengthen, test_line_shrink);
  suite_add_tcase (s, tc_lengthen);
  return s;
}
