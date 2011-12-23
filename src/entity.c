#include "entity.h"

// this same value is used for entity names, system names, and component specification names. i just don't think it's reasonable to use separate defines for each because i am lazy. - xph 2011 12 20
#define ENT_NAMELENGTH 32

struct entity {
	unsigned int
		guid;

  // this is a local variable to make fetching components from a specific entity faster. It stores the same data as a componentSpec->entities vector, which is to say EntComponents (something todo: this is named "components" whereas the system vector is named "entities". this is confusing and dumb.)
  Dynarr
		components,
		listeners,
		systems;
	struct entity_name_map
		* name;
};

struct messageTrigger
{
	char
		* message;
	Dynarr
		funcs;
};

struct entity_name_map
{
	char
		name[ENT_NAMELENGTH];
	Entity
		entity;
};

struct entity_group_map
{
	char
		name[ENT_NAMELENGTH];
	Dynarr
		members;
};

struct xph_component_specification
{
	char
		comp_name[ENT_NAMELENGTH];
	Dynarr
		entities,
		messageTriggers;

	void (* loaderCallback) (TIMER, EntComponent);
	unsigned char (* weighCallback) (EntComponent);
};
typedef struct xph_component_specification * componentSpec;


struct ent_component
{
	Entity
		e;
	componentSpec
		spec;
	void
		* comp_data;
	unsigned int
		comp_guid;
};

struct xph_entity_system
{
	char
		name[ENT_NAMELENGTH];
	sysFunc
		* update;
	Dynarr
		relevantComponents,		// const char * component names
		relevantEntities,		// Entity,
		messages;				// EntSpeech
};
typedef struct xph_entity_system * entitySystem;


static unsigned int EntityGUIDs = 0;
static unsigned int ComponentGUIDs = 0;

static Dynarr
	ToBeDestroyed = NULL,			// stores guids
	DestroyedEntities = NULL,		// stores guids
	ExistantEntities = NULL,		// stores struct entity *
	CompSpecRegistry = NULL,
	EntitySystemRegistry = NULL,
	UnusedSpeech = NULL,
	EntityNames = NULL,				// stores struct entity_name_map
	EntityGroups = NULL				// stores struct entity_group_map
	;

static int guid_sort (const void * a, const void * b);
static int guid_search (const void * k, const void * d);
static int comp_sort (const void * a, const void * b);
static int comp_search (const void * k, const void * d);
static int spec_sort (const void * a, const void * b);
static int spec_search (const void * k, const void * d);
static int sys_sort (const void * a, const void * b);
static int sys_search (const void * k, const void * d);
static int entname_sort (const void * a, const void * b);
static int entname_search (const void * k, const void * d);
static int entgroup_sort (const void * a, const void * b);
static int entgroup_search (const void * k, const void * d);

static struct messageTrigger * mt_create (const char * message);
static void mt_destroy (struct messageTrigger * mt);
static int mt_sort (const void * a, const void * b);
static int mt_search (const void * k, const void * d);

void entity_freeName (struct entity_name_map * name);

void entity_purge (Entity e);

static componentSpec comp_getSpec (const char * comp_name);
static void component_messageSystem (const char * comp_name, const char * message, void * arg);
static void component_message (EntComponent c, const char * message, void * arg);

static void group_destroy (struct entity_group_map * group);

static void entity_freeSpeech (struct entity_speech * speech);
static entitySystem entitySystem_get (const char * name);
static void entitySystem_removeEntity (entitySystem system, Entity entity);
static bool entitySystem_entityRelevant (const entitySystem system, const Entity entity);
static void entitySystem_messageAll (Entity from, const char * message, void * arg);
static void entitySystem_destroy (entitySystem sys);

static int guid_sort (const void * a, const void * b)
{
	//printf ("%s (%p, %p): %d vs. %d\n", __FUNCTION__, a, b, (*(const struct entity **)a)->guid, (*(const struct entity **)b)->guid);
	return
		(*(const struct entity **)a)->guid -
		(*(const struct entity **)b)->guid;
}
static int guid_search (const void * k, const void * d)
{
	//printf ("%s (%p, %p): %d vs. %d\n", __FUNCTION__, k, d, *(const unsigned int *)k, (*(const struct entity **)d)->guid);
	return
		*(const unsigned int *)k -
		(*(const struct entity **)d)->guid;
}

static int comp_sort (const void * a, const void * b) {
  //printf ("%s: got %d vs. %d\n", __FUNCTION__, (*(const struct ent_component **)a)->e->guid, (*(const struct ent_component **)b)->e->guid);
  return
    (*(const struct ent_component **)a)->e->guid -
    (*(const struct ent_component **)b)->e->guid;
}

static int comp_search (const void * k, const void * d) {
  //printf ("%s: got %d vs. %d\n", __FUNCTION__, (*(const struct entity **)k)->guid, (*(const struct ent_component **)d)->e->guid);
  return
    (*(const struct entity **)k)->guid -
    (*(const struct ent_component **)d)->e->guid;
}

static int spec_sort (const void * a, const void * b) {
  //printf ("%s: got \"%s\" vs. \"%s\"\n", __FUNCTION__, (*(const struct ent_system **)a)->comp_name, (*(const struct ent_system **)b)->comp_name);
  return
    strcmp (
      (*(const componentSpec *)a)->comp_name,
      (*(const componentSpec *)b)->comp_name
    );
}

static int spec_search (const void * k, const void * d) {
  //printf ("%s: got \"%s\" vs. \"%s\"\n", __FUNCTION__, *(char **)k, (*(const struct ent_system **)d)->comp_name);
  return
    strcmp (
      *(char **)k,
      (*(const componentSpec *)d)->comp_name
    );
}

static int sys_sort (const void * a, const void * b)
{
	return strcmp
	(
		(*(const entitySystem *)a)->name,
		(*(const entitySystem *)b)->name
	);
}

static int sys_search (const void * k, const void * d)
{
	return strcmp
	(
		*(char **)k,
		(*(const entitySystem *)d)->name
	);
}


static int entname_sort (const void * a, const void * b)
{
	return strcmp
	(
		(*(const struct entity_name_map **)a)->name,
		(*(const struct entity_name_map **)b)->name
	);
}

static int entname_search (const void * k, const void * d)
{
	return strcmp
	(
		*(char **)k,
		(*(const struct entity_name_map **)d)->name
	);
}

static int entgroup_sort (const void * a, const void * b)
{
	return strcmp
	(
		(*(const struct entity_group_map **)a)->name,
		(*(const struct entity_group_map **)b)->name
	);
}

static int entgroup_search (const void * k, const void * d)
{
	return strcmp
	(
		*(char **)k,
		(*(const struct entity_group_map **)d)->name
	);
}

void entity_freeName (struct entity_name_map * name)
{
	dynarr_remove_condense (EntityNames, name);
	xph_free (name);
}

/***
 * ENTITIES
 */

Entity entity_create ()
{
	struct entity
		* e = xph_alloc (sizeof (struct entity));
	if (DestroyedEntities != NULL && !dynarr_isEmpty (DestroyedEntities))
		e->guid = *(unsigned int *)dynarr_pop (DestroyedEntities);
	else
		e->guid = ++EntityGUIDs;
	if (e->guid == 0)
	{
		ERROR ("Entity GUIDs have wrapped around. Now anything is possible!");
		e->guid = ++EntityGUIDs;
	}
	e->components = dynarr_create (2, sizeof (EntComponent));
	e->listeners = dynarr_create (2, sizeof (Entity));
	e->systems = dynarr_create (2, sizeof (entitySystem));
	if (ExistantEntities == NULL)
		ExistantEntities = dynarr_create (128, sizeof (Entity));
	dynarr_push (ExistantEntities, e);
	dynarr_sort (ExistantEntities, guid_sort);
	return e;
}

void entity_destroy (Entity e)
{
	DEBUG ("Registering entity #%d for destruction", entity_GUID (e));
	if (ToBeDestroyed == NULL)
		ToBeDestroyed = dynarr_create (32, sizeof (unsigned int));
	dynarr_push (ToBeDestroyed, entity_GUID (e));
}


unsigned int entity_GUID (const Entity e)
{
	if (e == NULL)
		return 0;
	return e->guid;
}

bool entity_exists (unsigned int guid)
{
	if (guid > EntityGUIDs || (DestroyedEntities != NULL && in_dynarr (DestroyedEntities, (void *)guid) >= 0))
	{
		return false;
	}
	return true;
}

Entity entity_get (unsigned int guid)
{
	Entity
		e = *(Entity *)dynarr_search (ExistantEntities, guid_search, guid);
	return e;
}

bool entity_name (Entity e, const char * name)
{
	struct entity_name_map
		* map;
	if (EntityNames == NULL)
		EntityNames = dynarr_create (4, sizeof (struct entity_name_map *));
	if (e->name)
	{
		WARNING ("Renaming entity #%d, \"%s\", to \"%s\"", entity_GUID (e), e->name->name, name);
		entity_freeName (e->name);
		e->name = NULL;
	}
	map = *(struct entity_name_map **)dynarr_search (EntityNames, entname_search, name);
	if (map)
	{
		WARNING ("Replacing named entity #%d with new entity, #%d", entity_GUID (map->entity), entity_GUID (e));
		map->entity = e;
		return true;
	}
	map = xph_alloc (sizeof (struct entity_name_map));
	map->entity = e;
	strncpy (map->name, name, ENT_NAMELENGTH);
	e->name = map;
	dynarr_push (EntityNames, map);
	dynarr_sort (EntityNames, entname_sort);
	return true;
}

Entity entity_getByName (const char * name)
{
	struct entity_name_map
		* map;
	if (EntityNames == NULL)
		return NULL;
	map = *(struct entity_name_map **)dynarr_search (EntityNames, entname_search, name);
	if (map)
		return map->entity;
	return NULL;
}

bool entity_addToGroup (Entity e, const char * groupName)
{
	struct entity_group_map
		* group;
	if (EntityGroups == NULL)
		EntityGroups = dynarr_create (4, sizeof (struct entity_group_map *));
	group = *(struct entity_group_map **)dynarr_search (EntityGroups, entgroup_search, groupName);
	if (group)
	{
		dynarr_push (group->members, e);
		return true;
	}
	group = xph_alloc (sizeof (struct entity_group_map));
	strncpy (group->name, groupName, ENT_NAMELENGTH);
	group->members = dynarr_create (4, sizeof (Entity));
	dynarr_push (group->members, e);

	dynarr_push (EntityGroups, group);
	dynarr_sort (EntityGroups, entgroup_sort);
	return true;
}

const Dynarr entity_getGroup (const char * groupName)
{
	struct entity_group_map
		* group;
	if (EntityGroups == NULL)
		return NULL;
	group = *(struct entity_group_map **)dynarr_search (EntityGroups, entgroup_search, groupName);
	if (!group)
		return NULL;
	return group->members;
}

static void group_destroy (struct entity_group_map * group)
{
	dynarr_destroy (group->members);
	xph_free (group);
}

void entity_refresh (Entity e)
{
	entitySystem
		sys;
	int
		i = 0;

	if (EntitySystemRegistry == NULL)
		return;
	while ((sys = *(entitySystem *)dynarr_at (EntitySystemRegistry, i++)) != NULL)
	{
		if (entitySystem_entityRelevant (sys, e))
		{
			dynarr_push (sys->relevantEntities, e);
			dynarr_push (e->systems, sys);
		}
	}
}

/***
 * MESSAGING
 */

void entity_subscribe (Entity listener, Entity target)
{
	dynarr_push (target->listeners, listener);
}

void entity_unsubscribe (Entity listener, Entity target)
{
	dynarr_remove_condense (target->listeners, listener);
}

void entity_speak (const Entity speaker, char * message, void * arg)
{
	Entity
		listener;
	int
		i = 0;
	entity_message (speaker, speaker, message, arg);

	entitySystem_messageAll (speaker, message, arg);
	while ((listener = *(Entity *)dynarr_at (speaker->listeners, i++)) != NULL)
	{
		entity_message (listener, speaker, message, arg);
	}
}

bool entity_message (Entity e, Entity from, char * message, void * arg)
{
	EntComponent
		t;
	int
		i = 0;
	struct entity_speech
		speech;
	//printf ("%s (#%d, \"%s\", %p)\n", __FUNCTION__, entity_GUID (e), message, arg);
	if (e == NULL)
		return false;
	speech.from = from;
	if (from)
		speech.fromGUID = entity_GUID (from);
	else
		speech.fromGUID = 0;
	speech.message = message;
	speech.arg = arg;
	speech.references = 0;
	while ((t = *(EntComponent *)dynarr_at (e->components, i++)) != NULL)
	{
		speech.to = t;
		component_message (t, message, &speech);
	}
	return true;
}

static void component_messageSystem (const char * comp_name, const char * message, void * arg)
{
	componentSpec
		component = comp_getSpec (comp_name);
	struct messageTrigger
		* mt;
	int
		i = 0;
	if (component == NULL)
		return;
	mt = *(struct messageTrigger **)dynarr_search (component->messageTriggers, mt_search, message);
	if (mt == NULL)
		return;
	while (i < dynarr_size (mt->funcs))
	{
		(*(compFunc **)dynarr_at (mt->funcs, i++))(NULL, arg);
	}
}

static void component_message (EntComponent c, const char * message, void * arg)
{
	struct messageTrigger
		* mt = *(struct messageTrigger **)dynarr_search (c->spec->messageTriggers, mt_search, message);
	int
		i = 0;
	if (mt == NULL)
		return;
	while (i < dynarr_size (mt->funcs))
	{
		(*(compFunc **)dynarr_at (mt->funcs, i++))(c, arg);
	}
}

/***
 * COMPONENTS
 */

bool component_register (const char * comp_name, compFunc classInit)
{
	componentSpec
		spec;
	if (!classInit || !comp_name || !comp_name[0])
	{
		ERROR ("Can't create component; must have a component definition and name.");
		return false;
	}
	spec = xph_alloc (sizeof (struct xph_component_specification));

	strncpy (spec->comp_name, comp_name, ENT_NAMELENGTH - 1);
	
	spec->entities = dynarr_create (4, sizeof (EntComponent *));

	spec->loaderCallback = NULL;
	spec->weighCallback = NULL;

	spec->messageTriggers = dynarr_create (4, sizeof (struct messageTrigger *));
	if (CompSpecRegistry == NULL)
		CompSpecRegistry = dynarr_create (4, sizeof (componentSpec *));
	dynarr_push (CompSpecRegistry, spec);
	dynarr_sort (CompSpecRegistry, spec_sort);

	component_registerResponse (spec->comp_name, "__classInit", classInit);
	component_messageSystem (spec->comp_name, "__classInit", NULL);
	return true;
}

void component_destroy (const char * comp_name)
{
	componentSpec
		sys = comp_getSpec (comp_name);
	EntComponent
		c = NULL;
	//printf ("%s (\"%s\")...\n", __FUNCTION__, comp_name);
	if (sys == NULL)
		return;

	if (!dynarr_isEmpty (sys->entities))
	{
		WARNING ("EntComponent system \"%s\" still has %d entit%s with instantiations while being destroyed.", sys->comp_name, dynarr_size (sys->entities), dynarr_size (sys->entities) == 1 ? "y" : "ies");
		while (!dynarr_isEmpty (sys->entities)) {
			c = *(EntComponent *)dynarr_front (sys->entities);
			component_remove (sys->comp_name, c->e);
		}
	}

	component_messageSystem (sys->comp_name, "__classDestroy", NULL);

	dynarr_map (sys->messageTriggers, (void (*)(void *))mt_destroy);
	dynarr_destroy (sys->messageTriggers);

	dynarr_destroy (sys->entities);
	dynarr_remove_condense (CompSpecRegistry, sys);
	xph_free (sys);
}

bool component_instantiate (const char * comp_name, Entity e)
{
	componentSpec
		spec = comp_getSpec (comp_name);
	struct ent_component
		* instance = NULL;
	if (!spec)
		return false;
	instance = xph_alloc (sizeof (struct ent_component));
	instance->e = e;
	instance->spec = spec;
	instance->comp_guid = ++ComponentGUIDs;
	// we care less about enforcing uniqueness of component guids than we do about entities.
	dynarr_push (spec->entities, instance);
	dynarr_push (e->components, instance);
	dynarr_sort (spec->entities, comp_sort);
	dynarr_sort (e->components, comp_sort);

	instance->comp_data = NULL;
	component_message (instance, "__create", NULL);
	return true;
}

bool component_remove (const char * comp_name, Entity e)
{
	componentSpec
		sys = comp_getSpec (comp_name);
	EntComponent
		comp = NULL;

	if (sys == NULL)
		return false;
	//printf ("%s: removing component \"%s\" from entity #%d\n", __FUNCTION__, comp_name, entity_GUID (e));
	comp = *(EntComponent *)dynarr_search (sys->entities, comp_search, e);
	if (comp == NULL)
	{
		WARNING ("Entity #%d doesn't have component \"%s\"", e->guid, comp_name);
		return false;
	}
	component_message (comp, "__destroy", NULL);
	dynarr_remove_condense (sys->entities, comp);
	dynarr_remove_condense (e->components, comp);
	xph_free (comp);
	return true;
}

Entity component_entityAttached (EntComponent c)
{
	if (c == NULL)
		return NULL;
	return c->e;
}

bool component_setData (EntComponent c, void * data)
{
	if (c == NULL)
		return false;
	if (c->comp_data)
	{
		ERROR ("Could not set component data for \"%s\" on #%d: data already set to %p", c->spec->comp_name, entity_GUID (component_entityAttached (c)), c->comp_data);
		return false;
	}
	c->comp_data = data;
	return true;
}

void * component_getData (EntComponent c)
{
	if (c == NULL)
		return NULL;
	return c->comp_data;
}

bool component_clearData (EntComponent c)
{
	if (c == NULL)
		return false;
	c->comp_data = NULL;
	return true;
}


bool component_registerResponse (const char * comp_name, const char * message, compFunc * function)
{
	componentSpec
		sys = comp_getSpec (comp_name);
	struct messageTrigger
		* mt;
	if (sys == NULL)
	{
		ERROR ("No component \"%s\" registered", comp_name);
		return false;
	}
	mt = *(struct messageTrigger **)dynarr_search (sys->messageTriggers, mt_search, message);
	if (mt == NULL)
	{
		mt = mt_create (message);
		dynarr_push (sys->messageTriggers, mt);
		dynarr_sort (sys->messageTriggers, mt_sort);
	}
	// TODO: this allows for registering the same function multiple times. this should iterate over the set functions and only push if function isn't already set.
	dynarr_push (mt->funcs, function);
	return true;
}

bool component_clearResponses (const char * comp_name, const char * message)
{
	componentSpec
		sys = comp_getSpec (comp_name);
	struct messageTrigger
		* mt;
	if (sys == NULL)
		return false;
	// TODO: this is yet another place that would be served by the existance of dynarr_searchAndReturnIndex or w/e
	mt = *(struct messageTrigger **)dynarr_search (sys->messageTriggers, mt_search, message);
	if (mt == NULL)
		return true;
	dynarr_remove_condense (sys->messageTriggers, mt);
	mt_destroy (mt);
	return true;
}


static struct messageTrigger * mt_create (const char * message)
{
	struct messageTrigger
		* mt = xph_alloc (sizeof (struct messageTrigger));
	mt->message = xph_alloc (strlen (message) + 1);
	strcpy (mt->message, message);
	mt->funcs = dynarr_create (2, sizeof (compFunc *));
	return mt;
}

static void mt_destroy (struct messageTrigger * mt)
{
	xph_free (mt->message);
	/* don't wipe funcs; it's full of function pointers */
	dynarr_destroy (mt->funcs);
	xph_free (mt);
}

static int mt_sort (const void * a, const void * b)
{
	return strcmp
	(
		(*(const struct messageTrigger **)a)->message,
		(*(const struct messageTrigger **)b)->message
	);
}

static int mt_search (const void * k, const void * d)
{
	return strcmp
	(
		*(char **)k,
		(*(const struct messageTrigger **)d)->message
	);
}

/* returns a dynarr, which must be destroyed. In the event of a non-existant component or an intersection with no members, an empty dynarr is returned.
 */
Dynarr entity_getWithv (int n, va_list comps)
{
	int
		* indices = xph_alloc_name (sizeof (int) * n, "indices");
	Dynarr
		* components = xph_alloc_name (sizeof (Dynarr) * n, "vectors"),
		final = dynarr_create (2, sizeof (Entity *));
	const char
		** comp_names = xph_alloc_name (sizeof (char *) * n, "names");
	componentSpec
		sys = NULL;
	EntComponent
		c = NULL;
	int
		m = n,
		j = 0,
		highestGUID = 0;
	bool
		NULLbreak = false;
	m = 0;
	if (n == 0)
	{
		xph_free (comp_names);
		xph_free (components);
		xph_free (indices);
		return final;
	}
	while (m < n)
	{
		comp_names[m] = va_arg (comps, char *);
		sys = comp_getSpec (comp_names[m]);
		if (sys == NULL)
		{
			// it's impossible to have any intersection with an invalid component, since no entity will have it. Therefore, we can just return the empty dynarr.
			WARNING ("%s: intersection impossible; no such component \"%s\"", __FUNCTION__, comp_names[m]);
			xph_free (comp_names);
			xph_free (components);
			xph_free (indices);
			return final;
		}
		components[m] = sys->entities;
		indices[m] = 0;
		m++;
	}

	while (j < n)
	{
		c = *(EntComponent *)dynarr_front (components[j]);
		if (c == NULL)
		{
			// it's impossible to have any intersection, since there are no entities with this component. Therefore, we can just return the empty dynarr.
			DEBUG ("%s: intersection impossible; no entities have component \"%s\"", __FUNCTION__, comp_names[j]);
			xph_free (comp_names);
			xph_free (components);
			xph_free (indices);
			return final;
		}
		if (c->e->guid > highestGUID)
		{
			highestGUID = c->e->guid;
		}
		j++;
	}

  /* this whole thing is pretty horrific. Let me explain what I am attempting
   * to do here: since the system component lists are sorted by entity guid,
   * we can utilize that to do a relatively simple intersection merge. The
   * highest component in the first index across the lists is the lowest
   * possible entity which can have all components listed. So we iterate
   * across each list, incrementing the index until the guid is at least the
   * match of that 'highest guid'. Then we can check across-- if all the
   * values are the same, then we have a match. Otherwise, one of the other
   * values was higher, and it becomes the new target to reach. The current
   * code is really inefficient-- we don't really need two seperate passes to
   * determine the highestGUID value, and the NULLbreak variable is probably
   * completely unnecessary if we rewrite the outer loop to automatically
   * terminate when we hit a null value in one of the lists.
   */
	while (!NULLbreak)
	{
		j = 0;
		while (j < n)
		{
			while (
				(c = *(EntComponent *)dynarr_at (components[j], indices[j])) != NULL &&
				c->e->guid < highestGUID)
			{
				//printf ("guid in list %d at index %d is %d, which is lower than the mark of %d\n", j, indices[j], c->e->guid, highestGUID);
				indices[j]++;
			}
			if (c == NULL)
			{
				NULLbreak = true;
				break;
			}
			j++;
		}
		if (NULLbreak)
		{
			break;
		}
		j = 0;
		while (j < n)
		{
			c = *(EntComponent *)dynarr_at (components[j], indices[j]);
			if (c->e->guid > highestGUID)
			{
				highestGUID = c->e->guid;
				break;
			}
			j++;
		}
		if (j >= n)
		{
			dynarr_push (final, c->e);
			highestGUID++;
		}
	}
	xph_free (comp_names);
	xph_free (components);
	xph_free (indices);
	return final;
}

Dynarr entity_getWith (int n, ...)
{
	va_list
		components;
	Dynarr
		r;
	va_start (components, n);
	r = entity_getWithv (n, components);
	va_end (components);
	return r;
}

/***
 * ENTITY SYSTEM
 */

void entity_purgeDestroyed (TIMER timer)
{
	Entity
		e;
	EntSpeech
		speech;
	FUNCOPEN ();

	if (UnusedSpeech != NULL)
	{
		while ((speech = *(struct entity_speech **)dynarr_pop (UnusedSpeech)) != NULL)
		{
			entity_freeSpeech (speech);
			if (timer != NULL && outOfTime (timer))
				return;
		}
	}

	if (!ToBeDestroyed)
		return;

	while ((e = entity_get (*(unsigned int *)dynarr_pop (ToBeDestroyed))) != NULL)
	{
		entity_purge (e);
		if (timer != NULL && outOfTime (timer))
			return;
	}
	FUNCCLOSE ();
}

void entity_purge (Entity e)
{
	struct ent_component
		* c = NULL;
	entitySystem
		sys;
	int
		i = 0;
	EntSpeech
		speech = NULL;
	//printf ("destroying entity #%d; removing components:\n", e->guid);
	while (!dynarr_isEmpty (e->components))
	{
		//printf ("%d component%s.\n", dynarr_size (e->components), (dynarr_size (e->components) == 1 ? "" : "s"));
		c = *(struct ent_component **)dynarr_front (e->components);
		DEBUG ("Destroying #%d:\"%s\"", e->guid, c->spec->comp_name);
		component_remove (c->spec->comp_name, e);
	}
	dynarr_destroy (e->components);
	dynarr_destroy (e->listeners);
	while (!dynarr_isEmpty (e->systems))
	{
		sys = *(entitySystem *)dynarr_front (e->systems);
		i = 0;
		if (sys->messages != NULL)
		{
			while ((speech = *(struct entity_speech **)dynarr_at (sys->messages, i)) != NULL)
			{
				if (speech->from == e)
				{
					dynarr_unset (sys->messages, i);
					entity_freeSpeech (speech);
				}
				i++;
			}
			dynarr_condense (sys->messages);
		}
		entitySystem_removeEntity (sys, e);
	}
	if (e->name)
		entity_freeName (e->name);
	//printf ("adding #%d to the destroyed list, to be reused\n", e->guid);
	if (DestroyedEntities == NULL)
	{
		DestroyedEntities = dynarr_create (32, sizeof (unsigned int));
	}
	dynarr_remove_condense (ExistantEntities, e);
	dynarr_push (DestroyedEntities, e->guid);
	//printf ("done\n");
	xph_free (e);
}



static componentSpec comp_getSpec (const char * comp_name)
{
	componentSpec
		spec = NULL;
	if (CompSpecRegistry == NULL)
	{
		fprintf (stderr, "%s: no components registered\n", __FUNCTION__);
		return NULL;
	}
	spec = *(componentSpec *)dynarr_search (CompSpecRegistry, spec_search, comp_name);
	if (!spec)
		return NULL;
	return spec;
}

EntComponent entity_getAs (Entity e, const char * comp_name)
{
	componentSpec
		sys = comp_getSpec (comp_name);
	EntComponent
		comp = NULL;

	if (e == NULL || sys == NULL)
		return NULL;
	comp = *(EntComponent *)dynarr_search (sys->entities, comp_search, e);
	if (comp == NULL)
	{
		WARNING ("Entity #%d doesn't have component \"%s\"", entity_GUID (e), comp_name);
		return NULL;
	}
	return comp;
}

void entity_destroyEverything ()
{
	componentSpec
		sys = NULL;
	Entity
		e = NULL;
	DynIterator
		it;
	FUNCOPEN ();

	if (ExistantEntities != NULL)
	{
		it = dynIterator_create (ExistantEntities);
		while (!dynIterator_done (it))
		{
			e = *(Entity *)dynIterator_next (it);
			entity_destroy (e);
		}
		dynIterator_destroy (it);
		it = NULL;
	}
	entity_purgeDestroyed (NULL);
	if (ToBeDestroyed != NULL)
	{
		dynarr_destroy (ToBeDestroyed);
		ToBeDestroyed = NULL;
	}
	if (ExistantEntities != NULL)
	{
		dynarr_destroy (ExistantEntities);
		ExistantEntities = NULL;
	}
	if (DestroyedEntities != NULL)
	{
		dynarr_destroy (DestroyedEntities);
		DestroyedEntities = NULL;
	}

	if (EntitySystemRegistry != NULL)
	{
		dynarr_map (EntitySystemRegistry, (void (*)(void *))entitySystem_destroy);
		dynarr_destroy (EntitySystemRegistry);
		EntitySystemRegistry = NULL;
	}

	if (EntityNames)
	{
		dynarr_map (EntityNames, xph_free);
		dynarr_destroy (EntityNames);
		EntityNames = NULL;
	}

	if (EntityGroups)
	{
		dynarr_map (EntityGroups, (void(*)(void *))group_destroy);
		dynarr_destroy (EntityGroups);
		EntityGroups = NULL;
	}

	if (CompSpecRegistry == NULL)
		return;
	while (!dynarr_isEmpty (CompSpecRegistry))
	{
		sys = *(componentSpec *)dynarr_front (CompSpecRegistry);
		DEBUG ("Destroying system \"%s\"", sys->comp_name);
		component_destroy (sys->comp_name);
	}
	dynarr_destroy (CompSpecRegistry);
	CompSpecRegistry = NULL;
	FUNCCLOSE ();
}


/***
 * ENTITY SYSTEMS
 */

static void entity_freeSpeech (struct entity_speech * speech)
{
	DEBUG ("Freeing speech from #%d (\"%s\") (%p)", speech->fromGUID, speech->message, speech);
	xph_free (speech->message);
	xph_free (speech);
}

static entitySystem entitySystem_get (const char * sys_name)
{
	entitySystem
		sys = NULL;
	if (EntitySystemRegistry == NULL)
	{
		ERROR ("%s: no systems registered", __FUNCTION__);
		return NULL;
	}
	sys = *(entitySystem *)dynarr_search (EntitySystemRegistry, sys_search, sys_name);
	return sys;
}

void entitySystem_register (const char * name, sysFunc updateFunc, int components, ...)
{
	entitySystem
		sys = xph_alloc (sizeof (struct xph_entity_system));
	va_list
		args;
	const char
		* comp_name = NULL;
	char
		* stored = NULL;
	strncpy (sys->name, name, ENT_NAMELENGTH);
	sys->update = updateFunc;

	va_start (args, components);
	sys->relevantEntities = entity_getWithv (components, args);
	va_end (args);

	sys->relevantComponents = dynarr_create (components + 1, sizeof (char *));
	va_start (args, components);
	while (components > 0)
	{
		comp_name = va_arg (args, char *);
		stored = xph_alloc (strlen (comp_name) + 1);
		strcpy (stored, comp_name);
		dynarr_push (sys->relevantComponents, stored);
		components--;
	}
	va_end (args);

	sys->messages = dynarr_create (4, sizeof (EntSpeech));

	if (EntitySystemRegistry == NULL)
		EntitySystemRegistry = dynarr_create (2, sizeof (entitySystem));
	dynarr_push (EntitySystemRegistry, sys);
	dynarr_sort (EntitySystemRegistry, sys_sort);
	return;
}

static bool entitySystem_entityRelevant (const entitySystem system, const Entity entity)
{
	EntComponent
		comp;
	const char
		* comp_name;
	int
		i = 0,
		j = 0,
		match = 0,
		total = dynarr_size (system->relevantComponents);
	while ((comp_name = *(char **)dynarr_at (system->relevantComponents, i++)) != NULL)
	{
		j = 0;
		while ((comp = *(EntComponent *)dynarr_at (entity->components, j++)) != NULL)
		{
			if (strcmp (comp_name, comp->spec->comp_name) == 0)
			{
				match++;
				if (match == total)
					return true;
				break;
			}
		}
	}
	return false;
}

static void entitySystem_messageAll (Entity from, const char * message, void * arg)
{
	struct entity_speech
		* speech;
	entitySystem
		sys;
	int
		 i = 0;
	if (dynarr_size (from->systems) == 0)
		return;
	speech = xph_alloc (sizeof (struct entity_speech));
	speech->from = from;
	speech->fromGUID = entity_GUID (from);
	speech->arg = arg;
	speech->message = xph_alloc (strlen (message) + 1);
	strcpy (speech->message, message);
	speech->references = 0;
	while ((sys = *(entitySystem *)dynarr_at (from->systems, i++)) != NULL)
	{
		if (sys->messages == NULL)
			continue;
		DEBUG ("Queuing speech from #%d (\"%s\") on system %s (%p)", speech->fromGUID, speech->message, sys->name, speech);
		dynarr_push (sys->messages, speech);
		speech->references++;
	}
	if (speech->references == 0)
		entity_freeSpeech (speech);
}

void entitySystem_disableMessages (const char * system)
{
	entitySystem
		sys = entitySystem_get (system);
	if (!sys)
		return;
	if (dynarr_size (sys->messages))
	{
		WARNING ("Can't disable messages for system \"%s\"; there are already messages in the queue", system);
		return;
	}
	dynarr_destroy (sys->messages);
	sys->messages = NULL;
}

void entitySystem_message (const char * sys_name, Entity from, const char * message, void * arg)
{
	entitySystem
		sys = entitySystem_get (sys_name);
	EntSpeech
		speech;
	if (!sys)
		WARNING ("Tried to message system \"%s\" which does not exist.", sys_name);
	if (!sys || !sys->messages)
		return;
	speech = xph_alloc (sizeof (struct entity_speech));
	speech->from = from;
	speech->fromGUID = entity_GUID (from);
	speech->arg = arg;
	speech->message = xph_alloc (strlen (message) + 1);
	strcpy (speech->message, message);
	speech->references = 1;
	DEBUG ("Queuing speech from #%d (\"%s\") on system %s (%p)", speech->fromGUID, speech->message, sys->name, speech);
	dynarr_push (sys->messages, speech);
}

EntSpeech entitySystem_dequeueMessage (const char * system)
{
	FUNCOPEN ();
	entitySystem
		sys = entitySystem_get (system);
	EntSpeech
		speech;
	if (sys == NULL || sys->messages == NULL)
		return NULL;

	speech = *(EntSpeech *)dynarr_pop (sys->messages);
	if (speech)
		DEBUG ("Dequeueing speech from #%d (\"%s\") from system %s (%p)", speech->fromGUID, speech->message, system, speech);
	if (speech && --speech->references == 0)
	{
		if (!UnusedSpeech)
			UnusedSpeech = dynarr_create (4, sizeof (struct entity_speech *));
		dynarr_push (UnusedSpeech, speech);
	}
	FUNCCLOSE ();
	return speech;
}

void entitySystem_update (const char * name)
{
	entitySystem
		sys = entitySystem_get (name);
	if (!sys)
		return;
	sys->update (sys->relevantEntities);
}

static void entitySystem_removeEntity (entitySystem system, Entity entity)
{
	dynarr_remove_condense (system->relevantEntities, entity);
	dynarr_remove_condense (entity->systems, system);
}

static void entitySystem_destroy (entitySystem sys)
{
	dynarr_destroy (sys->relevantEntities);
	dynarr_map (sys->relevantComponents, xph_free);
	dynarr_destroy (sys->relevantComponents);
	// TODO: dynarr_map (sys->messages, {SOMETHING TO DECREMENT + DESTROY REFERENCE 0 MESSAGES}) ???
	if (sys->messages != NULL)
		dynarr_destroy (sys->messages);
	xph_free (sys);
}
