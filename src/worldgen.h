#ifndef XPH_WORLDGEN_H
#define XPH_WORLDGEN_H

#include <stdbool.h>
#include "xph_timer.h"
#include "map.h"

void worldConfig (Entity options);

void worldInit ();
void worldFinalize ();

void worldGenerate (TIMER * t);

void worldImprint (SUBHEX at);

#endif /* XPH_WORLDGEN_H */