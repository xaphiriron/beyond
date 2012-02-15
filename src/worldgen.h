/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

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