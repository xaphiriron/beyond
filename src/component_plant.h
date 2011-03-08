#ifndef XPH_COMPONENT_PLANT_H
#define XPH_COMPONENT_PLANT_H

#include "object.h"
#include "entity.h"

#include "sph.h"
#include "vector.h"
#include "lsystem.h"
#include "dynarr.h"

#include "ground_draw.h"

typedef struct plantData * plantData;

bool plant_createRandom (Entity e);
bool plant_createHybrid (Dynarr parents);

void plant_grow (plantData plant);
void plant_die (plantData plant);

void plant_draw (Entity e, CameraGroundLabel label);

//void plant_pruneSegment (plantData plant, ??? segment);
//aaah collision detection with plant segments to determine pruning??? D:

int component_plant (Object * obj, objMsg msg, void * a, void * b);

#endif /* XPH_COMPONENT_PLANT_H */