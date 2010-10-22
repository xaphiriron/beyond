#include "entity.h"

struct entity {
  unsigned int guid;

  // this is a local variable to make fetching components from a specific entity faster. It stores the same data as a System->entities vector, which is to say Components (something todo: this is named "components" whereas the system vector is named "entities". this is confusing and dumb.)
  Vector * components;
};

struct ent_system {
  Object * system;
  const char * comp_name;
  Vector * entities;
};

struct ent_component {
  Entity e;
  System reg;
  void * comp_data;
  unsigned int comp_guid;
};

static unsigned int EntityGUIDs = 0;
static Vector * DestroyedEntities = NULL;	// stores guids
static Vector * ExistantEntities = NULL;	// stores struct entity *

static unsigned int ComponentGUIDs = 0;
static Vector * SystemRegistry = NULL;

static Vector * SubsystemComponentStore = NULL;

static int comp_sort (const void * a, const void * b);
static int comp_search (const void * k, const void * d);
static int sys_sort (const void * a, const void * b);
static int sys_search (const void * k, const void * d);

static int comp_sort (const void * a, const void * b) {
  return
    (*(const struct ent_component **)a)->e->guid -
    (*(const struct ent_component **)b)->e->guid;
}

static int comp_search (const void * k, const void * d) {
  return
    ((const struct entity *)k)->guid -
    (*(const struct ent_component **)d)->e->guid;
}

static int sys_sort (const void * a, const void * b) {
  return
    strcmp (
      (*(const struct ent_system **)a)->comp_name,
      (*(const struct ent_system **)b)->comp_name
    );
}

static int sys_search (const void * k, const void * d) {
  return
    strcmp (
      k,
      (*(const struct ent_system **)d)->comp_name
    );
}



Entity entity_create () {
  new (struct entity, e);
  if (DestroyedEntities != NULL && vector_size (DestroyedEntities) > 0) {
    e->guid = vector_pop_back (e->guid, DestroyedEntities);
  } else {
    e->guid = ++EntityGUIDs;
  }
  if (e->guid == 0) {
    fprintf (stderr, "Entity GUIDs have wrapped around. Now anything is possible!\n");
    e->guid = ++EntityGUIDs;
  }
  e->components = vector_create (2, sizeof (Component));
  if (ExistantEntities == NULL) {
    ExistantEntities = vector_create (128, sizeof (Entity));
  }
  vector_push_back (ExistantEntities, e);
  return e;
}

void entity_destroy (Entity e) {
  struct ent_component * c = NULL;
  //printf ("destroying entity #%d; removing components:\n", e->guid);
  while (vector_size (e->components) > 0) {
    //printf ("%d component%s.\n", vector_size (e->components), (vector_size (e->components) == 1 ? "" : "s"));
    vector_front (c, e->components);
    component_removeFromEntity (c->reg->comp_name, e);
  }
  vector_destroy (e->components);
  //printf ("adding #%d to the destroyed list, to be reused\n", e->guid);
  if (DestroyedEntities == NULL) {
    DestroyedEntities = vector_create (32, sizeof (unsigned int));
  }
  vector_push_back (DestroyedEntities, e->guid);
  //printf ("done\n");
  xph_free (e);
}

bool entity_exists (unsigned int guid) {
  if (guid > EntityGUIDs || (DestroyedEntities != NULL && in_vector (DestroyedEntities, (void *)guid) >= 0)) {
    return FALSE;
  }
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
  reg->entities = vector_create (4, sizeof (Component *));
  if (SystemRegistry == NULL) {
    SystemRegistry = vector_create (4, sizeof (System *));
  }
  vector_push_back (SystemRegistry, reg);
  vector_sort (SystemRegistry, sys_sort);
  return TRUE;
}


/* returns a vector, which must be destroyed. In the event of a non-existant component or an intersection with no members, an empty vector is returned.
 */
Vector * entity_getEntitiesWithComponent (int n, ...) {
  int * indices = xph_alloc (sizeof (int) * n, "indices");
  Vector ** components = xph_alloc (sizeof (Vector *) * n, "vectors");
  const char ** comp_names = xph_alloc (sizeof (char *) * n, "names");
  Vector * final = vector_create (2, sizeof (Entity *));
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
    vector_front (c, components[j]);
    if (c == NULL) {
      // it's impossible to have any intersection, since there are no entities with this component. Therefore, we can just return the empty vector.
      printf ("%s: intersection impossible; no entities have component \"%s\"\n", __FUNCTION__, comp_names[j]);
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
        vector_at (c, components[j], indices[j]) != NULL &&
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
      vector_at (c, components[j], indices[j]);
      if (c->e->guid > highestGUID) {
        highestGUID = c->e->guid;
        break;
      }
      j++;
    }
    if (j >= n) {
      vector_push_back (final, c->e);
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
  sys = vector_search (SystemRegistry, comp_name, sys_search);
  if (sys == NULL) {
    fprintf (stderr, "%s: no component named \"%s\" registered\n", __FUNCTION__, comp_name);
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
  while (vector_size (sys->entities) > 0) {
    vector_front (c, sys->entities);
    component_removeFromEntity (sys->comp_name, c->e);
  }
  vector_remove (SystemRegistry, sys);
  vector_destroy (sys->entities);
  obj_message (sys->system, OM_DESTROY, NULL, NULL);
  objClass_destroy (sys->comp_name);
  xph_free (sys);
}

void entity_destroyEverything () {
  System sys = NULL;
  Entity e = NULL;
  while (vector_pop_back (e, ExistantEntities) != NULL) {
    entity_destroy (e);
  }
  vector_destroy (ExistantEntities);
  ExistantEntities = NULL;
  vector_destroy (DestroyedEntities);
  DestroyedEntities = NULL;
  vector_destroy (SubsystemComponentStore);
  SubsystemComponentStore = NULL;
  if (SystemRegistry == NULL) {
    return;
  }
  while (vector_size (SystemRegistry) > 0) {
    vector_front (sys, SystemRegistry);
    entity_destroySystem (sys->comp_name);
  }
  vector_destroy (SystemRegistry);
  SystemRegistry = NULL;
}

bool component_instantiateOnEntity (const char * comp_name, Entity e) {
  System sys = entity_getSystemByName (comp_name);
  if (sys == NULL) {
    return FALSE;
  }
  new (struct ent_component, instance);
  instance->e = e;
  instance->reg = sys;
  instance->comp_guid = ++ComponentGUIDs;
  // we care less about enforcing uniqueness of component guids than we do about entities.
  vector_push_back (sys->entities, instance);
  vector_push_back (e->components, instance);
  vector_sort (sys->entities, comp_sort);
  vector_sort (e->components, comp_sort);
  obj_message (sys->system, OM_COMPONENT_INIT_DATA, &instance->comp_data, e);
  return TRUE;
}

bool component_removeFromEntity (const char * comp_name, Entity e) {
  System sys = entity_getSystemByName (comp_name);
  Component comp = NULL;
  if (sys == NULL) {
    return FALSE;
  }
  comp = vector_search (sys->entities, e, comp_search);
  if (comp == NULL) {
    fprintf (stderr, "%s: Entity #%d doesn't have a component \"%s\"\n", __FUNCTION__, e->guid, comp_name);
    return FALSE;
  }
  obj_message (sys->system, OM_COMPONENT_DESTROY_DATA, &comp->comp_data, e);
  vector_remove (sys->entities, comp);
  vector_remove (e->components, comp);
  xph_free (comp);
  return TRUE;
}

Component entity_getAs (Entity e, const char * comp_name) {
  System sys = entity_getSystemByName (comp_name);
  Component comp = NULL;
  if (sys == NULL) {
    return NULL;
  }
  comp = vector_search (sys->entities, e, comp_search);
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

bool component_messageEntity (Component comp, char * message, void * arg) {
  new (struct comp_message, msg);
  Component t = NULL;
  int i = 0;
  msg->from = comp;
  msg->to = NULL;
  msg->message = message;
  while (vector_at (t, comp->e->components, i++) != NULL) {
    if (t == comp) {
      continue;
    }
    msg->to = t;
    obj_message (t->reg->system, OM_COMPONENT_RECEIVE_MESSAGE, msg, arg);
  }
  xph_free (msg);
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

void entitySubsystem_update (const char * comp_name) {
  System sys = entity_getSystemByName (comp_name);
  if (sys == NULL) {
    printf ("%s: got invalid component name (\"%s\")\n", __FUNCTION__, comp_name);
    return;
  }
  obj_message (sys->system, OM_UPDATE, NULL, NULL);
}

void entitySubsystem_postupdate (const char * comp_name) {
  System sys = entity_getSystemByName (comp_name);
  if (sys == NULL) {
    printf ("%s: got invalid component name (\"%s\")\n", __FUNCTION__, comp_name);
    return;
  }
  obj_message (sys->system, OM_POSTUPDATE, NULL, NULL);
}

bool entitySubsystem_store (const char * comp_name) {
  System s = NULL;
  if (SubsystemComponentStore == NULL) {
    SubsystemComponentStore = vector_create (6, sizeof (System));
  }
  s = entity_getSystemByName (comp_name);
  if (s == NULL) {
    return FALSE;
  }
  if (in_vector (SubsystemComponentStore, s) >= 0) {
    return TRUE;
  }
  vector_push_back (SubsystemComponentStore, s);
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
  if (in_vector (SubsystemComponentStore, s) < 0) {
    return TRUE; // already not in store
  }
  vector_remove (SubsystemComponentStore, s);
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
  while (vector_at (sys, SubsystemComponentStore, i++)) {
    t = obj_message (sys->system, msg, NULL, NULL);
    r = (r != TRUE) ? FALSE : t;
  }
  return r;
}

void entitySubsystem_clearStored () {
  if (SubsystemComponentStore != NULL) {
    vector_clear (SubsystemComponentStore);
  }
}
