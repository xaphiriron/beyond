#ifndef TIMER_H
#define TIMER_H

#include <stdbool.h>
#include <stdio.h>
#include <sys/time.h>
#include "xph_memory.h"
#include "dynarr.h"

typedef struct clock {
  struct timeval
    now,
    previous;
} CLOCK;

typedef struct timer * TIMER;

struct timeval timeval_subtract (const struct timeval * a, const struct timeval * b);
struct timeval timeval_add (const struct timeval * a, const struct timeval * b);
float timeval_cmp (const struct timeval * a, const struct timeval * b);
float timeval_asFloat (const struct timeval * t);

CLOCK * clock_create ();
void clock_destroy (CLOCK *);

void clock_update (CLOCK *);



TIMER timerCreate ();
void timerDestroy (TIMER t);

void timerSetClock (TIMER t, CLOCK * c);
void timerSetScale (TIMER t, float scale);
void timerSetGoal (TIMER t, float goal);
void timerPause (TIMER t);
void timerStart (TIMER t);

bool outOfTime (TIMER t);

void timerUpdate (TIMER t);
float lastTimestep (const TIMER t);
float timerGetTotalTimeElapsed (const TIMER t);
float timerGetTimeSinceLastUpdate (const TIMER t);

float timerPercentageToGoal (const TIMER t);

void xtimerUpdateAll ();
void xtimerDestroyRegistry ();




TIMER * xtimer_create (CLOCK * c, float scale);
void xtimer_destroy (TIMER *);
void xtimer_update (TIMER *);

float xtimer_timeElapsed (const TIMER * t);
float xtimer_timeSinceLastUpdate (const TIMER * t);

void xtimer_updateAll ();

void xtimer_destroyTimerRegistry ();

#endif /* TIMER_H */
