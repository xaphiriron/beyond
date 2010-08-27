#include "entity.h"

static unsigned int EntityGUIDs = 0;
static Vector * DestroyedEntities = NULL;

static unsigned int ComponentGUIDs = 0;
static Vector * SystemRegistry = NULL;

static int comp_sort (const void * a, const void * b);
static int comp_search (const void * k, const void * d);
static int sys_sort (const void * a, const void * b);
static int sys_search (const void * k, const void * d);

static int comp_sort (const void * a, const void * b) {
  return
    (*(const Component **)a)->e->guid -
    (*(const Component **)b)->e->guid;
}

static int comp_search (const void * k, const void * d) {
  return
    ((const Entity *)k)->guid -
    (*(const Component **)d)->e->guid;
}

static int sys_sort (const void * a, const void * b) {
  return
    strcmp (
      (*(const System **)a)->comp_name,
      (*(const System **)b)->comp_name
    );
}

static int sys_search (const void * k, const void * d) {
  return
    strcmp (
      k,
      (*(const System **)d)->comp_name
    );
}



Entity * entity_create () {
  Entity * e = xph_alloc (sizeof (Entity), "Entity");
  if (DestroyedEntities != NULL && vector_size (DestroyedEntities) > 0) {
    e->guid = vector_pop_back (e->guid, DestroyedEntities);
  } else {
    e->guid = ++EntityGUIDs;
  }
  if (e->guid == 0) {
    fprintf (stderr, "Entity GUIDs have wrapped around. Now anything is possible!\n");
    e->guid = ++EntityGUIDs;
  }
  e->components = vector_create (2, sizeof (Component *));
  return e;
}

void entity_destroy (Entity * e) {
  Component * c = NULL;
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

bool entity_registerComponentAndSystem (objHandler func) {
  System * reg = xph_alloc (sizeof (System), "System");
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
  System * sys = NULL;
  int
    m = n,
    j = 0,
    highestGUID = 0;
  bool NULLbreak = FALSE;
  Component * c = NULL;
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

  xph_free (components);
  xph_free (indices);
  return final;
}

System * entity_getSystemByName (const char * comp_name) {
  System * sys = NULL;
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


bool component_instantiateOnEntity (const char * comp_name, Entity * e) {
  void * comp_data = NULL;
  System * sys = entity_getSystemByName (comp_name);
  Component * instance = NULL;
  if (sys == NULL) {
    return FALSE;
  }
  obj_message (sys->system, OM_COMPONENT_INIT_DATA, &comp_data, e);
  instance = xph_alloc (sizeof (struct comp_map), "struct comp_map");
  instance->e = e;
  instance->comp_data = comp_data;
  instance->reg = sys;
  instance->comp_guid = ++ComponentGUIDs;
  // we care less about enforcing uniqueness of component guids than we do about entities.
  vector_push_back (sys->entities, instance);
  vector_push_back (e->components, instance);
  vector_sort (sys->entities, comp_sort);
  vector_sort (e->components, comp_sort);
  return TRUE;
}

bool component_removeFromEntity (const char * comp_name, Entity * e) {
  System * sys = entity_getSystemByName (comp_name);
  Component * comp = NULL;
  if (sys == NULL) {
    return FALSE;
  }
  comp = vector_search (sys->entities, e, comp_search);
  if (comp == NULL) {
    fprintf (stderr, "%s: Entity #%d doesn't have a component \"%s\"\n", __FUNCTION__, e->guid, comp_name);
    return FALSE;
  }
  obj_message (sys->system, OM_COMPONENT_DESTROY_DATA, comp->comp_data, e);
  vector_remove (sys->entities, comp);
  vector_remove (e->components, comp);
  xph_free (comp);
  return TRUE;
}

Component * entity_getAs (Entity * e, const char * comp_name) {
  System * sys = entity_getSystemByName (comp_name);
  Component * comp = NULL;
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

bool component_messageEntity (Component * comp, char * message) {
  struct comp_message * msg = xph_alloc (sizeof (struct comp_message), "struct comp_message");
  Component * t = NULL;
  int i = 0;
  msg->from = comp;
  msg->to = NULL;
  msg->message = message;
  while (vector_at (t, comp->e->components, i++) != NULL) {
    if (t == comp) {
      continue;
    }
    msg->to = t;
    obj_message (t->reg->system, OM_COMPONENT_RECEIVE_MESSAGE, msg, NULL);
  }
  xph_free (msg);
  return TRUE;
}

bool component_messageSystem (Component * comp, char * message) {
  obj_message (comp->reg->system, OM_SYSTEM_RECEIVE_MESSAGE, comp, message);
  return TRUE;
}

void entitySubsystem_update (const char * comp_name) {
  System * sys = entity_getSystemByName (comp_name);
  if (sys == NULL) {
    printf ("%s: got invalid component name (\"%s\")\n", __FUNCTION__, comp_name);
    return;
  }
  obj_message (sys->system, OM_UPDATE, NULL, NULL);
}

void entitySubsystem_postupdate (const char * comp_name) {
  System * sys = entity_getSystemByName (comp_name);
  if (sys == NULL) {
    printf ("%s: got invalid component name (\"%s\")\n", __FUNCTION__, comp_name);
    return;
  }
  obj_message (sys->system, OM_POSTUPDATE, NULL, NULL);
}

bool debugComponent_messageReceived (Component * c, char * message) {
  return FALSE;
}

