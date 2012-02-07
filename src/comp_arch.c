#include "comp_arch.h"

#include "xph_path.h"
#include "ogdl/ogdl.h"
#include "graph_common.h"

#include "component_position.h"
// for arch imprinting
#include "map_internal.h"

typedef struct xph_arch * Arch;

struct pattern_data
{
	// this is where any data that varies by pattern instantiation goes. right now it's just shape size, but this is also where stuff like pattern "completion" goes (expect to radically expand this as the simulation part of patterns gets up and running) - xph 2012 01 13
	unsigned int
		size;
};

struct xph_arch
{
	Entity
		parent;
	Pattern
		pattern;
	bool
		expanded;
	Dynarr
		connectedArches,
		subarches;
	unsigned int
		heightMarker;	// roughly what height index to use when looking for "the ground" as applicable for this pattern

	struct pattern_data
		data;
};

enum pattern_shape
{
	SHAPE_CIRCLE,
	SHAPE_HEX,
};

struct xph_pattern_shape_radial
{
	enum pattern_shape
		type;
	int
		radius[2];	// instantiate with a value between the two
};

// this should also have a shape: something simple and statistical, which constrains the graph expansion of an arch using this pattern. something simple would be "a sphere", something more complex would be a density spread or a generic graph model.
union xph_pattern_shape
{
	enum pattern_shape
		type;
	struct xph_pattern_shape_radial
		radial;
};

typedef union xph_pattern_shape patternShape;

struct xph_pattern_arg
{
	char
		name[32];
	union xph_pattern_arg_value
	{
		enum xph_pattern_arg_type
		{
			PATTERN_ARG_INT,
			PATTERN_ARG_INT_RANGE,
		} type;
		struct xph_pattern_arg_int
		{
			enum xph_pattern_arg_type
				type;
			int
				val;
		} intval;
		struct xph_pattern_arg_int_range
		{
			enum xph_pattern_arg_type
				type;
			int
				range[2];
		} intrange;
	} val;
};

enum xph_pattern_imprint_type
{
	IMP_HEIGHT,
	IMP_SMOOTH,
	IMP_TRANSMUTE,
};

enum xph_pattern_location
{
	PL_INVALID,
	PL_CENTRE,
	PL_INSIDE,
	PL_BORDER
};

enum xph_pattern_scale
{
	PS_CONSTANT,
	PS_LINEAR,
	PS_SINE,
	PS_EXPONENT,
};

struct pattern_imprint
{
	enum xph_pattern_imprint_type
		type;
	enum xph_pattern_location
		target;
	enum xph_pattern_scale
		scale;
	long
		value;
};
typedef struct pattern_imprint * ImprintRule;



struct xph_pattern_subpattern
{
	char
		name[32];
	struct xph_pattern
		* pattern;
};

struct xph_pattern
{
	int
		id;
	char
		name[32];
	patternShape
		shape;
	
	Dynarr
		args,
		imprints,
		subpatterns;
};

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

	arch->heightMarker = 1024;
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
		arch = component_getData (comp),
		parent = component_getData (entity_getAs (speech->arg, "arch"));
	arch->parent = speech->arg;
	
	// TODO: do something to calculate the arch's ground height marker based on the parent arch's height marker and any height imprinting rules instead of just blindly copying it over
	arch->heightMarker = parent->heightMarker;
}

unsigned int randBetween (unsigned int lo, unsigned int hi)
{
	return lo + (rand () % (hi - lo));
}

void arch_setPattern (EntComponent comp, EntSpeech speech)
{
	Arch
		arch = component_getData (comp);
	arch->pattern = speech->arg;
	if (!arch->pattern)
		return; // maybe also spit out a warning
	// TODO: instantiate any applicable pattern variables (size, etc)
	switch (arch->pattern->shape.type)
	{
		case SHAPE_CIRCLE:
		case SHAPE_HEX:
			if (arch->pattern->shape.radial.radius[0] != arch->pattern->shape.radial.radius[1])
			{
				arch->data.size = randBetween (arch->pattern->shape.radial.radius[0], arch->pattern->shape.radial.radius[1]);
			}
			else
				arch->data.size = arch->pattern->shape.radial.radius[0];
			break;
		default:
			arch->data.size = 1;
			break;
	}
	//printf ("set size of arch #%d to %d\n", entity_GUID (component_entityAttached (comp)), arch->data.size);
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
	//printf ("set position for arch #%d\n", entity_GUID (this));
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
	struct xph_pattern_subpattern
		* sub;

	int
		i = 0,
		childNumber = 20,
		subpatterns = 0;
	Entity
		child;
	hexPos
		childPos;

	if (arch->expanded)
		return;

	pos = position_get (this);
	subpatterns = dynarr_size (arch->pattern->subpatterns);
	if (!subpatterns)
	{
		arch->expanded = true;
		return;
	}

	while (i < childNumber)
	{
		child = entity_create ();
		component_instantiate ("position", child);
		component_instantiate ("arch", child);
		childPos = map_randomPositionNear (pos, 3);
		map_posSwitchFocus (childPos, 1);
		position_set (child, childPos);

		entity_message (child, this, "setArchParent", this);
		dynarr_push (arch->subarches, child);

		sub = *(struct xph_pattern_subpattern **)dynarr_at (arch->pattern->subpatterns, rand () % subpatterns);
		entity_message (child, this, "setArchPattern", sub->pattern);

		i++;
	}

	arch->expanded = true;
}

void arch_imprint (Entity archEntity, SUBHEX at)
{
	Arch
		arch = component_getData (entity_getAs (archEntity, "arch"));
	Pattern
		pattern = arch->pattern;
	ImprintRule
		imprint;
	int
		i = 0,
		j = 0;

	hexPos
		archFocus,
		hexPosition;
	HEXSTEP
		groundStep;
	Dynarr
		hexHull = NULL;
	SUBHEX
		hex;
	VECTOR3
		distance;
	unsigned int
		shiftAmount = 0;

	float
		scaledDistance = 0.0;

	if (!pattern)
	{
		WARNING ("Arch #%d has no pattern set; cannot use", entity_GUID (archEntity));
		return;
	}

	archFocus = position_get (archEntity);
	if (map_posFocusedPlatter (archFocus) != NULL && subhexSpanLevel (map_posFocusedPlatter (archFocus)) != 0)
	{
		/* okay so after this we get the hex hull (the set of all individual hexes that are loaded and could possibly be within the boundary of the arch effect) and that requires using the position of the arch focus. HOWEVER:
		 * - if the platter it's focused on isn't loaded then the lookup will fail (since it uses a SUBHEX instead of a hexPos; a null SUBHEX could be anywhere)
		 * - if the platter it's focused on isn't an individual hex the resultant hull won't be formed out of hexes, it'll be formed out of subdivs (since map_posAround returns subhexes of the same span as the subhex that was passed in) and that won't work for all the tile data setting we're going to do here
		 * both of these problems can be fixed: we can write an "around" function that takes hexPos structs and thus can return valid data even when the target subhex isn't loaded, and we can change the way the hull is calculated (remember we only imprint arches one subhex at a time; if we're dealing with a large pattern then the easy, common case is "the entire subhex is inside the hull") to fix the second one. but right now these are not problems that are going to come up with the rather trivial style of imprinting we're doing right now, so right now instead of fixing all that code i'm just going to pretend none of it exists.
		 *  - xph 2012 01 06
		 */
		if (subhexSpanLevel (map_posFocusedPlatter (archFocus)) == 1)
		{
			// TODO: try to phase out usage of map_posAround in favor of hexPos_around since the latter will not explode (by which i mean crash) if the subhex in question isn't loaded - xph 2012 02 07
			hexHull = map_posAround (subhexData (at, 0, 0), mapGetRadius ());
		}
		else
		{
			ERROR ("got a arch type we cannot handle yet. focus is either null (%p) or > 1 (%d)", map_posFocusedPlatter (archFocus), subhexSpanLevel (map_posFocusedPlatter (archFocus)));
			return;
		}
	}
	else
	{
		// given a large enough size or a close enough platter this could just be every hex in the platter - xph 2012 01 14
		hexHull = hexPos_around (archFocus, hexPos_focus (archFocus), arch->data.size * 1.4);
	}

	// what we could also do here is cull the hexes in the hull that aren't in the shape at all, so that we can operate on all the remaining ones (which we might be iterating through many times) without having to sanity-check them. (it might be useful to pre-generate the hull and use subsets of that for each imprinting but given that the pre-generation would probably happen when a bunch of platters inside the hull are unloaded it wouldn't be worth it i think)

	// every use of 52.0 here is using it as the distance between the centre of two adjacent hexes, right now usually to test the distance against the hardcoded pattern hull limit of eight hexes distant (in a circle)

	while ((imprint = *(ImprintRule *)dynarr_at (pattern->imprints, i++)))
	{
		j = 0;
		while ((hexPosition = *(hexPos *)dynarr_at (hexHull, j++)))
		{
			hex = map_posFocusedPlatter (hexPosition);
			//printf ("i: %d; hexPos: %p; got hex %p w/ parent %p (looking for %p)\n",i-1, hexPosition, hex, hex == NULL ? NULL : subhexParent (hex), at);
			// while any loaded hexes are fair game, imprinting happens at the platter level, so any loaded hexes in the hull have either already been imprinted or will be imprinted, so this is to avoid doubly-imprinting anything. - xph 2012 01 13
			if (!hex || subhexParent (hex) != at)
				continue;
			groundStep = hexGroundStepNear (&hex->hex, arch->heightMarker);
			distance = mapDistance (archFocus, hexPosition);
			// TODO: don't use a huge if/else here; have each shape have its own distance metric function that requires, max, the shape data + vector distance, that returns true/false for "vector distance within shape"
			if (arch->pattern->shape.type == SHAPE_CIRCLE)
			{
				scaledDistance = vectorMagnitude (&distance) / (52. * arch->data.size);
				if ((vectorMagnitude (&distance) / 52.) > arch->data.size)
					continue;
			}
			else if (arch->pattern->shape.type == SHAPE_HEX)
			{
				int
					x = 0, y = 0;
				v2c (&distance, &x, &y);

				scaledDistance = (float)hexMagnitude (x, y) / arch->data.size;
				if (hexMagnitude (x, y) > arch->data.size)
					continue;
			}
			else
			{
				scaledDistance = 1;
			}

			switch (imprint->type)
			{
				case IMP_HEIGHT:
					switch (imprint->scale)
					{
						case PS_LINEAR:
							shiftAmount = imprint->value - (scaledDistance * imprint->value);
							break;
						case PS_SINE:
							shiftAmount = imprint->value - (((sin (scaledDistance * M_PI - M_PI_2) + 1) / 2.0) * imprint->value);
							break;
						case PS_EXPONENT:
							// FIXME: wow i don't know what i'm doing here, also maybe having a log as well as an exp value would be good
							shiftAmount = imprint->value - (exp (scaledDistance) * imprint->value) + imprint->value;
							break;
						case PS_CONSTANT:
						default:
							shiftAmount = imprint->value;
							break;
					}
					stepShiftHeight (&hex->hex, groundStep, shiftAmount);
					break;
				case IMP_SMOOTH:
					// convert the distance into a vector, check the "vectors" of the vertices, slope to match adjacent hexes
					// take the 2/3 corners in the direction of the distance normal and slope them in the direction of whatever matching corners exist
					break;
				case IMP_TRANSMUTE:
					//printf ("transmuting to %ld (%p)\n", imprint->value, material (imprint->value));
					stepTransmute (&hex->hex, groundStep, material (imprint->value), 2);
					break;
				default:
					break;
			}
		}
	}

	dynarr_wipe (hexHull, (void (*)(void *))map_freePos);
	dynarr_destroy (hexHull);

}

/***
 * PATTERNS!
 */

static Dynarr
	PatternList = NULL;
static char
	PathBuffer[32];

static void parseShapeArg (Pattern context, Graph arg);
static void parseSubvarArg (Pattern context, Graph var);
static void parseImprintArg (Pattern context, Graph var);
static void parseSubpatterns (Pattern context, Graph var);

Pattern pattern_create ();
void pattern_destroy (Pattern pattern);

Pattern patternGet (unsigned int id)
{
	assert (PatternList != NULL);
	return *(Pattern *)dynarr_at (PatternList, id);
}

Pattern patternGetByName (const char * name)
{
	DynIterator
		it;
	Pattern
		p;

	assert (PatternList != NULL);
	it = dynIterator_create (PatternList);
	while ((p = *(Pattern *)dynIterator_next (it)))
	{
		if (strcmp (p->name, name) == 0)
		{
			dynIterator_destroy (it);
			return p;
		}
	}
	dynIterator_destroy (it);
	return NULL;
}

void patternLoadDefinitions (char * path)
{
	Graph
		patternRoot,
		patternNode,

		idNode,
		nameNode,
		shapeNode,
		imprintNode,
		subvarNode,
		subpatternNode,
		superpatternsNode;
	Pattern
		pattern = NULL;

	struct xph_pattern_subpattern
		* subpattern;
	int
		i = 0,
		j = 0;

	PatternList = dynarr_create (8, sizeof (struct xph_pattern));
	patternRoot = Ogdl_load (path);
	if (!patternRoot)
	{
		ERROR ("Could not load patterns file; expected it in %s", path);
		return;
	}

	while (1)
	{
		sprintf (PathBuffer, "pattern[%d]", i++);
		patternNode = Graph_get (patternRoot, PathBuffer);
		if (!patternNode)
			break;

		pattern = pattern_create ();

		idNode = Graph_get (patternNode, "id.[0]");
		nameNode = Graph_get (patternNode, "name.[0]");
		if (!idNode || !nameNode)
		{
			WARNING ("Invalid pattern specification");
			xph_free (pattern);
			continue;
		}
		// TODO: check for id validity as a number and as compared to existing patterns; ditto with names
		pattern->id = atoi (idNode->name);
		strncpy (pattern->name, nameNode->name, 32);
		printf ("got #%d: \"%s\"\n", pattern->id, pattern->name);

		shapeNode = Graph_get (patternNode, "shape");
		if (shapeNode)
			graph_parseNodeArgs (shapeNode, (void (*)(void *, Graph))parseShapeArg, pattern);

		subvarNode = Graph_get (patternNode, "subvars");
		if (subvarNode)
			graph_parseNodeArgs (subvarNode, (void (*)(void *, Graph))parseSubvarArg, pattern);

		imprintNode = Graph_get (patternNode, "imprint");
		if (imprintNode)
			graph_parseNodeArgs (imprintNode, (void (*)(void *, Graph))parseImprintArg, pattern);
		

		subpatternNode = Graph_get (patternNode, "subpatterns");
		if (subpatternNode)
			graph_parseNodeArgs (subpatternNode, (void (*)(void *, Graph))parseSubpatterns, pattern);

		superpatternsNode = Graph_get (patternNode, "superpatterns");

		if (*(Pattern *)dynarr_at (PatternList, pattern->id))
		{
			ERROR ("Duplicate pattern id: more than one pattern has an id of %d; ignoring whichever one came later in the parse order", pattern->id);
		}
		else
			dynarr_assign (PatternList, pattern->id, pattern);
	}
	Graph_free (patternRoot);

	i = 1;
	while ((pattern = *(Pattern *)dynarr_at (PatternList, i++)))
	{
		// check patterns with unlinked sub/super-patterns and link them or error w/ invalid pattern specified
		j = 0;
		printf ("pattern has %d unlinked subpatterns\n", dynarr_size (pattern->subpatterns));
		while ((subpattern = *(struct xph_pattern_subpattern **)dynarr_at (pattern->subpatterns, j++)))
		{
			subpattern->pattern = patternGetByName (subpattern->name);
			printf ("got %p for pattern \"%s\"\n", subpattern->pattern, subpattern->name);
			if (!subpattern->pattern)
			{
				ERROR ("Unlinkable pattern: pattern \"%s\" calls for pattern \"%s\" as a subpattern, but that pattern doesn't exist.", pattern->name, subpattern->name);
			}
		}
	}

}

static void parseShapeArg (Pattern pattern, Graph shape)
{
	const char
		* shapes [] = {"circle", "hex", "point", ""},
		* shapeArgs [] = {"radius", ""};
	int
		argIndex,
		i = 0;
	Graph
		arg,
		radius;

	argIndex = arg_match (shapes, shape->name);
	printf ("got \"%s\"\n", shape->name);
	switch (argIndex)
	{
		case 0:
			pattern->shape.type = SHAPE_CIRCLE;
			break;
		case 1:
			pattern->shape.type = SHAPE_HEX;
			break;
		case 2:
			// i don't actually know what a point shape would imply aside from an inside of a single hex
			break;
		case -1:
		default:
			WARNING ("Unknown pattern shape \"%s\"; ignoring", shape->name);
			break;
	}

	while (1)
	{
		snprintf (PathBuffer, 32, "[%d]", i++);
		arg = Graph_get (shape, PathBuffer);
		if (!arg)
			break;
		argIndex = arg_match (shapeArgs, arg->name);
		switch (argIndex)
		{
			case 0:
				radius = Graph_get (arg, "[0]");
				if (!radius)
				{
					ERROR ("Shape radius parameter requires at least one value.");
					break;
				}
				pattern->shape.radial.radius[0] = strtol (radius->name, NULL, 0);
				if ((radius = Graph_get (arg, "[1]")))
					pattern->shape.radial.radius[1] = strtol (radius->name, NULL, 0);
				else
					pattern->shape.radial.radius[1] = pattern->shape.radial.radius[0];
				break;
			case -1:
			default:
				WARNING ("Unknown shape argument \"%s\"; ignoring", arg->name);
				break;
		}
	}
}

static void parseSubvarArg (Pattern context, Graph var)
{
}

static void parseImprintArg (Pattern context, Graph var)
{
	struct pattern_imprint
		* imprint = xph_alloc (sizeof (struct pattern_imprint));
	Graph
		arg;
	int
		i = 0,
		argIndex;
	char
		* conversionError = NULL;
	const char
		* imprintTypes [] = {"height", "smooth", "transmute", ""},
		* imprintArgs [] = {"target", "scale", "from", "value", ""},
		* targets [] = {"centre", "inside", "border", ""},
		* scales [] = {"constant", "linear", "sine", "exponent", ""},
		* materials [] = {"air", "stone", "dirt", "grass", "sand", "snow", "silt", "water", ""};
	
	argIndex = arg_match (imprintTypes, var->name);
	printf ("got %s w/ index of %d\n", var->name, argIndex);
	switch (argIndex)
	{
		case 0:
			imprint->type = IMP_HEIGHT;
			break;
		case 1:
			imprint->type = IMP_SMOOTH;
			break;
		case 2:
			imprint->type = IMP_TRANSMUTE;
			break;

		case -1:
		default:
			WARNING ("Unknown imprinting rule \"%s\"; ignoring", var->name);
			xph_free (imprint);
			return;
	}

	while (1)
	{
		snprintf (PathBuffer, 32, "[%d]", i++);
		arg = Graph_get (var, PathBuffer);
		if (!arg)
			break;
		argIndex = arg_match (imprintArgs, arg->name);
		switch (argIndex)
		{
			case 0: // "target"
				arg = Graph_get (arg, "[0]");
				argIndex = arg_match (targets, arg->name);
				switch (argIndex)
				{
					case 0:	// "centre"
						imprint->target = PL_CENTRE;
						break;
					case 1:	// "inside"
						imprint->target = PL_INSIDE;
						break;
					case 2:	// "border"
						imprint->target = PL_BORDER;
						break;
					case -1:
					default:
						WARNING ("Unknown pattern area target of \"%s\"; ignoring", arg->name);
						break;
				}
				break;

			case 1: // "scale"
				arg = Graph_get (arg, "[0]");
				argIndex = arg_match (scales, arg->name);
				switch (argIndex)
				{
					case 0: // "constant"
						imprint->scale = PS_CONSTANT;
						break;
					case 1:	// "linear"
						imprint->scale = PS_LINEAR;
						break;
					case 2:	// "sine"
						imprint->scale = PS_SINE;
						break;
					case 3:	// "exponent"
						imprint->scale = PS_EXPONENT;
						break;
					case -1:
					default:
						WARNING ("Unknown pattern area target of \"%s\"; ignoring", arg->name);
						break;
				}
				break;

			case 2: // "from"
				break;

			case 3: // "value"
				arg = Graph_get (arg, "[0]");
				imprint->value = strtol (arg->name, &conversionError, 0);
				if (*conversionError != 0)
				{
					if (imprint->type == IMP_TRANSMUTE)
					{
						argIndex = arg_match (materials, arg->name);
						if (argIndex != -1)
						{
							imprint->value = argIndex;
							break;
						}
					}
					ERROR ("Could not parse value \"%s\"; using 0", arg->name);
					imprint->value = 0;
				}
				break;

			case -1:
			default:
				WARNING ("Unknown imprinting arg of \"%s\"; ignoring", arg->name);
				break;
		}
	}
	dynarr_push (context->imprints, imprint);
}

static void parseSubpatterns (Pattern context, Graph var)
{
	struct xph_pattern_subpattern
		* sub = xph_alloc (sizeof (struct xph_pattern_subpattern));
	printf (" -> stored subpattern \"%s\"\n", var->name);
	strncpy (sub->name, var->name, 32);

	dynarr_push (context->subpatterns, sub);
}

Pattern pattern_create ()
{
	Pattern
		pattern = xph_alloc (sizeof (struct xph_pattern));
	pattern->subpatterns = dynarr_create (2, sizeof (Entity));
	pattern->args = dynarr_create (2, sizeof (struct xph_pattern_arg *));
	pattern->imprints = dynarr_create (2, sizeof (struct pattern_imprint *));

	return pattern;
}

void pattern_destroy (Pattern pattern)
{

	// TODO: free the imprints/args/etc
	dynarr_destroy (pattern->imprints);
	dynarr_destroy (pattern->args);

	dynarr_destroy (pattern->subpatterns);
	xph_free (pattern);
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

	component_registerResponse ("builder", "FOCUS_INPUT", builder_inputResponse);
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

	switch (input->code)
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
			entity_message (newArch, NULL, "setArchPattern", patternGet (1));
			position_copy (newArch, speech->from);
			printf ("created new arch; entity #%d\n", entity_GUID (newArch));
		}
	}

}
