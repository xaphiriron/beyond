#include "entity.h"

struct entity {
  unsigned int guid;

  // this is a local variable to make fetching components from a specific entity faster. It stores the same data as a System->entities vector, which is to say Components (something todo: this is named "components" whereas the system vector is named "entities". this is confusing and dumb.)
  Dynarr components;
};

struct ent_system {
  Object * system;
  const char * comp_name;
  Dynarr entities;
	void (* loaderCallback) (Component);
	unsigned char (* weighCallback) (Component);
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
  System reg;
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
	DestroyedEntities = NULL,		// stores guids
	ExistantEntities = NULL,		// stores struct entity *
	SystemRegistry = NULL,
	SubsystemComponentStore = NULL,
	ComponentLoader = NULL;

static bool ComponentLoaderUnsorted = FALSE;


static int comp_sort (const void * a, const void * b);
static int comp_search (const void * k, const void * d);
static int sys_sort (const void * a, const void * b);
static int sys_search (const void * k, const void * d);

static int comp_weight_sort (const void * a, const void * b);

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



Entity entity_create () {
  struct entity * e = xph_alloc (sizeof (struct entity));
  if (DestroyedEntities != NULL && !dynarr_isEmpty (DestroyedEntities)) {
    e->guid = *(unsigned int *)dynarr_pop (DestroyedEntities);
  } else {
    e->guid = ++EntityGUIDs;
  }
  if (e->guid == 0) {
    fprintf (stderr, "Entity GUIDs have wrapped around. Now anything is possible!\n");
    e->guid = ++EntityGUIDs;
  }
  e->components = dynarr_create (2, sizeof (Component));
  if (ExistantEntities == NULL) {
    ExistantEntities = dynarr_create (128, sizeof (Entity));
  }
  dynarr_push (ExistantEntities, e);
  return e;
}

void entity_destroy (Entity e) {
  struct ent_component * c = NULL;
  //printf ("destroying entity #%d; removing components:\n", e->guid);
  while (!dynarr_isEmpty (e->components)) {
    //printf ("%d component%s.\n", vector_size (e->components), (vector_size (e->components) == 1 ? "" : "s"));
    c = *(struct ent_component **)dynarr_front (e->components);
    component_removeFromEntity (c->reg->comp_name, e);
  }
  dynarr_destroy (e->components);
  //printf ("adding #%d to the destroyed list, to be reused\n", e->guid);
  if (DestroyedEntities == NULL) {
    DestroyedEntities = dynarr_create (32, sizeof (unsigned int));
  }
  dynarr_push (DestroyedEntities, e->guid);
  //printf ("done\n");
  xph_free (e);
}

bool entity_exists (unsigned int guid) {
  if (guid > EntityGUIDs || (DestroyedEntities != NULL && in_dynarr (DestroyedEntities, (void *)guid) >= 0)) {
    return FALSE;
  }
  return TRUE;
}

bool entity_message (Entity e, char * message, void * arg)
{
	Component
		t;
	struct comp_message
		msg;
	DynIterator
		it;
	//printf ("%s (#%d, \"%s\", %p)\n", __FUNCTION__, entity_GUID (e), message, arg);
	if (e == NULL)
		return FALSE;
	//msg = xph_alloc (sizeof (struct comp_message));
	msg.from = NULL;
	msg.to = NULL;
	msg.message = message;
	it = dynIterator_create (e->components);
	while (!dynIterator_done (it))
	{
		t = *(Component *)dynIterator_next (it);
		msg.to = t;
		obj_message (t->reg->system, OM_COMPONENT_RECEIVE_MESSAGE, &msg, arg);
	}
	dynIterator_destroy (it);
	return TRUE;
}

unsigned int entity_GUID (const Entity e) {
  return e->guid;
}


bool entity_registerComponentAndSystem (objHandler func) {
  new (struct ent_system, reg);
  ObjClass * oc = objClass_init (func, NULL, NULL, NULL);
  Object * sys = obj_create (oc->name, NULL, NULL, NULL);
  reg->system = sys;
  reg->comp_name = obj_getClassName (sys);
  reg->entities = dynarr_create (4, sizeof (Component *));
	reg->loaderCallback = NULL;
	reg->weighCallback = NULL;
	obj_message (sys, OM_COMPONENT_GET_LOADER_CALLBACK, &reg->loaderCallback, NULL);
	obj_message (sys, OM_COMPONENT_GET_WEIGH_CALLBACK, &reg->weighCallback, NULL);
  if (SystemRegistry == NULL) {
    SystemRegistry = dynarr_create (4, sizeof (System *));
  }
  dynarr_push (SystemRegistry, reg);
  dynarr_sort (SystemRegistry, sys_sort);
	printf ("%s: registered component \"%s\"\n", __FUNCTION__, reg->comp_name);
  return TRUE;
}


/* returns a vector, which must be destroyed. In the event of a non-existant component or an intersection with no members, an empty vector is returned.
 */
Dynarr entity_getEntitiesWithComponent (int n, ...) {
  int * indices = xph_alloc_name (sizeof (int) * n, "indices");
  Dynarr* components = xph_alloc_name (sizeof (Dynarr) * n, "vectors");
  const char ** comp_names = xph_alloc_name (sizeof (char *) * n, "names");
  Dynarr final = dynarr_create (2, sizeof (Entity *));
  System sys = NULL;
  Component c = NULL;
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
    c = *(Component *)dynarr_front (components[j]);
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
        (c = *(Component *)dynarr_at (components[j], indices[j])) != NULL &&
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
      c = *(Component *)dynarr_at (components[j], indices[j]);
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

System entity_getSystemByName (const char * comp_name) {
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
  System sys = entity_getSystemByName (comp_name);
  Component c = NULL;
  if (sys == NULL) {
    return;
  }
  while (!dynarr_isEmpty (sys->entities)) {
    c = *(Component *)dynarr_front (sys->entities);
    component_removeFromEntity (sys->comp_name, c->e);
  }
  dynarr_destroy (sys->entities);
  dynarr_remove_condense (SystemRegistry, sys);
  obj_message (sys->system, OM_DESTROY, NULL, NULL);
  objClass_destroy (sys->comp_name);
  xph_free (sys);
}

void entity_destroyEverything () {
  System sys = NULL;
  Entity e = NULL;
  while ((e = *(Entity *)dynarr_pop (ExistantEntities)) != NULL) {
    entity_destroy (e);
  }
  dynarr_destroy (ExistantEntities);
  ExistantEntities = NULL;
  dynarr_destroy (DestroyedEntities);
  DestroyedEntities = NULL;
  dynarr_destroy (SubsystemComponentStore);
  SubsystemComponentStore = NULL;
  if (SystemRegistry == NULL) {
    return;
  }
  while (!dynarr_isEmpty (SystemRegistry)) {
    sys = *(System *)dynarr_front (SystemRegistry);
    entity_destroySystem (sys->comp_name);
  }
  dynarr_destroy (SystemRegistry);
  SystemRegistry = NULL;
}

bool component_instantiateOnEntity (const char * comp_name, Entity e) {
  System sys = entity_getSystemByName (comp_name);
  if (sys == NULL) {
    return FALSE;
  }
	struct ent_component * instance = xph_alloc (sizeof (struct ent_component));
  instance->e = e;
  instance->reg = sys;
  instance->comp_guid = ++ComponentGUIDs;
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

bool component_removeFromEntity (const char * comp_name, Entity e) {
  System sys = entity_getSystemByName (comp_name);
  Component comp = NULL;
  if (sys == NULL) {
    return FALSE;
  }
  comp = *(Component *)dynarr_search (sys->entities, comp_search, e);
  if (comp == NULL) {
    fprintf (stderr, "%s: Entity #%d doesn't have a component \"%s\"\n", __FUNCTION__, e->guid, comp_name);
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

Component entity_getAs (Entity e, const char * comp_name) {
  System sys = entity_getSystemByName (comp_name);
  Component comp = NULL;
  if (e == NULL || sys == NULL) {
    return NULL;
  }
  comp = *(Component *)dynarr_search (sys->entities, comp_search, e);
  if (comp == NULL) {
    fprintf (stderr, "%s: Entity #%d doesn't have a component \"%s\"\n", __FUNCTION__, e->guid, comp_name);
    return NULL;
  }
  return comp;
}

void * component_getData (Component c) {
  if (c == NULL) {
    return NULL;
  }
  return c->comp_data;
}

Entity component_entityAttached (Component c) {
  if (c == NULL) {
    return NULL;
  }
  return c->e;
}

bool component_messageEntity (Component comp, char * message, void * arg)
{
	struct comp_message
		msg;
	Component
		t = NULL;
	int
		i = 0;
	msg.from = comp;
	msg.to = NULL;
	msg.message = message;
	while ((t = *(Component *)dynarr_at (comp->e->components, i++)) != NULL)
	{
		// we're purposefully messaging the component back, in case it has a response to its own message. consequentally, it's important that no components decide to send off the same message in response to getting a message.
		msg.to = t;
		//printf ("%s: #%d: messaging component \"%s\"\n", __FUNCTION__, comp->e->guid, t->reg->comp_name);
		obj_message (t->reg->system, OM_COMPONENT_RECEIVE_MESSAGE, &msg, arg);
	}
	return TRUE;
}

bool component_messageSystem (Component comp, char * message, void * arg) {
  new (struct comp_message, msg);
  msg->from = comp;
  msg->message = message;
  obj_message (comp->reg->system, OM_SYSTEM_RECEIVE_MESSAGE, msg, message);
  xph_free (msg);
  return TRUE;
}

bool entitySubsystem_message (const char * comp_name, enum object_messages message, void * a, void * b)
{
	System
		s = entity_getSystemByName (comp_name);
	if (s == NULL)
		return FALSE;
	obj_message (s->system, message, a, b);
	return TRUE;
}

bool entitySubsystem_store (const char * comp_name) {
  System s = NULL;
  if (SubsystemComponentStore == NULL) {
    SubsystemComponentStore = dynarr_create (6, sizeof (System));
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
  System s = NULL;
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
  System sys = NULL;
  int i = 0;
  if (SubsystemComponentStore == NULL) {
    return FALSE;
  }
  while ((sys = *(System *)dynarr_at (SubsystemComponentStore, i++)) != NULL) {
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





void component_setLoadTarget (Component c, unsigned int m)
{
	if (c == NULL || c->loader == NULL)
		return;
	c->loader->status = COMPONENT_LOADING;
	c->loader->totalValues = m;
}

void component_updateLoadData (Component c, unsigned int v)
{
	if (c == NULL || c->loader == NULL)
		return;
	c->loader->status = COMPONENT_LOADING;
	c->loader->loadedValues = (v > c->loader->totalValues)
		? c->loader->totalValues
		: v;
}

void component_setLoadComplete (Component c)
{
	if (c == NULL || c->loader == NULL)
		return;
	c->loader->status = COMPONENT_LOADED;
	dynarr_remove_condense (ComponentLoader, c);
}



void component_setAsLoadable (Component c)
{
	COMPONENT_LOAD
		* l = c->loader;
	if (l != NULL)
	{
		fprintf (stderr, "%s (%p): Dual loading??? HOW CAN THIS BE?\not really sure what to do here...\n", __FUNCTION__, c);
		return;
	}
	l = xph_alloc (sizeof (COMPONENT_LOAD));
	c->loader = l;
	l->status = COMPONENT_UNLOADED;
	if (c->reg->weighCallback != NULL)
		c->loader->loadWeight = c->reg->weighCallback (c);
	else
		c->loader->loadWeight = 32;
	dynarr_push (ComponentLoader, c);
	ComponentLoaderUnsorted = TRUE;
}

bool component_isLoaderActive ()
{
	if (ComponentLoader == NULL)
		return FALSE;
	if (dynarr_isEmpty (ComponentLoader))
		return FALSE;
	return TRUE;
}

void component_runLoader (const TIMER * t)
{
	Component
		c;
	float
		timeElapsed;
	if (ComponentLoader == NULL)
		ComponentLoader = dynarr_create (8, sizeof (Component));
	if (ComponentLoaderUnsorted == TRUE)
	{
		dynarr_sort (ComponentLoader, comp_weight_sort);
		ComponentLoaderUnsorted = FALSE;
	}
	while (component_isLoaderActive () && (timeElapsed = xtimer_timeSinceLastUpdate (t)) < 0.1)
	{
		c = *(Component *)dynarr_front (ComponentLoader);
		if (c->reg->loaderCallback == NULL)
		{
			dynarr_remove_condense (ComponentLoader, c);
			continue;
		}
		c->reg->loaderCallback (c);
	}
}

static int comp_weight_sort (const void * a, const void * b)
{
	return (*(Component *)a)->loader->loadWeight - (*(Component *)b)->loader->loadWeight;
}
