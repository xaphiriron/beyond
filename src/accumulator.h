#ifndef ACCUMULATOR_H
#define ACCUMULATOR_H

#include "bool.h"
#include "xph_memory.h"
#include "timer.h"

typedef struct accumulator {
  TIMER * timer;
  bool active;
  float
    delta,
    maxDelta,
    accumulated,
    timerElapsedLastUpdate;
} ACCUMULATOR;

ACCUMULATOR * accumulator_create (TIMER *, float delta);
void accumulator_destroy (ACCUMULATOR *);

bool accumulator_withdrawlTime (ACCUMULATOR *);
void accumulator_update (ACCUMULATOR *);

#endif /* ACCUMULATOR_H */