/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#ifndef XPH_COMP_ARCH_H
#define XPH_COMP_ARCH_H

#include "entity.h"
#include "dynarr.h"

#include "map.h"

void arch_define (EntComponent comp, EntSpeech speech);
void pattern_define (EntComponent comp, EntSpeech speech);

// a thing with a builder component can directly affect arches, by building, supplying, linking, or destroying them
void builder_define (EntComponent comp, EntSpeech speech);

// instantiated on the player and npcs; handles messages that affect arches
void builder_system (Dynarr entities);

/*
// loads and unloads platters
void worldLoading_system (Dynarr entities);
// ??????
void worldSimulate_system (Dynarr entities);
*/
void arch_imprint (Entity arch, SUBHEX at);

typedef struct xph_pattern * Pattern;

void patternLoadDefinitions (char * path);

Pattern patternGet (unsigned int id);
Pattern patternGetByName (const char * name);

#endif /* XPH_COMP_ARCH_H */