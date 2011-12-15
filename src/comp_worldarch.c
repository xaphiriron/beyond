#include "comp_worldarch.h"

#include "map.h"
#include "component_position.h"

struct xph_worldarch
{
	unsigned int
		scale;
	Entity
		parent,
		pattern;
	bool
		expanded;
	Dynarr
		connectedArches,
		subarches;
};

typedef struct xph_worldarch * worldArch;

struct xph_pattern
{
	Dynarr
		potentialSubpatterns;
	// this should also have a shape: something simple and statistical, which constrains the graph expansion of an arch using this pattern. something simple would be "a sphere", something more complex would be a density spread or a generic graph model.
};

typedef struct xph_pattern * Pattern;

void worldarch_create (EntComponent comp, EntSpeech speech);
void worldarch_destroy (EntComponent comp, EntSpeech speech);

void worldarch_setParent (EntComponent comp, EntSpeech speech);
void worldarch_setPattern (EntComponent comp, EntSpeech speech);

void worldarch_updatePosition (EntComponent comp, EntSpeech speech);

void worldarch_define (EntComponent comp, EntSpeech speech)
{

	component_registerResponse ("worldArch", "__create", worldarch_create);
	component_registerResponse ("worldArch", "__destroy", worldarch_destroy);

	component_registerResponse ("worldArch", "setArchParent", worldarch_setParent);
	component_registerResponse ("worldArch", "setArchPattern", worldarch_setPattern);

	component_registerResponse ("worldArch", "positionSet", worldarch_updatePosition);

}

void worldarch_create (EntComponent comp, EntSpeech speech)
{
	Entity
		this = component_entityAttached (comp);
	worldArch
		arch = xph_alloc (sizeof (struct xph_worldarch));
	hexPos
		pos;
	memset (arch, 0, sizeof (struct xph_worldarch));

	arch->connectedArches = dynarr_create (2, sizeof (Entity));
	arch->subarches = dynarr_create (2, sizeof (Entity));

	component_setData (comp, arch);

	if ((pos = position_get (this)))
		subhexAddArch (map_posBestMatchPlatter (pos), this);
}

void worldarch_destroy (EntComponent comp, EntSpeech speech)
{
	worldArch
		arch = component_getData (comp);
	dynarr_destroy (arch->connectedArches);
	dynarr_destroy (arch->subarches);
	xph_free (arch);

	component_clearData (comp);
}

void worldarch_setParent (EntComponent comp, EntSpeech speech)
{
	worldArch
		arch = component_getData (comp);
	arch->parent = speech->arg;
}

void worldarch_setPattern (EntComponent comp, EntSpeech speech)
{
	worldArch
		arch = component_getData (comp);
	arch->pattern = speech->arg;
}

void worldarch_updatePosition (EntComponent comp, EntSpeech speech)
{
	Entity
		this = component_entityAttached (comp);
	hexPos
		pos;
	// TODO: if the arch is moving (?!) remove arch from old platter
	pos = position_get (this);
	if (!pos)
		return;
	subhexAddArch (map_posBestMatchPlatter (pos), this);
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
