#include "timer.h"

static void xtimer_createTimerRegistry ();
static void xtimer_registerTimer (TIMER *);
static void xtimer_unregisterTimer (TIMER *);

struct timeval timeval_subtract (const struct timeval * a, const struct timeval * b) {
  struct timeval diff;
  diff.tv_sec = a->tv_sec - b->tv_sec;
  diff.tv_usec = a->tv_usec - b->tv_usec;
  if (diff.tv_usec < 0) {
    diff.tv_usec += 1000000;
    diff.tv_sec -= 1;
  }
  return diff;
}

struct timeval timeval_add (const struct timeval * a, const struct timeval * b) {
  struct timeval diff;
  diff.tv_sec = a->tv_sec + b->tv_sec;
  diff.tv_usec = a->tv_usec + b->tv_usec;
  if (diff.tv_usec > 1000000) {
    diff.tv_usec -= 1000000;
    diff.tv_sec += 1;
  }
  return diff;
}

float timeval_cmp (const struct timeval * a, const struct timeval * b) {
  struct timeval diff = timeval_subtract (a, b);
  return timeval_asFloat (&diff);
}

float timeval_asFloat (const struct timeval * t) {
  return t->tv_sec + (t->tv_usec / 1000000.0);
}




CLOCK * clock_create () {
  CLOCK * c = xph_alloc (sizeof (CLOCK));
  memset (c, '\0', sizeof (CLOCK));
  return c;
}

void clock_destroy (CLOCK * c) {
  xph_free (c);
}

void clock_update (CLOCK * c) {
  c->previous = c->now;
  gettimeofday (&c->now, NULL);
}

TIMER * xtimer_create (CLOCK * c, float scale) {
  TIMER * t = xph_alloc (sizeof (TIMER));
  t->scale = scale;
  t->elapsed = 0.0;
  t->clock = c;
  t->paused = TIMER_RUNNING;
  memset (&t->lastUpdate, '\0', sizeof (struct timeval));
  xtimer_registerTimer (t);
  return t;
}

void xtimer_destroy (TIMER * t) {
  xtimer_unregisterTimer (t);
  xph_free (t);
}

void xtimer_update (TIMER * t) {
  float u = 0.0;
  if (TIMER_RUNNING != t->paused) {
    return;
  }
  if (t->lastUpdate.tv_sec == 0) {
    // If we're on the first update of the timer, skip calculating the difference, since it'll be huge and break everything.
    t->lastUpdate = t->clock->now;
    return;
  }
  u = timeval_cmp (&t->clock->now, &t->lastUpdate);
  t->elapsed += u * t->scale;
  t->lastUpdate = t->clock->now;
  //printf ("%s: %f time elapsed total, %f between updates, %f inc. scaling\n", __FUNCTION__, t->elapsed, u, u * t->scale);
}

float xtimer_timeElapsed (const TIMER * t) {
  return t->elapsed;
}

float xtimer_timeSinceLastUpdate (const TIMER * t) {
  struct timeval now;
  if (t->elapsed == 0.0) {
    return -1.0;
  }
  gettimeofday (&now, NULL);
  return timeval_cmp (&now, &t->lastUpdate);
}

static Dynarr TimerRegistry = NULL;

void xtimer_updateAll () {
  TIMER * t = NULL;
  int i = 0;
  if (TimerRegistry == NULL)
    xtimer_createTimerRegistry ();
  while ((t = *(TIMER **)dynarr_at (TimerRegistry, i++)) != NULL) {
    xtimer_update (t);
  }
}

static void xtimer_registerTimer (TIMER * t) {
  if (TimerRegistry == NULL)
    xtimer_createTimerRegistry ();
  dynarr_push (TimerRegistry, t);
}

static void xtimer_unregisterTimer (TIMER * t) {
  if (TimerRegistry == NULL)
    xtimer_createTimerRegistry ();
  dynarr_remove_condense (TimerRegistry, t);
}

static void xtimer_createTimerRegistry () {
  if (NULL != TimerRegistry) {
    return;
  }
  TimerRegistry = dynarr_create (8, sizeof (struct timer *));
}

void xtimer_destroyTimerRegistry () {
  while (!dynarr_isEmpty (TimerRegistry)) {
    xtimer_destroy (*(TIMER **)dynarr_pop (TimerRegistry));
  }
  dynarr_destroy (TimerRegistry);
  TimerRegistry = NULL;
}
