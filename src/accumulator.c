#include "accumulator.h"

ACCUMULATOR * accumulator_create (TIMER * t, float delta) {
  ACCUMULATOR * a = xph_alloc (sizeof (ACCUMULATOR), "ACCUMULATOR");
  a->timer = t;
  a->timerElapsedLastUpdate = timer_timeElapsed (t);
  a->delta = delta;
  a->maxDelta = delta * 25;
  a->accumulated = 0;
  return a;
}

void accumulator_destroy (ACCUMULATOR * a) {
  xph_free (a);
}

bool accumulator_withdrawlTime (ACCUMULATOR * a) {
  if (a->accumulated < a->delta)
    return FALSE;
  a->accumulated -= a->delta;
  return TRUE;
}

void accumulator_update (ACCUMULATOR * a) {
  float passed = timer_timeElapsed (a->timer) - a->timerElapsedLastUpdate;
/*
  logger (E_DEBUG, "%s: %f seconds passed since last update", __FUNCTION__, passed);
*/
  if (a->active) {
    a->accumulated += passed > a->maxDelta ? a->maxDelta : passed;
  }
  a->timerElapsedLastUpdate = timer_timeElapsed (a->timer);
}
