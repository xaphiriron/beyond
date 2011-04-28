#include "component_plant.h"

struct plantData
{
	char
		* growth,
		* thickness;
	LSYSTEM
		* growthRules;
	struct plantSymbolSet
		* symbols;
	Dynarr
		branches;
	unsigned short
		growthTimer,
		growthThreshhold,
		growthsTotal,
		growthsTilDeath;
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

static char * plant_generateBranchThickness (const char * growth, const struct plantSymbolSet * set);
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
	pd->growthsTilDeath = 0;
	pd->growthsTotal = 0;
	pd->growthTimer = 0;
	pd->growthThreshhold = 0;
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
	EntComponent
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
	lsystem_addProduction (plantData->growthRules, 'R', "[-Y+_TZR]['-Y+_TTZR][''-Y+_YTZR]");
	lsystem_addProduction (plantData->growthRules, 'Y', "YY");
	lsystem_addProduction (plantData->growthRules, 'Z', "Z");
	lsystem_addProduction (plantData->growthRules, 'Z', ",");
	lsystem_addProduction (plantData->growthRules, 'Z', ".");
	lsystem_addProduction (plantData->growthRules, 'Z', "<");
	lsystem_addProduction (plantData->growthRules, 'Z', ">");
	plant_addSymbol (plantData->symbols, plant_specifySymbol ('G', BRANCH_MODIFIER, "grow", 1, 8));
	plant_addSymbol (plantData->symbols, plant_specifySymbol ('T', BRANCH_MODIFIER, "grow", 1, 1));
	plant_addSymbol (plantData->symbols, plant_specifySymbol ('Y', BRANCH_MODIFIER, "grow", 1, 1));
	plant_addSymbol (plantData->symbols, plant_specifySymbol ('\'', POINTER_MODIFIER, "twist", 1, 120));
	plant_addSymbol (plantData->symbols, plant_specifySymbol ('<', POINTER_MODIFIER, "turn", 1, -15));
	plant_addSymbol (plantData->symbols, plant_specifySymbol ('>', POINTER_MODIFIER, "turn", 1, 15));
	plant_addSymbol (plantData->symbols, plant_specifySymbol (',', POINTER_MODIFIER, "look", 1, -15));
	plant_addSymbol (plantData->symbols, plant_specifySymbol ('.', POINTER_MODIFIER, "look", 1, 15));
	plant_addSymbol (plantData->symbols, plant_specifySymbol ('-', POINTER_MODIFIER, "look", 1, -60));
	plant_addSymbol (plantData->symbols, plant_specifySymbol ('_', POINTER_MODIFIER, "look", 1, -20));
	plant_addSymbol (plantData->symbols, plant_specifySymbol ('+', POINTER_MODIFIER, "look", 1, 60));
	plantData->growth = xph_alloc (4);
	strcpy (plantData->growth, ":GR");
	plantData->thickness = plant_generateBranchThickness (plantData->growth, plantData->symbols);
	plantData->growthsTilDeath = 6;
	plantData->growthThreshhold = 1200;
	return TRUE;
}

void plant_update (EntComponent pc)
{
	plantData
		pd = component_getData (pc);
	if (pd->growthThreshhold == 0)
		return;
	if (++pd->growthTimer >= pd->growthThreshhold)
	{
		pd->growthTimer -= pd->growthThreshhold;
		if (++pd->growthsTotal > pd->growthsTilDeath)
			entity_message (component_entityAttached (pc), "PLANT_DEATH", NULL);
		else
			plant_grow (pd);
	}
}

void plant_grow (plantData plant)
{
	char
		* t;
	t = lsystem_iterate (plant->growth, plant->growthRules, 1);
	xph_free (plant->growth);
	plant->growth = t;
	xph_free (plant->thickness);
	plant->thickness = plant_generateBranchThickness (plant->growth, plant->symbols);

	//printf ("total growth: \"%s\"\n", plant->growth);
/*
	while ((b = *(struct plantBranch **)dynarr_at (plant->branches, i++)) != NULL)
	{
		printf ("branch #%d: \"%s\" (connects to %d at %d)\n", b->id, b->growth, b->parent, b->parentConnectionOffset);
	}
*/
}

void plant_draw (Entity e, CameraGroundLabel label)
{
	struct plantData
		* pd = component_getData (entity_getAs (e, "plant"));
	VECTOR3
		offset = label_getOriginOffset (label),
		loc = position_getLocalOffset (e),
		total = vectorAdd (&offset, &loc),
		lastPos,
		pos;
	int
		i = 0;
	float
		growthConstant = 10.0;
	char
		c;
	const struct plantSymbol
		* ps;
	Turtle
		ttl = turtleCreate (TURTLE_3D);
	turtleLookUp (ttl, 90.0);
	pos = turtleGetPosition (ttl);
	//glColor3f (0.4, 0.28, 0.15);
	glColor3f (1.0, 0.0, 1.0);
	while ((c = pd->growth[i++]) != 0)
	{
		//printf ("%d: \'%c\'; %d \n", i-1, c, pd->thickness[i-1]);
		glLineWidth (pd->thickness[i-1] / 2.0);
		glBegin (GL_LINE_STRIP);
		lastPos = pos;
		glVertex3f (
			total.x + lastPos.x * growthConstant,
			total.y + lastPos.y * growthConstant,
			total.z + lastPos.z * growthConstant
		);
		//printf ("'%c'\n", c);
		switch (c)
		{
			case '[':
				turtlePushStack (ttl);
				break;
			case ']':
				turtlePopStack (ttl);
				pos = turtleGetPosition (ttl);
/*
				printf ("stack pop; old pos: %5.2f, %5.2f, %5.2f\n", pos.x * growthConstant, pos.y * growthConstant, pos.z * growthConstant);
*/
/*
				glEnd ();
				glBegin (GL_LINE_STRIP);
*/
/*
				printf ("drawing at %5.2f, %5.2f, %5.2f\n",
					total.x + pos.x * growthConstant,
					total.y + pos.y * growthConstant,
					total.z + pos.z * growthConstant
				);
				glVertex3f (
					total.x + pos.x * growthConstant,
					total.y + pos.y * growthConstant,
					total.z + pos.z * growthConstant
				);
*/
				break;
			default:
				ps = plant_getSymbol (pd->symbols, c);
				if (ps == NULL)
				{
					//printf ("no such symbol as '%c'\n", c);
					break;
				}
				if (strcmp (ps->action, "grow") == 0)
				{
					turtleMoveForward (ttl, *(int *)dynarr_at (ps->args, 0));
				}
				else if (strcmp (ps->action, "look") == 0)
				{
					turtleLookUp (ttl, *(int *)dynarr_at (ps->args, 0));
				}
				else if (strcmp (ps->action, "twist") == 0)
				{
					turtleTwistClockwise (ttl, *(int *)dynarr_at (ps->args, 0));
				}
				else if (strcmp (ps->action, "turn") == 0)
				{
					turtleTurnRight (ttl, *(int *)dynarr_at (ps->args, 0));
				}
				pos = turtleGetPosition (ttl);
/*
				printf ("drawing at %5.2f, %5.2f, %5.2f\n",
					total.x + pos.x * growthConstant,
					total.y + pos.y * growthConstant,
					total.z + pos.z * growthConstant
				);
*/
				glVertex3f (
					total.x + pos.x * growthConstant,
					total.y + pos.y * growthConstant,
					total.z + pos.z * growthConstant
				);
		}
		glEnd ();
	}
	turtleDestroy (ttl);
	glLineWidth (1);
}

/*
void plant_draw (Entity e, CameraGroundLabel label)
{
	struct plantData
		* pd = component_getData (entity_getAs (e, "plant"));
	int
		i = 0;
	struct plantBranch
		* b;
	VECTOR3
		offset = label_getOriginOffset (label),
		loc = position_getLocalOffset (e),
		total = vectorAdd (&offset, &loc);
	//glPushMatrix ();

	glLineWidth (8);
	while ((b = *(struct plantBranch **)dynarr_at (pd->branches, i++)) != NULL)
	{
		glColor3f (0.4, 0.28, 0.15);
		glBegin (GL_LINES);
		glVertex3f (total.x, total.y, total.z);
		glVertex3f (total.x, total.y + 100, total.z);
		glEnd ();
	}

	//glPopMatrix ();
}
*/

static char * plant_generateBranchThickness (const char * growth, const struct plantSymbolSet * set)
{
	int
		i = 1,
		l = strlen (growth),
		thickness = 0/*,
		max = 0*/;
	char
// 		c,
		* r = xph_alloc (l);
/*
	Dynarr
		stack = dynarr_create (6, sizeof (int));
*/
/*
	const struct plantSymbol
		* sym;
*/
	thickness = 1;
	while (i < l)
	{
/*
		c = growth[l-i];
		if (c == ']' || i == 1)
			thickness = 1;
		else
		{
			thickness++;
		}
*/
		r[i] = thickness;
		i++;
	}

/*
	while (i < l)
	{
		c = growth[i];
		if (c == '[')
			dynarr_push (stack, thickness);
		else if (c == ']')
			thickness = *(int *)dynarr_pop (stack);
		else
			thickness++;
		r[i] = thickness;
		if (thickness > max)
			max = thickness;
		i++;
	}
	dynarr_destroy (stack);
	stack = NULL;
	i = 0;
	while (i < l)
	{
		r[i] = (max - r[i]) + 1;
		i++;
	}
*/
	return r;
}

/*
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
*/

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
	DynIterator
		it;
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
			it = dynIterator_create (comp_entdata);
			while (!dynIterator_done (it))
			{
				e = *(Entity *)dynIterator_next (it);
				//pd = component_getData (entity_getAs (e, "plant"));
				plant_update (entity_getAs (e, "plant"));
			}
			dynIterator_destroy (it);
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
				return EXIT_SUCCESS;
			}
			else if (strcmp (c_msg->message, "PLANT_DEATH") == 0)
			{
				// FIXME: if there is ever something that is a plant in addition to being other things, this is not a good thing to do
				position_unset (e);
				component_removeFromEntity ("plant", e);
				entity_destroy (e);
				return EXIT_SUCCESS;
			}
			return EXIT_FAILURE;
		case OM_SYSTEM_RECEIVE_MESSAGE:
			return EXIT_FAILURE;

		default:
			return obj_pass ();
	}
	return 0;
}
