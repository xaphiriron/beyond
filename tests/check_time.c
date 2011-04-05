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
  TIMER t = timerCreate ();
	timerSetClock (t, clock_create ());
  fail_unless (
    t != NULL
  );
  fail_unless (
    timerGetTimeSinceLastUpdate (t) < 0,
    "A timer which has never been updated should return a negative value for time since last update."
  );
  fail_unless (
    timerGetTotalTimeElapsed (t) == 0.0,
    "A timer which has never been updated should say no time has elapsed since starting"
  );
  timerDestroy (t);
}
END_TEST

START_TEST (test_timer_update) {
  TIMER t = timerCreate ();
  float elapsed = 0.0;
	timerSetClock (t, clock_create ());
  timerUpdate (t);
  elapsed = timerGetTotalTimeElapsed (t);
  fail_unless (
    elapsed >= 0,
    "timerUpdate should update the specified timer (%f)",
    elapsed
  );
  xtimerUpdateAll ();
  fail_unless (
    timerGetTotalTimeElapsed (t) >= elapsed,
    "xtimerUpdateAll should update every created timer"
  );
  timerDestroy (t);
}
END_TEST

START_TEST (test_timer_scale) {
  CLOCK
    * c = clock_create ();
  TIMER
    t = timerCreate (),
    u = timerCreate ();
	timerSetClock (t, c);
	timerSetClock (u, c);
	timerSetScale (u, 2.0);
  xtimerUpdateAll ();
  fail_unless (
    fcmp (timerGetTotalTimeElapsed (t) * 2.0, timerGetTotalTimeElapsed (u)) == TRUE,
    "The scale of a timer should work as a multiplier for the time elapsed"
  );
  timerDestroy (t);
  timerDestroy (u);
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
