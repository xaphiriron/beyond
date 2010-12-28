#ifndef XPH_COMPONENT_PLANT_H
#define XPH_COMPONENT_PLANT_H

#include "object.h"
#include "entity.h"

#include "vector.h"
#include "shapes.h"
#include "dynarr.h"

#include "ground_draw.h"

struct plant_data * plantComponent;

void plant_generateSeed (struct plant_data * pd);

void plant_generateRandom (Entity e);


void plant_draw (Entity e, CameraGroundLabel label);

int component_plant (Object * obj, objMsg msg, void * a, void * b);

#endif /* XPH_COMPONENT_PLANT_H */