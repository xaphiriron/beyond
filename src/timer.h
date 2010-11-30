#ifndef TIMER_H
#define TIMER_H

#include <stdio.h>
#include <sys/time.h>
#include "bool.h"
#include "xph_memory.h"
#include "dynarr.h"

typedef struct clock {
  struct timeval
    now,
    previous;
} CLOCK;

typedef struct timer {
  enum {
    TIMER_PAUSED = TRUE,
    TIMER_RUNNING = FALSE
  } paused;
  float
    scale,
    elapsed;
  struct timeval lastUpdate;
  CLOCK * clock;
} TIMER;

struct timeval timeval_subtract (const struct timeval * a, const struct timeval * b);
struct timeval timeval_add (const struct timeval * a, const struct timeval * b);
float timeval_cmp (const struct timeval * a, const struct timeval * b);
float timeval_asFloat (const struct timeval * t);

CLOCK * clock_create ();
void clock_destroy (CLOCK *);

void clock_update (CLOCK *);

TIMER * timer_create (CLOCK * c, float scale);
void timer_destroy (TIMER *);
void timer_update (TIMER *);

float timer_timeElapsed (const TIMER * t);
float timer_timeSinceLastUpdate (const TIMER * t);

void timer_updateAll ();

void timer_destroyTimerRegistry ();

#endif /* TIMER_H */