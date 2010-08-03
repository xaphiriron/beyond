#include <stdlib.h>
#include <check.h>
#include "../src/fcmp.h"

START_TEST (test_float_cmp)
{
  float
    x = 1000.25,
    y = -2383574.0,
    z = 756218.145263;
  fail_unless (fcmp (x, x) == TRUE, "fcmp (x, x) must be true unless x != x");
  fail_unless (fcmp (y, y) == TRUE, NULL);
  fail_unless (fcmp (z, z) == TRUE, NULL);
  fail_unless (fcmp (x, y) == FALSE);
  fail_unless (fcmp (x, z) == FALSE);
  fail_unless (fcmp (y, z) == FALSE);
  fail_unless (fcmp (x, -x) == FALSE);
  fail_unless (fcmp (y, -y) == FALSE);
  fail_unless (fcmp (z, -z) == FALSE);
  fail_unless (fcmp (x, 1000.49) == FALSE);
}
END_TEST

Suite * make_float_suite (void) {
  Suite * s = suite_create ("Float");
  TCase * tc_core = tcase_create ("Core");
  tcase_add_test (tc_core, test_float_cmp);
  suite_add_tcase (s, tc_core);
  return s;
}
