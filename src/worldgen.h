#ifndef XPH_WORLDGEN_H
#define XPH_WORLDGEN_H

#include <stdbool.h>
#include "timer.h"
#include "map.h"

void worldgenAbsHocNihilo ();
void worldgenFinalizeCreation ();

void worldgenExpandWorldGraph (TIMER t);

void worldgenImprintMapData (SUBHEX at);
void worldgenImprintAllArches (SUBHEX at);

#endif /* XPH_WORLDGEN_H */