#include "component_plant.h"

struct plantData
{
	char
		* growth;
	LSYSTEM
		* growthRules;
	struct plantSymbolSet
		* symbols;
};

struct plantSymbol
{
	char
		x;
	enum plantSymType
	{
		BRANCH_TOKEN = 1,
		BRANCH_MODIFIER,
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

static struct plantData * plant_createEmpty ();
static void plant_destroy (struct plantData * plant);

static struct plantSymbol * plant_specifySymbol (const char x, enum plantSymType type, const char * action, int args, ...);
static void plant_destroySymbol (struct plantSymbol *);

static struct plantSymbolSet * plant_createSymbolSet ();
static void plant_destroySymbolSet (struct plantSymbolSet * set);
static bool plant_addSymbol (struct plantSymbolSet * set, struct plantSymbol * sym);

static int sym_sort (const void * a, const void * b);
static int sym_search (const void * k, const void * d);

//static void plant_parseGrowth (const char * growth, const struct plantSymbolSet * symbols);

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
	return pd;
}

static void plant_destroy (struct plantData * plant)
{
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
	// TODO: LOL THIS IS NOT RANDOM AT ALL VVV
	lsystem_addProduction (plantData->growthRules, 'R', "X['['[':X]:X]:X]");
	lsystem_addProduction (plantData->growthRules, 'X', "X[+:R]X[-:R]X");
	plant_addSymbol (plantData->symbols, plant_specifySymbol (':', BRANCH_TOKEN, NULL, 0));
	plant_addSymbol (plantData->symbols, plant_specifySymbol ('X', BRANCH_MODIFIER, "grow", 1));
	plantData->growth = xph_alloc (3);
	strcpy (plantData->growth, ":R");
	plant_grow (plantData);
	plant_grow (plantData);
	plant_grow (plantData);
	return TRUE;
}

void plant_grow (plantData plant)
{
	char
		* t;
	t = lsystem_iterate (plant->growth, plant->growthRules, 1);
	xph_free (plant->growth);
	plant->growth = t;
}

void plant_draw (Entity e, CameraGroundLabel label)
{
	struct plantData
		* pd = component_getData (entity_getAs (e, "plant"));
	VECTOR3
		offset = label_getOriginOffset (label),
		loc = position_getLocalOffset (e),
		total = vectorAdd (&offset, &loc);
	int
		i,
		l = strlen (pd->growth);
	char
		c;
	glPushMatrix ();
	glTranslatef (total.x, total.y, total.z);
	//printf ("\"%s\"\n", pd->growth);
	while (i < l)
	{
		c = pd->growth[i++];
	}
	glPopMatrix ();
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
