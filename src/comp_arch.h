#ifndef XPH_COMP_WORLDARCH_H
#define XPH_COMP_WORLDARCH_H

#include "entity.h"
#include "dynarr.h"

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

#endif /* XPH_COMP_WORLDARCH_H */