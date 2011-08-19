#include "entity.h"

struct entity {
  unsigned int guid;

  // this is a local variable to make fetching components from a specific entity faster. It stores the same data as a EntSystem->entities vector, which is to say EntComponents (something todo: this is named "components" whereas the system vector is named "entities". this is confusing and dumb.)
  Dynarr components;

	Dynarr
		listeners;
};

struct messageTrigger
{
	char * message;
	Dynarr
		funcs;
};

struct ent_system {
  Object * system;
  const char * comp_name;
  Dynarr entities;

	void (* loaderCallback) (TIMER, EntComponent);
	unsigned char (* weighCallback) (EntComponent);

	Dynarr
		messageTriggers;
};


enum componentLoadStatus
{
	COMPONENT_NONEXISTANT = 0,
	COMPONENT_UNLOADED,
	COMPONENT_LOADING,
	COMPONENT_LOADED
};

typedef struct
{
	unsigned int
		totalValues,
		loadedValues;
	unsigned char
		loadWeight;
	enum componentLoadStatus
		status;
} COMPONENT_LOAD;

struct ent_component
{
  Entity e;
  EntSystem reg;
  void * comp_data;
	COMPONENT_LOAD
		* loader;
  unsigned int comp_guid;
	bool
		loaded;
};


static unsigned int EntityGUIDs = 0;
static unsigned int ComponentGUIDs = 0;

static Dynarr
	ToBeDestroyed = NULL,			// stores guids
	DestroyedEntities = NULL,		// stores guids
	ExistantEntities = NULL,		// stores struct entity *
	SystemRegistry = NULL,
	SubsystemComponentStore = NULL,
	ComponentLoader = NULL;

static bool ComponentLoaderUnsorted = FALSE;

static int guid_sort (const void * a, const void * b);
static int guid_search (const void * k, const void * d);
static int comp_sort (const void * a, const void * b);
static int comp_search (const void * k, const void * d);
static int sys_sort (const void * a, const void * b);
static int sys_search (const void * k, const void * d);

static int comp_weight_sort (const void * a, const void * b);

static struct messageTrigger * mt_create (const char * message);
static void mt_destroy (struct messageTrigger * mt);
static int mt_sort (const void * a, const void * b);
static int mt_search (const void * k, const void * d);


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

static int sys_sort (const void * a, const void * b) {
  //printf ("%s: got \"%s\" vs. \"%s\"\n", __FUNCTION__, (*(const struct ent_system **)a)->comp_name, (*(const struct ent_system **)b)->comp_name);
  return
    strcmp (
      (*(const struct ent_system **)a)->comp_name,
      (*(const struct ent_system **)b)->comp_name
    );
}

static int sys_search (const void * k, const void * d) {
  //printf ("%s: got \"%s\" vs. \"%s\"\n", __FUNCTION__, *(char **)k, (*(const struct ent_system **)d)->comp_name);
  return
    strcmp (
      *(char **)k,
      (*(const struct ent_system **)d)->comp_name
    );
}



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
		ERROR ("Entity GUIDs have wrapped around. Now anything is possible!", NULL);
		e->guid = ++EntityGUIDs;
	}
	e->components = dynarr_create (2, sizeof (EntComponent));
	e->listeners = dynarr_create (2, sizeof (Entity));
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
		return FALSE;
	}
	return TRUE;
}

Entity entity_get (unsigned int guid)
{
	Entity
		e = *(Entity *)dynarr_search (ExistantEntities, guid_search, guid);
	return e;
}

/***
 * MESSAGING
 */

static void component_sendMessage (const char * message, EntComponent c);

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
	while ((listener = *(Entity *)dynarr_at (speaker->listeners, i++)) != NULL)
	{
		entity_message (listener, speaker, message, arg);
	}
}

bool entity_message (Entity e, Entity from, char * message, void * arg)
{
	EntComponent
		t;
	struct comp_message
		msg;
	int
		i = 0;
	//printf ("%s (#%d, \"%s\", %p)\n", __FUNCTION__, entity_GUID (e), message, arg);
	if (e == NULL)
		return FALSE;
	//msg = xph_alloc (sizeof (struct comp_message));
	msg.entFrom = from;
	msg.from = NULL;
	msg.to = NULL;
	msg.message = message;
	while ((t = *(EntComponent *)dynarr_at (e->components, i++)) != NULL)
	{
		msg.to = t;
		/* this does two separate things to received messages (sending an
		 * object system message and checking the component message triggers)
		 * because the entity system is currently in a strange
		 * half-transitioned state, moving away from the object system.
		 * Eventually all the 'object' code will be removed and old components
		 * will be rewritten to use message triggers instead of a monolithic
		 * object system, but in the mean time both potential responses are
		 * messaged here
		 *  - xph 2011 08 19 */
		obj_message (t->reg->system, OM_COMPONENT_RECEIVE_MESSAGE, &msg, arg);
		component_sendMessage (message, t);
	}
	return TRUE;
}

static void component_sendMessage (const char * message, EntComponent c)
{
	struct messageTrigger
		* mt = *(struct messageTrigger **)dynarr_search (c->reg->messageTriggers, mt_search, message);
	int
		i = 0;
	if (mt == NULL)
		return;
	while (i < dynarr_size (mt->funcs))
	{
		(*(compFunc **)dynarr_at (mt->funcs, i++))(c);
	}
}

/***
 * COMPONENTS
 */

bool component_instantiate (const char * comp_name, Entity e)
{
	EntSystem
		sys = entity_getSystemByName (comp_name);
	struct ent_component
		* instance = NULL;
	if (sys == NULL)
		return FALSE;
	instance = xph_alloc (sizeof (struct ent_component));
	instance->e = e;
	instance->reg = sys;
	instance->comp_guid = ++ComponentGUIDs;
	instance->comp_data = NULL;
	instance->loaded = FALSE;
	instance->loader = NULL;
	// we care less about enforcing uniqueness of component guids than we do about entities.
	dynarr_push (sys->entities, instance);
	dynarr_push (e->components, instance);
	dynarr_sort (sys->entities, comp_sort);
	dynarr_sort (e->components, comp_sort);
	obj_message (sys->system, OM_COMPONENT_INIT_DATA, &instance->comp_data, e);
	return TRUE;
}

bool component_remove (const char * comp_name, Entity e)
{
	EntSystem
		sys = entity_getSystemByName (comp_name);
	EntComponent
		comp = NULL;

	if (sys == NULL)
		return FALSE;
	//printf ("%s: removing component \"%s\" from entity #%d\n", __FUNCTION__, comp_name, entity_GUID (e));
	comp = *(EntComponent *)dynarr_search (sys->entities, comp_search, e);
	if (comp == NULL)
	{
		WARNING ("Entity #%d doesn't have component \"%s\"", e->guid, comp_name);
		return FALSE;
	}
	obj_message (sys->system, OM_COMPONENT_DESTROY_DATA, &comp->comp_data, e);
	dynarr_remove_condense (sys->entities, comp);
	dynarr_remove_condense (e->components, comp);
	if (comp->loader != NULL)
		xph_free (comp->loader);
	xph_free (comp);
	return TRUE;
}

Entity component_entityAttached (EntComponent c)
{
	if (c == NULL)
		return NULL;
	return c->e;
}

void * component_getData (EntComponent c)
{
	if (c == NULL)
		return NULL;
	return c->comp_data;
}


bool component_registerResponse (const char * comp_name, const char * message, compFunc * function)
{
	EntSystem
		sys = entity_getSystemByName (comp_name);
	struct messageTrigger
		* mt;
	if (sys == NULL)
		return FALSE;
	mt = *(struct messageTrigger **)dynarr_search (sys->messageTriggers, mt_search, message);
	if (mt == NULL)
	{
		mt = mt_create (message);
		dynarr_push (sys->messageTriggers, mt);
		dynarr_sort (sys->messageTriggers, mt_sort);
	}
	// TODO: this allows for registering the same function multiple times. this should iterate over the set functions and only push if function isn't already set.
	dynarr_push (mt->funcs, function);
	return TRUE;
}

bool component_clearResponses (const char * comp_name, const char * message)
{
	EntSystem
		sys = entity_getSystemByName (comp_name);
	struct messageTrigger
		* mt;
	if (sys == NULL)
		return FALSE;
	// TODO: this is yet another place that would be served by the existance of dynarr_searchAndReturnIndex or w/e
	mt = *(struct messageTrigger **)dynarr_search (sys->messageTriggers, mt_search, message);
	if (mt == NULL)
		return TRUE;
	dynarr_remove_condense (sys->messageTriggers, mt);
	mt_destroy (mt);
	return TRUE;
}







void entity_purgeDestroyed (TIMER t)
{
	struct ent_component
		* c = NULL;
	Entity
		e;
	FUNCOPEN ();
	if (ToBeDestroyed == NULL)
	{
		FUNCCLOSE ();
		return;
	}
	while ((e = entity_get (*(unsigned int *)dynarr_pop (ToBeDestroyed))) != NULL)
	{
		DEBUG ("Destroying entity #%d", entity_GUID (e));
		//printf ("destroying entity #%d; removing components:\n", e->guid);
		while (!dynarr_isEmpty (e->components))
		{
			//printf ("%d component%s.\n", dynarr_size (e->components), (dynarr_size (e->components) == 1 ? "" : "s"));
			c = *(struct ent_component **)dynarr_front (e->components);
			DEBUG ("Destroying #%d:\"%s\"", e->guid, c->reg->comp_name);
			component_remove (c->reg->comp_name, e);
		}
		dynarr_destroy (e->components);
		dynarr_destroy (e->listeners);
		//printf ("adding #%d to the destroyed list, to be reused\n", e->guid);
		if (DestroyedEntities == NULL)
		{
			DestroyedEntities = dynarr_create (32, sizeof (unsigned int));
		}
		// TODO: this function requires two passes through the array even though since ExistantEntities is kept sorted it ought to be possible to search for the entity by GUID, unset it, and then condense from that index.
		dynarr_remove_condense (ExistantEntities, e);
		dynarr_push (DestroyedEntities, e->guid);
		//printf ("done\n");
		xph_free (e);
		if (t != NULL && outOfTime (t))
			return;
	}
	//printf ("...%s ()\n", __FUNCTION__);
	FUNCCLOSE ();
}


bool entity_registerComponentAndSystem (objHandler func) {
	struct ent_system
		* reg = xph_alloc (sizeof (struct ent_system));
  ObjClass * oc = objClass_init (func, NULL, NULL, NULL);
  Object * sys = obj_create (oc->name, NULL, NULL, NULL);
  reg->system = sys;
  reg->comp_name = obj_getClassName (sys);
  reg->entities = dynarr_create (4, sizeof (EntComponent *));
	reg->loaderCallback = NULL;
	reg->weighCallback = NULL;
	obj_message (sys, OM_COMPONENT_GET_LOADER_CALLBACK, &reg->loaderCallback, NULL);
	obj_message (sys, OM_COMPONENT_GET_WEIGH_CALLBACK, &reg->weighCallback, NULL);
	reg->messageTriggers = dynarr_create (4, sizeof (struct messageTrigger));
  if (SystemRegistry == NULL) {
    SystemRegistry = dynarr_create (4, sizeof (EntSystem *));
  }
  dynarr_push (SystemRegistry, reg);
  dynarr_sort (SystemRegistry, sys_sort);
	//printf ("%s: registered component \"%s\"\n", __FUNCTION__, reg->comp_name);
  return TRUE;
}


/* returns a vector, which must be destroyed. In the event of a non-existant component or an intersection with no members, an empty vector is returned.
 */
Dynarr entity_getEntitiesWithComponent (int n, ...) {
  int * indices = xph_alloc_name (sizeof (int) * n, "indices");
  Dynarr* components = xph_alloc_name (sizeof (Dynarr) * n, "vectors");
  const char ** comp_names = xph_alloc_name (sizeof (char *) * n, "names");
  Dynarr final = dynarr_create (2, sizeof (Entity *));
  EntSystem sys = NULL;
  EntComponent c = NULL;
  int
    m = n,
    j = 0,
    highestGUID = 0;
  bool NULLbreak = FALSE;
  va_list comps;
  va_start (comps, n);
  m = 0;
  while (m < n) {
    comp_names[m] = va_arg (comps, char *);
    sys = entity_getSystemByName (comp_names[m]);
    if (sys == NULL) {
      // it's impossible to have any intersection with an invalid component, since no entity will have it. Therefore, we can just return the empty vector.
      printf ("%s: intersection impossible; no such component \"%s\"", __FUNCTION__, comp_names[m]);
      xph_free (comp_names);
      xph_free (components);
      xph_free (indices);
      return final;
    }
    components[m] = sys->entities;
    indices[m] = 0;
    m++;
  }

  while (j < n) {
    c = *(EntComponent *)dynarr_front (components[j]);
    if (c == NULL) {
      // it's impossible to have any intersection, since there are no entities with this component. Therefore, we can just return the empty vector.
      //printf ("%s: intersection impossible; no entities have component \"%s\"\n", __FUNCTION__, comp_names[j]);
      xph_free (comp_names);
      xph_free (components);
      xph_free (indices);
      return final;
    }
    if (c->e->guid > highestGUID) {
      highestGUID = c->e->guid;
    }
    j++;
  }
  j = 0;


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
  while (!NULLbreak) {
    j = 0;
    while (j < n) {
      while (
        (c = *(EntComponent *)dynarr_at (components[j], indices[j])) != NULL &&
        c->e->guid < highestGUID) {
        //printf ("guid in list %d at index %d is %d, which is lower than the mark of %d\n", j, indices[j], c->e->guid, highestGUID);
        indices[j]++;
      }
      if (c == NULL) {
        NULLbreak = TRUE;
        break;
      }
      j++;
    }
    if (NULLbreak) {
      break;
    }
    j = 0;
    while (j < n) {
      c = *(EntComponent *)dynarr_at (components[j], indices[j]);
      if (c->e->guid > highestGUID) {
        highestGUID = c->e->guid;
        break;
      }
      j++;
    }
    if (j >= n) {
      dynarr_push (final, c->e);
      highestGUID++;
    }
  }

  xph_free (comp_names);
  xph_free (components);
  xph_free (indices);
  return final;
}

EntSystem entity_getSystemByName (const char * comp_name) {
  struct ent_system * sys = NULL;
  if (SystemRegistry == NULL) {
    fprintf (stderr, "%s: no components registered\n", __FUNCTION__);
    return NULL;
  }
  sys = *(struct ent_system **)dynarr_search (SystemRegistry, sys_search, comp_name);
  if (sys == NULL) {
    //fprintf (stderr, "%s: no component named \"%s\" registered\n", __FUNCTION__, comp_name);
    return NULL;
  }
  return sys;
}

void entity_destroySystem (const char * comp_name) {
  EntSystem sys = entity_getSystemByName (comp_name);
  EntComponent c = NULL;
	//printf ("%s (\"%s\")...\n", __FUNCTION__, comp_name);
  if (sys == NULL) {
    return;
  }
	if (!dynarr_isEmpty (sys->entities))
	{
		WARNING ("EntComponent system \"%s\" still has %d entit%s with instantiations while being destroyed.", sys->comp_name, dynarr_size (sys->entities), dynarr_size (sys->entities) == 1 ? "y" : "ies");
		while (!dynarr_isEmpty (sys->entities)) {
			c = *(EntComponent *)dynarr_front (sys->entities);
			component_remove (sys->comp_name, c->e);
		}
	}
	dynarr_wipe (sys->messageTriggers, (void (*)(void *))mt_destroy);
	dynarr_destroy (sys->messageTriggers);
  dynarr_destroy (sys->entities);
  dynarr_remove_condense (SystemRegistry, sys);
  obj_message (sys->system, OM_DESTROY, NULL, NULL);
  objClass_destroy (sys->comp_name);
  xph_free (sys);
	//printf ("...%s\n", __FUNCTION__);
}


EntComponent entity_getAs (Entity e, const char * comp_name)
{
	EntSystem
		sys = entity_getSystemByName (comp_name);
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
	EntSystem
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
	dynarr_destroy (SubsystemComponentStore);
	SubsystemComponentStore = NULL;
	if (SystemRegistry == NULL)
		return;
	while (!dynarr_isEmpty (SystemRegistry))
	{
		sys = *(EntSystem *)dynarr_front (SystemRegistry);
		DEBUG ("Destroying system \"%s\"", sys->comp_name);
		entity_destroySystem (sys->comp_name);
	}
	dynarr_destroy (SystemRegistry);
	SystemRegistry = NULL;
	FUNCCLOSE ();
}


bool entitySubsystem_store (const char * comp_name) {
  EntSystem s = NULL;
  if (SubsystemComponentStore == NULL) {
    SubsystemComponentStore = dynarr_create (6, sizeof (EntSystem));
  }
  s = entity_getSystemByName (comp_name);
  if (s == NULL) {
    return FALSE;
  }
  if (in_dynarr (SubsystemComponentStore, s) >= 0) {
    return TRUE;
  }
  dynarr_push (SubsystemComponentStore, s);
  return TRUE;
}

bool entitySubsystem_unstore (const char * comp_name) {
  EntSystem s = NULL;
  if (SubsystemComponentStore == NULL) {
    return FALSE;
  }
  s = entity_getSystemByName (comp_name);
  if (s == NULL) {
    return FALSE;
  }
  if (in_dynarr (SubsystemComponentStore, s) < 0) {
    return TRUE; // already not in store
  }
  dynarr_remove_condense (SubsystemComponentStore, s);
  return TRUE;
}

bool entitySubsystem_runOnStored (objMsg msg) {
  bool
    r = TRUE,
    t = TRUE;
  EntSystem sys = NULL;
  int i = 0;
  if (SubsystemComponentStore == NULL) {
    return FALSE;
  }
  while ((sys = *(EntSystem *)dynarr_at (SubsystemComponentStore, i++)) != NULL) {
    t = obj_message (sys->system, msg, NULL, NULL);
    r = (r != TRUE) ? FALSE : t;
  }
  return r;
}

void entitySubsystem_clearStored () {
  if (SubsystemComponentStore != NULL) {
    dynarr_clear (SubsystemComponentStore);
  }
}



/***
 * LOADING (old and terrible)
 */

void component_setLoadGoal (EntComponent c, unsigned int m)
{
	if (c == NULL || c->loader == NULL)
		return;
	c->loader->status = COMPONENT_LOADING;
	c->loader->totalValues = m;
}

void component_updateLoadAmount (EntComponent c, unsigned int v)
{
	if (c == NULL || c->loader == NULL)
		return;
	c->loader->status = COMPONENT_LOADING;
	c->loader->loadedValues = (v > c->loader->totalValues)
		? c->loader->totalValues
		: v;
}

void component_setLoadComplete (EntComponent c)
{
	if (c == NULL || c->loader == NULL)
		return;
	c->loader->status = COMPONENT_LOADED;
	c->loaded = TRUE;
	//printf ("---component %p fully loaded\n", c);
	dynarr_remove_condense (ComponentLoader, c);
}

bool component_isFullyLoaded (const EntComponent c)
{
	assert (c != NULL);
	return c->loaded;
}

void component_dropLoad (EntComponent c)
{
	if (ComponentLoader == NULL || c == NULL)
		return;
	dynarr_remove_condense (ComponentLoader, c);
	if (c->loader != NULL)
		xph_free (c->loader);
	c->loader = NULL;
	c->loaded = FALSE;
}



void component_reweigh (EntComponent c)
{
	if (c->loader == NULL)
		return;
	if (c->reg->weighCallback != NULL)
		c->loader->loadWeight = c->reg->weighCallback (c);
	else
		c->loader->loadWeight = 32;
}

void component_forceLoaderResort ()
{
	dynarr_sort (ComponentLoader, comp_weight_sort);
	ComponentLoaderUnsorted = FALSE;
}




void component_setAsLoadable (EntComponent c)
{
	COMPONENT_LOAD
		* l = c->loader;
	//printf ("%s (%p)...\n", __FUNCTION__, c);
	if (l != NULL)
	{
		fprintf (stderr, "%s (%p): Dual loading??? HOW CAN THIS BE?\not really sure what to do here...\n", __FUNCTION__, c);
		return;
	}
	l = xph_alloc (sizeof (COMPONENT_LOAD));
	c->loader = l;
	c->loaded = FALSE;
	l->status = COMPONENT_UNLOADED;
	component_reweigh (c);
	if (ComponentLoader == NULL)
		ComponentLoader = dynarr_create (8, sizeof (EntComponent));
	dynarr_push (ComponentLoader, c);
	ComponentLoaderUnsorted = TRUE;
	//printf ("...%s\n", __FUNCTION__);
}

bool component_isLoaderActive ()
{
	if (ComponentLoader == NULL)
		return FALSE;
	if (dynarr_isEmpty (ComponentLoader))
		return FALSE;
	return TRUE;
}

void component_forceRunLoader (unsigned int load)
{
	EntComponent
		c;
	unsigned int
		loaded = 0;
	printf ("%s...\n", __FUNCTION__);
	if (ComponentLoader == NULL)
		ComponentLoader = dynarr_create (8, sizeof (EntComponent));
	if (ComponentLoaderUnsorted == TRUE)
	{
		component_forceLoaderResort ();
	}
	while (component_isLoaderActive () && (loaded < load || load == 0))
	{
		c = *(EntComponent *)dynarr_front (ComponentLoader);
		printf ("%s: got %p\n", __FUNCTION__, c);
		dynarr_remove_condense (ComponentLoader, c);
		if (c->reg->loaderCallback == NULL)
		{
			continue;
		}
		// TODO: this makes a mockery of the priority queue.
		dynarr_push (ComponentLoader, c);
		c->reg->loaderCallback (NULL, c);
	}
	printf ("...%s\n", __FUNCTION__);
}

void component_runLoader (const TIMER t)
{
	EntComponent
		c;
	float
		timeElapsed;
	FUNCOPEN ();
	if (ComponentLoader == NULL)
		ComponentLoader = dynarr_create (8, sizeof (EntComponent));
	if (ComponentLoaderUnsorted == TRUE)
	{
		//printf ("%s: resorting components by weight\n", __FUNCTION__);
		dynarr_sort (ComponentLoader, comp_weight_sort);
		ComponentLoaderUnsorted = FALSE;
	}
	while (component_isLoaderActive () && (timeElapsed = timerGetTimeSinceLastUpdate (t)) < 0.05)
	{
		c = *(EntComponent *)dynarr_back (ComponentLoader);
		//printf ("%s: got front component with weight %d\n", __FUNCTION__, c->loader->loadWeight);
		if (c->reg->loaderCallback == NULL)
		{
			dynarr_pop (ComponentLoader);
			continue;
		}
		c->reg->loaderCallback (t, c);
	}
	FUNCCLOSE ();
}

static int comp_weight_sort (const void * a, const void * b)
{
	return (*(EntComponent *)a)->loader->loadWeight - (*(EntComponent *)b)->loader->loadWeight;
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
	dynarr_wipe (mt->funcs, xph_free);
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

