#ifndef XPH_TIMER_H
#define XPH_TIMER_H

#include <stdbool.h>
#include "xph_memory.h"

typedef struct xph_timer
{
	unsigned long
		lastUpdate,
		lastUpdateSpan,
		elapsed,
		goal,
		goalElapsed;
	float
		scale;
	bool
		paused;
} TIMER;

typedef struct xph_accumulator
{
	TIMER
		* timer;
	bool
		active;
	unsigned long
		delta,
		maxDelta,
		accumulated;
} ACCUMULATOR;

void timerUpdateRegistry ();
void timerDestroyRegistry ();

TIMER * timerCreate ();
void timerDestroy (TIMER * t);
void timerUpdate (TIMER * t);

void timerPause (TIMER * t);
void timerStart (TIMER * t);

void timerSetGoal (TIMER * t, unsigned long milliseconds);
void timerSetScale (TIMER * t, float scale);

bool outOfTime (TIMER * t);
float timerPercentageToGoal (const TIMER * t);

ACCUMULATOR * accumulator_create (TIMER * t, unsigned long delta);
void accumulator_destroy (ACCUMULATOR *);

bool accumulator_withdrawlTime (ACCUMULATOR *);
void accumulator_update (ACCUMULATOR *);

#endif /* XPH_TIMER_H */