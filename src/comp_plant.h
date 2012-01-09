#ifndef XPH_COMP_PLANT_H
#define XPH_COMP_PLANT_H

#include "entity.h"

#include "lsystem.h"
#include "turtle3d.h"
#include "shapes.h"

struct xph_plant
{
	LSYSTEM
		* expansion;
	SymbolSet
		render;
	char
		* body;
	short
		lastUpdated,
		updateFrequency;

	SHAPE
		fauxShape;
};
typedef struct xph_plant * Plant;

void plant_define (EntComponent comp, EntSpeech speech);

void plantUpdate_system (Dynarr entities);
void plantRender_system (Dynarr entities);

#endif /* XPH_COMP_PLANT_H */