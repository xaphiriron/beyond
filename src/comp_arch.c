#include "comp_arch.h"

#include "map.h"
#include "component_position.h"

struct xph_arch
{
	Entity
		parent,
		pattern;
	bool
		expanded;
	Dynarr
		connectedArches,
		subarches;
};

typedef struct xph_arch * Arch;

struct xph_pattern
{
	Dynarr
		potentialSubpatterns;
	// this should also have a shape: something simple and statistical, which constrains the graph expansion of an arch using this pattern. something simple would be "a sphere", something more complex would be a density spread or a generic graph model.
};

typedef struct xph_pattern * Pattern;

void arch_create (EntComponent comp, EntSpeech speech);
void arch_destroy (EntComponent comp, EntSpeech speech);

void arch_setParent (EntComponent comp, EntSpeech speech);
void arch_setPattern (EntComponent comp, EntSpeech speech);

void arch_updatePosition (EntComponent comp, EntSpeech speech);

void arch_expand (EntComponent comp, EntSpeech speech);

void arch_define (EntComponent comp, EntSpeech speech)
{
	component_registerResponse ("arch", "__create", arch_create);
	component_registerResponse ("arch", "__destroy", arch_destroy);

	component_registerResponse ("arch", "setArchParent", arch_setParent);
	component_registerResponse ("arch", "setArchPattern", arch_setPattern);

	component_registerResponse ("arch", "positionSet", arch_updatePosition);

	component_registerResponse ("arch", "archExpand", arch_expand);
}

void arch_create (EntComponent comp, EntSpeech speech)
{
	Entity
		this = component_entityAttached (comp);
	Arch
		arch = xph_alloc (sizeof (struct xph_arch));
	hexPos
		pos;
	memset (arch, 0, sizeof (struct xph_arch));

	arch->connectedArches = dynarr_create (2, sizeof (Entity));
	arch->subarches = dynarr_create (2, sizeof (Entity));

	component_setData (comp, arch);

	if ((pos = position_get (this)))
		subhexAddArch (map_posBestMatchPlatter (pos), this);
}

void arch_destroy (EntComponent comp, EntSpeech speech)
{
	Arch
		arch = component_getData (comp);
	dynarr_destroy (arch->connectedArches);
	dynarr_destroy (arch->subarches);
	xph_free (arch);

	component_clearData (comp);
}

void arch_setParent (EntComponent comp, EntSpeech speech)
{
	Arch
		arch = component_getData (comp);
	arch->parent = speech->arg;
}

void arch_setPattern (EntComponent comp, EntSpeech speech)
{
	Arch
		arch = component_getData (comp);
	arch->pattern = speech->arg;
}

void arch_updatePosition (EntComponent comp, EntSpeech speech)
{
	Entity
		this = component_entityAttached (comp);
	hexPos
		pos;
	SUBHEX
		platter;
	// TODO: if the arch is moving (?!) remove arch from old platter
	pos = position_get (this);
	if (!pos)
	{
		ERROR ("can't update arch (%p, #%d) position: no position", this, entity_GUID (this));
		return;
	}
	printf ("set position for arch #%d\n", entity_GUID (this));
	platter = map_posBestMatchPlatter (pos);
	if (subhexSpanLevel (platter) == 0)
		platter = subhexParent (platter);
	subhexAddArch (platter, this);
	/* if the position or any of its adjacent platters are imprinted they're going to need to be re-imprinted due to this new arch. this isn't the best way to do it (since depending on how arches and platters are loaded this could result in a /lot/ of wasted calculation, given how intensive the imprinting process can be) but it's /a/ way, and that's what matters right now. FIXME later once patterns do more things and the loading process has been actually coded. - xph 2011 12 23 */
	subhexResetLoadStateForNewArch (platter);
}

void arch_expand (EntComponent comp, EntSpeech speech)
{
	Entity
		this = component_entityAttached (comp);
	Arch
		arch = component_getData (comp);
	hexPos
		pos;

	int
		i = 0,
		childNumber = 10;
	Entity
		child;
	hexPos
		childPos;

	if (arch->expanded)
		return;

	pos = position_get (this);

	while (i < childNumber)
	{
		child = entity_create ();
		component_instantiate ("position", child);
		component_instantiate ("arch", child);
		childPos = map_randomPositionNear (pos, 2);
		map_posSwitchFocus (childPos, 1);
		position_set (child, childPos);

		entity_message (child, this, "setArchParent", this);
		dynarr_push (arch->subarches, child);

		i++;
	}

	arch->expanded = true;
}


void pattern_create (EntComponent comp, EntSpeech speech);
void pattern_destroy (EntComponent comp, EntSpeech speech);


void pattern_define (EntComponent comp, EntSpeech speech)
{

	component_registerResponse ("pattern", "__create", pattern_create);
	component_registerResponse ("pattern", "__destroy", pattern_destroy);

}

void pattern_create (EntComponent comp, EntSpeech speech)
{
	Pattern
		pattern = xph_alloc (sizeof (struct xph_pattern));
	memset (pattern, 0, sizeof (struct xph_pattern));
	pattern->potentialSubpatterns = dynarr_create (2, sizeof (Entity));

	component_setData (comp, pattern);
}

void pattern_destroy (EntComponent comp, EntSpeech speech)
{
	Pattern
		pattern = component_getData (comp);
	dynarr_destroy (pattern->potentialSubpatterns);
	xph_free (pattern);

	component_clearData (comp);
}

/***
 * BUILDERS
 */

void builder_create (EntComponent comp, EntSpeech speech);
void builder_destroy (EntComponent comp, EntSpeech speech);
void builder_inputResponse (EntComponent comp, EntSpeech speech);

void builder_define (EntComponent comp, EntSpeech speech)
{
	component_registerResponse ("builder", "__create", builder_create);
	component_registerResponse ("builder", "__destroy", builder_destroy);

	component_registerResponse ("builder", "CONTROL_INPUT", builder_inputResponse);
}

void builder_create (EntComponent comp, EntSpeech speech)
{
	
}

void builder_destroy (EntComponent comp, EntSpeech speech)
{
	
}

void builder_inputResponse (EntComponent comp, EntSpeech speech)
{
	Entity
		this = component_entityAttached (comp);
	const struct input_event
		* input = speech->arg;

	switch (input->ir)
	{
		case IR_WORLD_PLACEARCH:
			// FIXME: extra information this message needs to transmit somehow: the centre of the blueprint, the blueprint data itself (which is probably gonna be more complex than just "this pattern")
			entity_speak (this, "builder:placeBlueprint", NULL);
			break;
		default:
			break;
	}

}

/***
 * BUILDER SYSTEM
 */

void builder_system (Dynarr entities)
{
	EntSpeech
		speech;

	Entity
		newArch;

	while ((speech = entitySystem_dequeueMessage ("builder")))
	{
		if (!strcmp (speech->message, "builder:placeBlueprint"))
		{
			newArch = entity_create ();
			component_instantiate ("position", newArch);
			component_instantiate ("arch", newArch);
			entity_refresh (newArch);
			position_copy (newArch, speech->from);
			printf ("created new arch; entity #%d\n", entity_GUID (newArch));
		}
	}

}
