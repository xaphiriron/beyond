#ifndef XPH_COMP_CHASER
#define XPH_COMP_CHASER

#include "entity.h"

#include "vector.h"

struct target_info
{
	Entity
		target;
	float
		weight;		// [-1.0 .. 1.0]; negative means it repels
};

struct xph_chaser
{
	Dynarr
		targets;
};
typedef struct xph_chaser * Chaser;

void chaser_target (Entity chaser, Entity target, float weight);

void chaser_define (EntComponent comp, EntSpeech speech);

void chaser_update (Dynarr entities);

#endif /* XPH_COMP_CHASER */