#include "accumulator.h"

ACCUMULATOR * accumulator_create (TIMER * t, float delta) {
  ACCUMULATOR * a = xph_alloc (sizeof (ACCUMULATOR));
  a->timer = t;
  a->timerElapsedLastUpdate = xtimer_timeElapsed (t);
  a->delta = delta;
  a->maxDelta = delta * 50;
  a->accumulated = 0;
  a->active = TRUE;
  return a;
}

void accumulator_destroy (ACCUMULATOR * a) {
  xph_free (a);
}

bool accumulator_withdrawlTime (ACCUMULATOR * a) {
  //printf ("%s: %f time accumulated with a delta of %f\n", __FUNCTION__, a->accumulated, a->delta);
  if (a->accumulated < a->delta)
    return FALSE;
  a->accumulated -= a->delta;
  return TRUE;
}

void accumulator_update (ACCUMULATOR * a) {
  float passed = xtimer_timeElapsed (a->timer) - a->timerElapsedLastUpdate;
  //printf ("%s: %f seconds passed since last update\n", __FUNCTION__, passed);
  if (a->active) {
    a->accumulated += passed > a->maxDelta ? a->maxDelta : passed;
  }
  a->timerElapsedLastUpdate = xtimer_timeElapsed (a->timer);
}
