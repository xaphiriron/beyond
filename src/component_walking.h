#ifndef XPH_COMPONENT_WALKING_H
#define XPH_COMPONENT_WALKING_H

#include <math.h>

#include "vector.h"

#include "entity.h"

#include "component_position.h"
#include "component_input.h"

#include "system.h"

void walking_define (EntComponent comp, EntSpeech speech);

void walking_system (Dynarr entities);

#endif