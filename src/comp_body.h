#ifndef XPH_COMPONENT_BODY_H
#define XPH_COMPONENT_BODY_H

#include "entity.h"

struct xph_body
{
	float
		height;
};
typedef struct xph_body * Body;

void body_define (EntComponent comp, EntSpeech speech);

void bodyRender_system (Dynarr entities);


float body_height (Entity e);

#endif /* XPH_COMPONENT_BODY_H */