#ifndef XPH_COMPONENT_WALKING_H
#define XPH_COMPONENT_WALKING_H

#include <math.h>

#include "vector.h"

#include "entity.h"

#include "component_position.h"
#include "component_input.h"

#include "system.h"

enum walk_move {
  WALK_MOVE_NONE     = 0x00,
  WALK_MOVE_FORWARD  = 0x01,
  WALK_MOVE_BACKWARD = 0x02,
  WALK_MOVE_LEFT     = 0x04,
  WALK_MOVE_RIGHT    = 0x08
};

typedef struct walkmove_data * walkingComponent;
typedef struct walkmove_data * walkData;

void walking_begin_movement (Entity e, enum walk_move w);
void walking_end_movement (Entity e, enum walk_move w);

void walk_move (Entity e);

void walking_define (EntComponent comp, EntSpeech speech);

void walking_system (Dynarr entities);

#endif