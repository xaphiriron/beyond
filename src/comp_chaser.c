/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#include "comp_chaser.h"

#include "component_position.h"
#include "component_walking.h"

void chaser_target (Entity chaser, Entity target, float weight)
{
	struct target_info
		* info;
	Chaser
		chaseData = component_getData (entity_getAs (chaser, "chaser"));
	if (!chaseData)
		return;
	info = xph_alloc (sizeof (struct target_info));
	info->target = target;
	info->weight = weight;
	dynarr_push (chaseData->targets, info);
}

static void chaser_create (EntComponent comp, EntSpeech speech);
static void chaser_destroy (EntComponent comp, EntSpeech speech);

void chaser_define (EntComponent comp, EntSpeech speech)
{
	component_registerResponse ("chaser", "__create", chaser_create);
	component_registerResponse ("chaser", "__destroy", chaser_destroy);
}

static void chaser_create (EntComponent comp, EntSpeech speech)
{
	Chaser
		chaser = xph_alloc (sizeof (struct xph_chaser));
	chaser->targets = dynarr_create (2, sizeof (struct chase_target_info *));

	component_setData (comp, chaser);
}

static void chaser_destroy (EntComponent comp, EntSpeech speech)
{
	Chaser
		chaser = component_getData (comp);
	dynarr_wipe (chaser->targets, xph_free);
	dynarr_destroy (chaser->targets);
	xph_free (chaser);

	component_clearData (comp);
}

void chaser_update (Dynarr entities)
{
	Entity
		active;
	int
		i = 0,
		j = 0;
	Chaser
		chaser;
	struct target_info
		* target;
	VECTOR3
		direction,
		distance;
	while ((active = *(Entity *)dynarr_at (entities, i++)))
	{
		chaser = component_getData (entity_getAs (active, "chaser"));
		direction = vectorCreate (0, 0, 0);
		j = 0;
		while ((target = *(struct target_info **)dynarr_at (chaser->targets, j++)))
		{
			distance = position_distanceBetween (active, target->target);
			distance = vectorNormalize (&distance);
			distance = vectorMultiplyByScalar (&distance, target->weight);
			direction = vectorAdd (&direction, &distance);
		}
		direction = vectorNormalize (&direction);
		// direction is NaN; can't use
		if (direction.x != direction.x || direction.z != direction.z)
			continue;
		position_face (active, &direction);
		walking_start (active);
	}
}
