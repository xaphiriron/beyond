#ifndef XPH_COMPONENT_PLANT_H
#define XPH_COMPONENT_PLANT_H

#include "object.h"
#include "entity.h"

#include "sph.h"
#include "vector.h"
#include "lsystem.h"
#include "turtle3d.h"
#include "dynarr.h"

typedef struct plantData * PLANT;

bool plant_createRandom (Entity e);
bool plant_createHybrid (Dynarr parents);

void plant_update (EntComponent plant);
void plant_grow (PLANT plant);
void plant_die (PLANT plant);

void plant_draw (Entity e);

//void plant_pruneSegment (plantData plant, ??? segment);
//aaah collision detection with plant segments to determine pruning??? D:

int component_plant (Object * obj, objMsg msg, void * a, void * b);

#endif /* XPH_COMPONENT_PLANT_H */