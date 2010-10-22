#ifndef XPH_COMPONENT_POSITION_H
#define XPH_COMPONENT_POSITION_H

#include "xph_memory.h"
#include "vector.h"
#include "hex.h"
#include "world.h"
#include "object.h"
#include "entity.h"

#include "component_collide.h"

typedef struct axes {
  VECTOR3
    forward,
    side,
    up;
} AXES;

struct position_data {
  VECTOR3 pos;
  AXES
    orient;
  HEX * tileOccupying;
  Vector * tileFootprint;
};

void setPosition (Entity e, VECTOR3 pos);
int component_position (Object * obj, objMsg msg, void * a, void * b);

#endif /* XPH_COMPONENT_POSITION_H */