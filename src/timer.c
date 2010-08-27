#include "timer.h"

static void timer_createTimerRegistry ();
static void timer_registerTimer (TIMER *);
static void timer_unregisterTimer (TIMER *);

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
  CLOCK * c = xph_alloc (sizeof (CLOCK), "CLOCK");
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

TIMER * timer_create (CLOCK * c, float scale) {
  TIMER * t = xph_alloc (sizeof (TIMER), "TIMER");
  t->scale = scale;
  t->elapsed = 0.0;
  t->clock = c;
  t->paused = TIMER_RUNNING;
  memset (&t->lastUpdate, '\0', sizeof (struct timeval));
  timer_registerTimer (t);
  return t;
}

void timer_destroy (TIMER * t) {
  timer_unregisterTimer (t);
  xph_free (t);
}

void timer_update (TIMER * t) {
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

float timer_timeElapsed (const TIMER * t) {
  return t->elapsed;
}

float timer_timeSinceLastUpdate (const TIMER * t) {
  struct timeval now;
  if (t->elapsed == 0.0) {
    return -1.0;
  }
  gettimeofday (&now, NULL);
  return timeval_cmp (&now, &t->lastUpdate);
}

static Vector * TimerRegistry = NULL;

void timer_updateAll () {
  TIMER * t = NULL;
  int i = 0;
  if (TimerRegistry == NULL)
    timer_createTimerRegistry ();
  while (vector_at (t, TimerRegistry, i++) != NULL) {
    timer_update (t);
  }
}

static void timer_registerTimer (TIMER * t) {
  if (TimerRegistry == NULL)
    timer_createTimerRegistry ();
  vector_push_back (TimerRegistry, t);
}

static void timer_unregisterTimer (TIMER * t) {
  if (TimerRegistry == NULL)
    timer_createTimerRegistry ();
  vector_remove (TimerRegistry, t);
}

static void timer_createTimerRegistry () {
  if (NULL != TimerRegistry) {
    return;
  }
  TimerRegistry = vector_create (8, sizeof (struct timer *));
}

void timer_destroyTimerRegistry () {
  TIMER * t = NULL;
  while (vector_size (TimerRegistry) > 0) {
    timer_destroy (vector_pop_back (t, TimerRegistry));
  }
  vector_destroy (TimerRegistry);
  TimerRegistry = NULL;
}
