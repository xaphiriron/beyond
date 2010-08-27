#ifndef XPH_COMPONENT_INTEGRATE_H
#define XPH_COMPONENT_INTEGRATE_H

#include "vector.h"
#include "object.h"
#include "entity.h"
#include "physics.h"

#include "component_position.h"

struct integrate_data {
  VECTOR3
    velocity,
    acceleration;
  float mass;

  VECTOR3
    tar_velocity,
    tar_acceleration,
    tar_pos;
};

void applyForce (struct integrate_data * idata, VECTOR3 * force, float delta);
void applyGravity (struct integrate_data * idata, float delta);
void commitIntegration (Entity * e);
void integrate (Entity * e, float delta);

int component_integrate (Object * obj, objMsg msg, void * a, void * b);

#endif /* XPH_COMPONENT_INTEGRATE_H */