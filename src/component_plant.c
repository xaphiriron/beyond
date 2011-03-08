#include "component_plant.h"

struct plantData
{
	char
		* growth;
	LSYSTEM
		* growthRules;
	struct plantSymbolSet
		* symbols;
	Dynarr
		branches;
};

struct plantSymbol
{
	char
		x;
	enum plantSymType
	{
		BRANCH_MODIFIER = 1,
		POINTER_MODIFIER,
	} type;
	char
		* action;
	Dynarr
		args;
};

struct plantSymbolSet
{
	Dynarr
		symbols;
};

/*
#define PLANT_MAX_SIZE 512
struct plantBranch
{
	unsigned int
		connects,
		id;
	unsigned char
		size,
		connectionOffset;
};
struct plantHierarchy
{
	struct plantBranch
		plant [PLANT_MAX_SIZE];
};
*/

#define PLANT_BRANCH_MAX	128
struct plantBranch
{
	VECTOR3
		base;
	SPH3
		orient;
	unsigned int
		id,
		parent;
	unsigned char
		parentConnectionOffset,
		growthUsed,
		growth[PLANT_BRANCH_MAX];
};

static struct plantData * plant_createEmpty ();
static void plant_destroy (struct plantData * plant);

static struct plantSymbol * plant_specifySymbol (const char x, enum plantSymType type, const char * action, int args, ...);
static void plant_destroySymbol (struct plantSymbol *);

static struct plantSymbolSet * plant_createSymbolSet ();
static void plant_destroySymbolSet (struct plantSymbolSet * set);
static bool plant_addSymbol (struct plantSymbolSet * set, struct plantSymbol * sym);
static const struct plantSymbol * plant_getSymbol (const struct plantSymbolSet * set, const char x);

static int sym_sort (const void * a, const void * b);
static int sym_search (const void * k, const void * d);

static Dynarr plant_generateBranches (const char * growth, const struct plantSymbolSet * set);
static void plant_destroyBranches (Dynarr branches);

/*
static bool plant_parseSymbol (const char sym, const struct plantSymbolSet * set, VECTOR3 * pos, SPH3 * orient, VECTOR3 * align);
*/

/*
static struct plantHierarchy * plant_calculateHierarchy (const char * growth, const struct plantSymbol * symbols);
static void plant_destroyHierarchy (struct plantHierarchy *);
*/


static struct plantData * plant_createEmpty ()
{
	struct plantData
		* pd = xph_alloc (sizeof (struct plantData));
	pd->growth = NULL;
	pd->symbols = plant_createSymbolSet ();
	pd->growthRules = lsystem_create ();
	pd->branches = dynarr_create (2, sizeof (struct plantBranch *));
	return pd;
}

static void plant_destroy (struct plantData * plant)
{
	plant_destroyBranches (plant->branches);
	lsystem_destroy (plant->growthRules);
	plant_destroySymbolSet (plant->symbols);
	xph_free (plant->growth);
	xph_free (plant);
}

/*
	some token strings:
	(these are generic placeholders. the idea is each tree would have an alphabet set, and so e.g., if a plant's alphabet is ABCD then when these values are generated for the plant's iterator, all instances of 'X' will be replaced with one of those four values. the \/-+ tokens will probably remain the same but have different values depending)

	X -> XX

	X -> X[`:X]
	X -> X[`[`:X]:X]
	X -> X[`[`[`:X]:X]:X]
	^^^ etc

	X -> X[-:X]X
	X -> X[-:X]X[+:X]X
	X -> X[-:X]X[+:X]X[-X:]X
	^^^ etc

	X -> -X+X-
 */

static struct plantSymbol * plant_specifySymbol (const char x, enum plantSymType type, const char * action, int argc, ...)
{
	struct plantSymbol
		* sym = xph_alloc (sizeof (struct plantSymbol));
	va_list
		argv;
	switch (x)
	{
		case ':':
		case '[':
		case ']':
			fprintf (stderr, "%s: Error: the character \"%c\" is a reserved symbol. (Attempted to specify it as a %s modifier.)", __FUNCTION__, x, type == BRANCH_MODIFIER ? "branch" : "pointer");
			xph_free (sym);
			return NULL;
			break;
		default:
			break;
	}
	sym->x = x;
	sym->type = type;
	if (action != NULL)
	{
		sym->action = xph_alloc (strlen (action) + 1);
		strcpy (sym->action, action);
	}
	else
	{
		sym->action = NULL;
	}
	sym->args = dynarr_create (argc + 1, sizeof (int));
	va_start (argv, argc);
	while (argc > 0)
	{
		dynarr_push (sym->args, va_arg (argv, int));
		argc--;
	}
	va_end (argv);
	return sym;
}

static void plant_destroySymbol (struct plantSymbol * sym)
{
	dynarr_destroy (sym->args);
	if (sym->action != NULL)
		xph_free (sym->action);
	xph_free (sym);
}

static struct plantSymbolSet * plant_createSymbolSet ()
{
	struct plantSymbolSet
		* set = xph_alloc (sizeof (struct plantSymbolSet));
	set->symbols = dynarr_create (3, sizeof (struct plantSymbol *));
	return set;
}

static void plant_destroySymbolSet (struct plantSymbolSet * set)
{
	dynarr_wipe (set->symbols, (void (*)(void *))plant_destroySymbol);
	dynarr_destroy (set->symbols);
	xph_free (set);
}

static bool plant_addSymbol (struct plantSymbolSet * set, struct plantSymbol * sym)
{
	struct plantSymbol
		* match = *(struct plantSymbol **)dynarr_search (set->symbols, sym_search, sym->x);
	if (match != NULL)
		return FALSE;
	dynarr_push (set->symbols, sym);
	dynarr_sort (set->symbols, sym_sort);
	return TRUE;
}

static const struct plantSymbol * plant_getSymbol (const struct plantSymbolSet * set, const char x)
{
	struct plantSymbol
		* match = *(struct plantSymbol **)dynarr_search (set->symbols, sym_search, x);
	return match;
}

static int sym_sort (const void * a, const void * b)
{
	//printf ("%s: got %p/%p & %p/%p\n", __FUNCTION__, a, *(void **)a, b, *(void **)b);
	return (*(struct plantSymbol **)a)->x - (*(struct plantSymbol **)b)->x;
}

static int sym_search (const void * k, const void * d)
{
	//printf ("%s: got %p/%p & %p/%p\n", __FUNCTION__, k, *(void **)k, d, *(void **)d);
	return (*(char *)k) - (*(struct plantSymbol **)d)->x;
}

bool plant_createRandom (Entity e)
{
	Component
		plantComponent = entity_getAs (e, "plant");
	struct plantData
		* plantData = NULL;
	if (plantComponent != NULL)
		return FALSE;
	component_instantiateOnEntity ("plant", e);
	// ... this ought to initialize the plant data to an empty struct
	plantComponent = entity_getAs (e, "plant");
	plantData = component_getData (plantComponent);
	// TODO: THIS IS NOT RANDOM AT ALL >:E
	lsystem_addProduction (plantData->growthRules, 'R', "-X['['[':X]:X]:X]");
	lsystem_addProduction (plantData->growthRules, 'X', "X[+:R]X[-:R]X");
	plant_addSymbol (plantData->symbols, plant_specifySymbol ('X', BRANCH_MODIFIER, "grow", 1, 1));
	plant_addSymbol (plantData->symbols, plant_specifySymbol ('\'', POINTER_MODIFIER, "azimuth", 1, 85));
	plant_addSymbol (plantData->symbols, plant_specifySymbol ('-', POINTER_MODIFIER, "inclination", 1, -32));
	plant_addSymbol (plantData->symbols, plant_specifySymbol ('+', POINTER_MODIFIER, "inclination", 1, 32));
	plantData->growth = xph_alloc (3);
	strcpy (plantData->growth, ":R");
	plant_destroyBranches (plantData->branches);
	plantData->branches = plant_generateBranches (plantData->growth, plantData->symbols);
	plant_grow (plantData);
	plant_grow (plantData);
	plant_grow (plantData);
	return TRUE;
}

void plant_grow (plantData plant)
{
	char
		* t;
	int
		 i = 0;
	struct plantBranch
		* b;
	t = lsystem_iterate (plant->growth, plant->growthRules, 1);
	xph_free (plant->growth);
	plant->growth = t;
	plant_destroyBranches (plant->branches);
	plant->branches = plant_generateBranches (plant->growth, plant->symbols);

	printf ("total growth: \"%s\"\n", plant->growth);
	while ((b = *(struct plantBranch **)dynarr_at (plant->branches, i++)) != NULL)
	{
		printf ("branch #%d: \"%s\" (connects to %d at %d)\n", b->id, b->growth, b->parent, b->parentConnectionOffset);
	}
}

void plant_draw (Entity e, CameraGroundLabel label)
{
	struct plantData
		* pd = component_getData (entity_getAs (e, "plant"));
	VECTOR3
		offset = label_getOriginOffset (label),
		loc = position_getLocalOffset (e),
		total = vectorAdd (&offset, &loc);
	glPushMatrix ();
	glTranslatef (total.x, total.y, total.z);

	pd = pd;

	glPopMatrix ();
}

static Dynarr plant_generateBranches (const char * growth, const struct plantSymbolSet * set)
{
	char
		c,
		buffer[PLANT_BRANCH_MAX];
	unsigned char
		bufferUsed = 0;
	int
		i = 0,
		l = strlen (growth),
		id = 1;
	Dynarr
		branches = dynarr_create (2, sizeof (struct plantBranch *)),
		stack = dynarr_create (2, sizeof (struct plantBranch *)),
		bufferStack = dynarr_create (2, 1);
	struct plantBranch
		* b = NULL,
		* t = NULL;
	const struct plantSymbol
		* sym;
	bool
		newFork = FALSE,
		recording = FALSE;
	while (i < l)
	{
		c = growth[i++];
		switch (c)
		{
			case ':':
				newFork = FALSE;
				t = xph_alloc (sizeof (struct plantBranch));
				t->id = id++;
				t->growthUsed = 0;
				memset (t->growth, '\0', PLANT_BRANCH_MAX);
				if (b != NULL)
				{
					t->parent = b->id;
					t->parentConnectionOffset = b->growthUsed;
				}
				else
				{
					t->parent = 0;
					t->parentConnectionOffset = 0;
				}
				if (bufferUsed > 0)
				{
					memcpy (t->growth, buffer, bufferUsed);
					t->growthUsed = bufferUsed;
				}
				dynarr_push (branches, t);
				b = t;
				break;
			case '[':
				dynarr_push (stack, b);
				newFork = TRUE;
				dynarr_push (bufferStack, bufferUsed);
				break;
			case ']':
				bufferUsed = *(unsigned char *)dynarr_pop (bufferStack);
				b = *(struct plantBranch **)dynarr_pop (stack);
				if (b == NULL)
				{
					fprintf (stderr, "%s: Stack underflow at offset %d\n", __FUNCTION__, i-1);
				}
				break;
			default:
				recording = FALSE;
				if (newFork && bufferUsed < PLANT_BRANCH_MAX)
				{
					buffer[bufferUsed++] = c;
					recording = TRUE;
				}
				else if (!newFork && b->growthUsed < PLANT_BRANCH_MAX)
				{
					b->growth[b->growthUsed++] = c;
					recording = TRUE;
				}
				if (recording)
				{
					sym = plant_getSymbol (set, c);
					if (sym == NULL)
						break;
				}
				break;
		}
		//printf ("%c\n", c);
	}
	dynarr_destroy (stack);
	return branches;
}

static void plant_destroyBranches (Dynarr branches)
{
	dynarr_wipe (branches, xph_free);
	dynarr_destroy (branches);
}


static Dynarr comp_entdata = NULL;
int component_plant (Object * obj, objMsg msg, void * a, void * b)
{
	Entity
		e;
	plantData
		pd;
/*
	DynIterator
		it;
*/
	struct comp_message
		* c_msg;
	switch (msg)
	{
		case OM_CLSNAME:
			strncpy (a, "plant", 32);
			return EXIT_SUCCESS;
		case OM_CLSINIT:
			comp_entdata = dynarr_create (16, sizeof (Entity));
			return EXIT_SUCCESS;
		case OM_CLSFREE:
			dynarr_destroy (comp_entdata);
			comp_entdata = NULL;
			return EXIT_SUCCESS;
		case OM_CLSVARS:
		case OM_CREATE:
			return EXIT_FAILURE;
		default:
			break;
	}
	switch (msg)
	{
		case OM_SHUTDOWN:
		case OM_DESTROY:
			obj_destroy (obj);
			return EXIT_SUCCESS;

		case OM_COMPONENT_INIT_DATA:
			e = (Entity)b;
			pd = plant_createEmpty ();
			*(struct plantData **)a = pd;
			dynarr_push (comp_entdata, e);
			return EXIT_SUCCESS;

		case OM_COMPONENT_DESTROY_DATA:
			e = (Entity)b;
			pd = *(struct plantData **)a;
			plant_destroy (pd);
			dynarr_remove_condense (comp_entdata, e);
			return EXIT_SUCCESS;

		case OM_UPDATE:
/*
			it = dynIterator_create (comp_entdata);
			while (!dynIterator_done (it))
			{
				e = *(Entity *)dynIterator_next (it);
				pd = component_getData (entity_getAs (e, "plant"));
			}
			dynIterator_destroy (it);
*/
			return EXIT_FAILURE;

		case OM_POSTUPDATE:
/*
			it = dynIterator_create (comp_entdata);
			while (!dynIterator_done (it))
			{
				e = *(Entity *)dynIterator_next (it);
				pd = component_getData (entity_getAs (e, "plant"));
			}
			dynIterator_destroy (it);
*/
			return EXIT_FAILURE;

		case OM_COMPONENT_RECEIVE_MESSAGE:
			c_msg = a;
			e = component_entityAttached (c_msg->to);
			if (strcmp (c_msg->message, "RENDER") == 0)
			{
				plant_draw (e, b);
			}
			return EXIT_FAILURE;
		case OM_SYSTEM_RECEIVE_MESSAGE:
			return EXIT_FAILURE;

		default:
			return obj_pass ();
	}
	return 0;
}
