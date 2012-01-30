#include "xph_timer.h"

#include "dynarr.h"
#include <SDL/SDL.h>

static void timerRegister (TIMER * t);
static void timerUnregister (TIMER * t);

static Dynarr
	timerRegistry = NULL;
void timerUpdateRegistry ()
{
	if (!timerRegistry)
		timerRegistry = dynarr_create (2, sizeof (TIMER *));
	dynarr_map (timerRegistry, (void (*)(void *))timerUpdate);
}

void timerDestroyRegistry ()
{
	dynarr_map (timerRegistry, (void (*)(void *))timerDestroy);
	dynarr_destroy (timerRegistry);
	timerRegistry = NULL;
}

static void timerRegister (TIMER * t)
{
	if (!timerRegistry)
		timerRegistry = dynarr_create (2, sizeof (TIMER *));
	dynarr_push (timerRegistry, t);
}

static void timerUnregister (TIMER * t)
{
	dynarr_remove_condense (timerRegistry, t);
}

TIMER * timerCreate ()
{
	TIMER
		* t = xph_alloc (sizeof (TIMER));
	t->scale = 1.0;
	t->paused = false;
	timerRegister (t);
	return t;
}

void timerDestroy (TIMER * t)
{
	timerUnregister (t);
	xph_free (t);
}

void timerUpdate (TIMER * t)
{
	unsigned long
		now;
	if (t->paused)
		return;
	now = SDL_GetTicks ();
	t->lastUpdateSpan = (now - t->lastUpdate) * t->scale;
	t->lastUpdate = now;
	t->elapsed += t->lastUpdateSpan;
	if (t->goal)
		t->goalElapsed += t->lastUpdateSpan;
}

void timerPause (TIMER * t)
{
	t->paused = true;
}

void timerStart (TIMER * t)
{
	t->paused = false;
	t->lastUpdateSpan = 0;
	t->lastUpdate = SDL_GetTicks ();
}

void timerSetGoal (TIMER * t, unsigned long milliseconds)
{
	t->goalElapsed = 0;
	t->goal = milliseconds;
}

void timerSetScale (TIMER * t, float scale)
{
	t->scale = scale;
}


bool outOfTime (TIMER * t)
{
	if (!t)
		return false;
	timerUpdate (t);
	return t->goal != 0 && t->goalElapsed >= t->goal;
}

float timerPercentageToGoal (const TIMER * t)
{
	if (t->goal == 0.0)
		return 1.0;
	return (float)t->goalElapsed / t->goal;
}


ACCUMULATOR * accumulator_create (TIMER * t, unsigned long delta)
{
  ACCUMULATOR * a = xph_alloc (sizeof (ACCUMULATOR));
  a->timer = t;
  a->delta = delta;
  a->maxDelta = delta * 5;
  a->accumulated = 0;
  a->active = true;
  return a;
}

void accumulator_destroy (ACCUMULATOR * a)
{
	xph_free (a);
}

bool accumulator_withdrawlTime (ACCUMULATOR * a)
{
	if (a->accumulated < a->delta)
		return false;
	a->accumulated -= a->delta;
	return true;
}

void accumulator_update (ACCUMULATOR * a)
{
	if (!a->active)
		return;
	a->accumulated += a->timer->lastUpdateSpan;
	if (a->accumulated > a->maxDelta)
		a->accumulated = a->maxDelta;
}
