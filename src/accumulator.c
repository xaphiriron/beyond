#include "accumulator.h"

ACCUMULATOR * accumulator_create (TIMER t, float delta) {
  ACCUMULATOR * a = xph_alloc (sizeof (ACCUMULATOR));
  a->timer = t;
  a->timerElapsedLastUpdate = timerGetTotalTimeElapsed (t);
  a->delta = delta;
  a->maxDelta = delta * 50;
  a->accumulated = 0;
  a->active = true;
  return a;
}

void accumulator_destroy (ACCUMULATOR * a) {
  xph_free (a);
}

bool accumulator_withdrawlTime (ACCUMULATOR * a) {
  //printf ("%s: %f time accumulated with a delta of %f\n", __FUNCTION__, a->accumulated, a->delta);
  if (a->accumulated < a->delta)
    return false;
  a->accumulated -= a->delta;
  return true;
}

void accumulator_update (ACCUMULATOR * a) {
	float
		elapsed = timerGetTotalTimeElapsed (a->timer),
		passed = elapsed - a->timerElapsedLastUpdate;
  //printf ("%s: %f seconds passed since last update (%f total)\n", __FUNCTION__, passed, elapsed);
  if (a->active) {
    a->accumulated += passed > a->maxDelta ? a->maxDelta : passed;
  }
  a->timerElapsedLastUpdate = elapsed;
}
