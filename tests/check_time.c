#include <stdlib.h>
#include <check.h>
#include "../src/timer.h"
#include "../src/fcmp.h"

START_TEST (test_clock_create) {
  CLOCK * c = clock_create ();
  fail_unless (
    c != NULL
  );
}
END_TEST

START_TEST (test_timer_create) {
  TIMER * t = xtimer_create (clock_create (), 1.0);
  fail_unless (
    t != NULL
  );
  fail_unless (
    xtimer_timeSinceLastUpdate (t) < 0,
    "A timer which has never been updated should return a negative value for time since last update."
  );
  fail_unless (
    xtimer_timeElapsed (t) == 0.0,
    "A timer which has never been updated should say no time has elapsed since starting"
  );
  xtimer_destroy (t);
}
END_TEST

START_TEST (test_timer_update) {
  TIMER * t = xtimer_create (clock_create (), 1.0);
  float elapsed = 0.0;
  xtimer_update (t);
  elapsed = xtimer_timeElapsed (t);
  fail_unless (
    elapsed >= 0,
    "timer_update should update the specified timer (%f)",
    elapsed
  );
  xtimer_updateAll ();
  fail_unless (
    xtimer_timeElapsed (t) >= elapsed,
    "timer_updateAll should update every created timer"
  );
  xtimer_destroy (t);
}
END_TEST

START_TEST (test_timer_scale) {
  CLOCK
    * c = clock_create ();
  TIMER
    * t = xtimer_create (c, 1.0),
    * u = xtimer_create (c, 2.0);
  xtimer_updateAll ();
  fail_unless (
    fcmp (xtimer_timeElapsed (t) * 2.0, xtimer_timeElapsed (u)) == TRUE,
    "The scale of a timer should work as a multiplier for the time elapsed"
  );
  xtimer_destroy (t);
  xtimer_destroy (u);
}
END_TEST

Suite * make_timer_suite (void) {
  Suite * s = suite_create ("Timers");
  TCase
    * tc_init = tcase_create ("Creation"),
    * tc_update = tcase_create ("Updating");
  tcase_add_test (tc_init, test_clock_create);
  tcase_add_test (tc_init, test_timer_create);
  suite_add_tcase (s, tc_init);

  tcase_add_test (tc_update, test_timer_update);
  tcase_add_test (tc_update, test_timer_scale);
  suite_add_tcase (s, tc_update);

  return s;
}
