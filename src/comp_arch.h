#ifndef XPH_COMP_WORLDARCH_H
#define XPH_COMP_WORLDARCH_H

#include "entity.h"
#include "dynarr.h"

void arch_define (EntComponent comp, EntSpeech speech);
void pattern_define (EntComponent comp, EntSpeech speech);


/*
void worldAffects_define (EntComponent comp, EntSpeech speech);

// loads and unloads platters
void worldLoading_system (Dynarr entities);
// instantiated on the player and npcs; handles messages that affect arches
void worldAffects_system (Dynarr entities);
// ??????
void worldSimulate_system (Dynarr entities);
*/

#endif /* XPH_COMP_WORLDARCH_H */