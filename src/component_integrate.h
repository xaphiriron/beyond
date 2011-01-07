#ifndef XPH_COMPONENT_INTEGRATE_H
#define XPH_COMPONENT_INTEGRATE_H

#include "vector.h"
#include "object.h"
#include "entity.h"

#include "component_position.h"

#include "system.h"

struct integrate_data {
  VECTOR3
    velocity,
    acceleration;
  float mass;

  VECTOR3
    new_movement,
    tar_velocity,
    tar_acceleration,
    tar_pos,
    extra_velocity;	// velocity not from applied forces, e.g., movement when walking
};

void addExtraVelocity (struct integrate_data * idata, VECTOR3 * vel);
void applyForce (struct integrate_data * idata, VECTOR3 * force, float delta);
void applyGravity (struct integrate_data * idata, float delta);
void commitIntegration (Entity e, float delta);
void integrate (Entity e, float delta);

int component_integrate (Object * obj, objMsg msg, void * a, void * b);

#endif /* XPH_COMPONENT_INTEGRATE_H */