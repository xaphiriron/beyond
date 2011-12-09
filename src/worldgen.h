#ifndef XPH_WORLDGEN_H
#define XPH_WORLDGEN_H

#include <stdbool.h>
#include "timer.h"
#include "map.h"

void worldInit ();
void worldFinalize ();

void worldGenerate (TIMER t);

void worldImprint (SUBHEX at);

#endif /* XPH_WORLDGEN_H */