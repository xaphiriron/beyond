#include "entity.h"

#define ENTITY_VERIFY 0

static struct list * entclasses = NULL;
static struct list * entCallstack = NULL;
static bool EntitySkipChildren = FALSE;
static bool EntityHaltMessaging = FALSE;

static unsigned int entityGuid = 0;

static void entclass_initList ();
static bool entclass_addToList (ENTCLASS * c);
static bool entclass_rmFromList (ENTCLASS * c);

static int entclass_sort (const void * a, const void * b);
static int entclass_search (const void * k, const void * d);

static int entdata_sort (const void * a, const void * b);
static int entdata_search (const void * k, const void * d);

static int entity_pushCallstack (ENTITY * e, eMessage msg, void * a, void * b, const char * func);
static int entity_popCallstack (const ENTITY * e);
static int entity_switchTopCallmode (enum entcalls c);
static bool entity_matchTopCall (const ENTITY *, eMessage msg, enum entcalls c);
static bool entity_matchTopCallStatus (enum entcalls c);

ENTCLASS * entclass_init (eHandler h, const char * parent, void * a, void * b) {
  ENTCLASS
    * ec = NULL,
    * p = NULL;
#ifdef MEM_DEBUG
  char
    classname[32],
    memstr[64];
  h (NULL, EM_CLSNAME, &classname, NULL);
  snprintf (memstr, 63, "ENTCLASS:%s", classname);
  ec = xph_alloc (sizeof (ENTCLASS), memstr);
  strncpy (ec->name, classname, 31);
#else /* MEM_DEBUG */
  ec = xph_alloc (sizeof (ENTCLASS), "...");
  h (NULL, EM_CLSNAME, &ec->name, NULL);
#endif /* MEM_DEBUG */
  ec->parent = NULL;
  ec->firstChild = NULL;
  ec->nextSibling = NULL;
  if (parent != NULL) {
    p = entclass_get (parent);
    if (NULL == p) {
      fprintf (stderr, "Request for non-existant parent class (of \"%s\")\n", parent);
      xph_free (ec);
      return NULL;
    }
    entclass_addChild (p, ec);
  }
  ec->instances = 0;
  ec->handler = h;
  h (NULL, EM_CLSNAME, &ec->name, NULL);
  // entclass added to registry before init in case init wants to make an instance of the class
  entclass_addToList (ec);
  h (NULL, EM_CLSINIT, a, b);
  return ec;
}

ENTCLASS * entclass_get (const char * c) {
  ENTCLASS * ec = NULL;
  if (entclasses == NULL) {
    entclass_initList ();
  }
  if (c == NULL)
    return NULL;
  ec = listSearch (entclasses, c, entclass_search);
  //printf ("%s (\"%s\"): %p\n", __FUNCTION__, c, ec);
  return listSearch (entclasses, c, entclass_search);
}

static void entclass_initList () {
  if (entclasses != NULL) {
    return;
  }
  entclasses = listCreate (4, sizeof (ENTCLASS *));
}

static bool entclass_addToList (ENTCLASS * c) {
  if (entclasses == NULL) {
    entclass_initList ();
  }
  if (entclass_get (c->name) != NULL) {
    fprintf (stderr, "Attempt to add duplicate classes to class list (\"%s\")\n", c->name);
    return FALSE;
  }
  listAdd (entclasses, c);
  listSort (entclasses, entclass_sort);
  return TRUE;
}

static bool entclass_rmFromList (ENTCLASS * c) {
  bool r = listRemove (entclasses, c);
  if (r == TRUE) {
    listSort (entclasses, entclass_sort);
  }/* else {
    fprintf (stderr, "%s (%p): LOL FAIL :(\n", __FUNCTION__, c);
  }*/
  return r;
}

bool entclass_destroyS (const char * c) {
  ENTCLASS * ec = entclass_get (c);
  if (ec == NULL)
    return FALSE;
  return entclass_destroy (ec);
}

bool entclass_destroy (ENTCLASS * e) {
  if (e->instances > 0) {
    return FALSE;
  }
  e->handler (NULL, EM_CLSFREE, NULL, NULL);
  entclass_rmFromList (e);
  entclass_rmChild (e->parent, e);
  xph_free (e);
  return TRUE;
}

void entclass_destroyAll () {
  ENTCLASS * c = NULL;
  while (listItemCount (entclasses) > 0) {
    c = listPop (entclasses);
    if (c->instances > 0) {
      fprintf (stderr, "%s: destroying class \"%s\", which has instances. expect this to crash everything.\n", __FUNCTION__, c->name);
    }
    entclass_destroy (c);
  }
  listDestroy (entclasses);
  entclasses = NULL;
  if (entCallstack != NULL) {
    if (listItemCount (entCallstack) != 0) {
      fprintf (stderr, "%s: destroying the entity callstack even through it has entries in it. Wow, is this a bad idea.\n", __FUNCTION__);
    }
    listDestroy (entCallstack);
    entCallstack = NULL;
  }
}

bool entclass_chparent (ENTCLASS * c, ENTCLASS * p) {
  if (c->parent == NULL)
    return entclass_addChild (p, c);
  return (entclass_rmChild (c->parent, c)
           && entclass_addChild (p, c))
             ? TRUE
             : FALSE;
}

bool entclass_chparentS (char * c, char * p) {
  ENTCLASS
    * ec = entclass_get (c),
    * ep = entclass_get (p);
  if (ec == NULL || (p != NULL && ep == NULL)) {
    return FALSE;
  }
  //printf ("%s (%s, %s): doing real chparent\n", __FUNCTION__, c, p);
  return entclass_chparent (ec, ep);
}

bool entclass_inheritsFrom (const ENTCLASS * ecc, const ENTCLASS * ecp) {
#ifdef ENTITY_VERIFY
  const ENTCLASS
    * t = ecc,
    * u = NULL;
  while (t != NULL) {
    if (t->parent != NULL) {
      u = t->parent->firstChild;
      while (u->nextSibling != NULL) {
        if (u == t) {
          break;
        }
        u = u->nextSibling;
      }
      if (u != t) {
        fprintf (stderr, "%s (%p, %p): Malformed entclass heirarchy: %p is a child of %p, but isn't in its list of children.\n", __FUNCTION__, ecc, ecp, t, t->parent);
        exit (1);
        return FALSE;
      }
    }
    if (t == ecp) {
      return TRUE;
    }
    t = t->parent;
  }
  return FALSE;
#else /* ENTITY_VERIFY */
  const ENTCLASS * c = ecc;
  while (c != NULL) {
    if (c == ecp) {
      return TRUE;
    }
    c = c->parent;
  }
  return FALSE;
#endif /* ENTITY_VERIFY */
}

bool entclass_inheritsFromS (const char * c, const char * p) {
  ENTCLASS
    * ecc = entclass_get (c),
    * ecp = entclass_get (p);
  if (ecc == NULL || ecp == NULL) {
    return FALSE;
  }
  return entclass_inheritsFrom (ecc, ecp);
}

bool entclass_addChild (ENTCLASS * p, ENTCLASS * c) {
  ENTCLASS * t = NULL;
  //printf ("%s (%p, %p)\n", __FUNCTION__, p, c);
  if (entclass_inheritsFrom (p, c) == TRUE) {
    fprintf (stderr, "%s (%p, %p): Classes can't be their own children, at any remove\n", __FUNCTION__, p, c);
    return FALSE;
  }
  c->parent = p;
  if (p == NULL) {
    return TRUE;
  }
  if (p->firstChild == NULL) {
    p->firstChild = c;
    return TRUE;
  }
  t = p->firstChild;
  while (t->nextSibling != NULL) {
    t = t->nextSibling;
  }
  t->nextSibling = c;
  return TRUE;
}

bool entclass_rmChild (ENTCLASS * p, ENTCLASS * c) {
  ENTCLASS * t = NULL;
  //printf ("%s (%p, %p)\n", __FUNCTION__, p, c);
  if (p == NULL) {
    // this happens so often it's not worth warning for. all it means is that this was a useless function call, not anything bad
    //fprintf (stderr, "%s (%p, %p): can't remove %p from null parent\n", __FUNCTION__, p, c, c);
    return FALSE;
  } else if (c->parent != p) {
    fprintf (stderr, "%s (%p, %p): %p is no child of %p.\n", __FUNCTION__, p, c, c, p);
    return FALSE;
  }
  c->parent = NULL;
  if (p->firstChild == c) {
    p->firstChild = c->nextSibling;
    c->nextSibling = NULL;
    return TRUE;
  }
  t = p->firstChild;
  while (t->nextSibling != NULL) {
    if (t->nextSibling == c) {
      t->nextSibling = c->nextSibling;
      c->nextSibling = NULL;
      return TRUE;
    }
    t = t->nextSibling;
  }
  c->nextSibling = NULL;
  fprintf (stderr, "%s (%p, %p): Malformed entclass hierarchy: %p is a child of %p, but wasn't in its list of children\n", __FUNCTION__, p, c, c, p);
  return FALSE;
}

static int entclass_sort (const void * a, const void * b) {
  //printf ("%s (%p, %p): \"%s\" vs. \"%s\"\n", __FUNCTION__, a, b, (*(ENTCLASS **)a)->name, (*(ENTCLASS **)b)->name);
  return strcmp ((*(ENTCLASS **)a)->name, (*(ENTCLASS **)b)->name);
}

static int entclass_search (const void * k, const void * d) {
  //printf ("%s (%p, %p): \"%s\" vs. \"%s\"\n", __FUNCTION__, k, d, (char *)k, (*(ENTCLASS **)d)->name);
  return strcmp (k, (*(ENTCLASS **)d)->name);
}

ENTITY * entity_create (const char * entclass, ENTITY * p, void * a, void * b) {
  ENTITY * e = NULL;
  ENTCLASS
    * ec = entclass_get (entclass),
    * t = NULL;
  struct list * l = NULL;
#ifdef MEM_DEBUG
  char
    classname[32],
    memstr[64];
#endif /* MEM_DEBUG */
  if (ec == NULL) {
    fprintf (stderr, "%s (%s, %p, [...]): Can't make entity with non-existant class \"%s\"\n", __FUNCTION__, entclass, p, entclass);
    return NULL;
  }
#ifdef MEM_DEBUG
  ec->handler (NULL, EM_CLSNAME, &classname, NULL);
  snprintf (memstr, 63, "ENTITY:%s", classname);
  e = xph_alloc (sizeof (ENTITY), memstr);
  e->class = ec;
#else
  e = xph_alloc (sizeof (ENTITY), "...");
  e->class = ec;
#endif
  e->entData = listCreate (3, sizeof (struct entData *));
  e->guid = ++entityGuid;
  if (e->guid == 0) {
    fprintf (stderr, "entity GUIDs have overflowed at entity %p. Now, anything is possible! Expect everything to come crashing down in about a tick or two\n", e);
  }
  e->parent = NULL;
  e->firstChild = NULL;
  e->nextSibling = NULL;
  if (p != NULL) {
    entity_addChild (p, e);
  }
  // we do this so that the entity's classes will be messaged in the right order (base -> derived)
  l = listCreate (4, sizeof (ENTCLASS *));
  t = ec;
  while (t != NULL) {
    //printf ("%s (\"%s\", %p, ...): new entity %p is class \"%s\", which has %d instance%s (now %d)\n", __FUNCTION__, entclass, p, e, t->name, t->instances, (t->instances == 1 ? "" : "s"), t->instances+ 1);
    t->instances++;
    listPush (l, t);
    t = t->parent;
  }
  while ((t = listPop (l)) != NULL) {
    t->handler (e, EM_CREATE, a, b);
  }
  listDestroy (l);
  if (listItemCount (e->entData) > 1) {
    listSort (e->entData, entdata_sort);
  }
  return e;
}

void entity_destroy (ENTITY * e) {
  ENTCLASS
    * ec = e->class;
  ENTITY
    * t = NULL,
    * u = NULL;
  struct entData * ed = NULL;
  //printf ("%s (%p)\n", __FUNCTION__, e);
  while (ec != NULL) {
    //printf ("\"%s\" w/ %d instance%s (now %d)\n", t->name, t->instances, (t->instances == 1 ? "" : "s"), t->instances - 1);
    ec->instances--;
    ec = ec->parent;
  }
  while (listItemCount (e->entData) > 0) {
    ed = listPop (e->entData);
    fprintf (stderr, "%s (%p): un-destroyed \"%s\" class data (%p) at %p\n", __FUNCTION__, e, ed->ref->name, ed->ref, ed->data);
    xph_free (ed);
  }
  listDestroy (e->entData);
  t = e->firstChild;
  while (t != NULL) {
    u = t->nextSibling;
    //printf ("t: %p; u: %p; e: %p; e->parent: %p\n", t, u, e, e->parent);
    entity_chparent (t, e->parent);
    t = u;
  }
  entity_rmChild (e->parent, e);
  xph_free (e);
}

bool entity_addChild (ENTITY * p, ENTITY * c) {
  ENTITY * t = NULL;
  //printf ("%s (%p, %p) ... \n", __FUNCTION__, p, c);
  // TODO: jegus fuck, previously the p == NULL check was later on but I guess that leaves room somewhere in the two ifchecks here (_ischildof and c->parent == p) for p to get dereferenced and crash everything. it's late and i'm tired so i'm just moving this block of code upwards so it passes the unit test; later i should go back through and figure out where it breaks more specifically and fix it there - xph 2010-07-13
  if (p == NULL) {
    c->parent = NULL;
    //printf (" ... %s (TRUE; NULL PARENT)\n", __FUNCTION__);
    return TRUE;
  }
  if (entity_isChildOf (p, c) == TRUE) {
    fprintf (stderr, "%s (%p, %p): Making %p the child of %p would create a hierarchal loop, which is forbidden.\n", __FUNCTION__, p, c, c, p);
    return FALSE;
  } else if (c->parent == p) {
    fprintf (stderr, "%s (%p, %p): Entity %p is already the child of %p.\n", __FUNCTION__, p, c, c, p);
    return FALSE;
  }
  if (c->parent != NULL) {
    fprintf (stderr, "%s (%p, %p): child (%p) alreay has a parent (%p); this could get tangled...\n", __FUNCTION__, p, c, c, p);
    entity_rmChild (c->parent, c);
  }
  c->parent = p;
  if (p->firstChild == NULL) {
    p->firstChild = c;
    //printf (" ... %s (TRUE; ONLY CHILD)\n", __FUNCTION__);
    return TRUE;
  }
  t = p->firstChild;
  while (t->nextSibling != NULL) {
    t = t->nextSibling;
  }
  t->nextSibling = c;
  //printf (" ... %s (TRUE)\n", __FUNCTION__);
  return TRUE;
}

bool entity_rmChild (ENTITY * p, ENTITY * c) {
  ENTITY * t = NULL;
  //printf ("%s (%p, %p) ... \n", __FUNCTION__, p, c);
  if (p == NULL) {
    // this happens so often it's not worth warning for. all it means is that this was a useless function call, not anything bad
    //fprintf (stderr, "%s (%p, %p): can't remove %p from null parent\n", __FUNCTION__, p, c, c);
    return FALSE;
  } else if (c->parent != p) {
    fprintf (stderr, "%s (%p, %p): %p is no child of %p\n", __FUNCTION__, p, c, c, p);
    return FALSE;
  }
  c->parent = NULL;
  if (p->firstChild == c) {
    p->firstChild = c->nextSibling;
    c->nextSibling = NULL;
    //printf (" ... %s (TRUE; ONLY CHILD)\n", __FUNCTION__);
    return TRUE;
  }
  t = p->firstChild;
  while (t->nextSibling != NULL) {
    if (t->nextSibling == c) {
      t->nextSibling = c->nextSibling;
      c->nextSibling = NULL;
      //printf (" ... %s (TRUE)\n", __FUNCTION__);
      return TRUE;
    }
    t = t->nextSibling;
  }
  fprintf (stderr, "%s (%p, %p): Malformed entity hierarchy: %p is a child of %p, but wasn't in its list of children\n", __FUNCTION__, p, c, c, p);
  c->nextSibling = NULL;
  return FALSE;
}

bool entity_addClassData (ENTITY * e, const char * c, void * d) {
  void * od = entity_getClassData (e, c);
  ENTCLASS * ec = entclass_get (c);
  struct entData * ed = NULL;
  if (od != NULL) {
    fprintf (stderr, "%s (%p, \"%s\", %p): data already exists at %p; unable to add new class data\n", __FUNCTION__, e, c, d, od);
    return FALSE;
  } else if (ec == NULL) {
    fprintf (stderr, "%s (%p, \"%s\", %p): class \"%s\" does not exist; unable to add new class data\n", __FUNCTION__, e, c, d, c);
    return FALSE;
  }
  ed = xph_alloc (sizeof (struct entData), "struct entData");
  //printf ("!!! ADDING CLASS DATA TO %p: %p \"%s\" (at %p) !!!\n", e, ec, c, ed);
  ed->ref = ec;
  ed->data = d;
  listAdd (e->entData, ed);
  listSort (e->entData, entdata_sort);
  assert (entity_getClassData (e, c) == ed->data);
  return TRUE;
}

void * entity_getClassData (const ENTITY * e, const char * c) {
  struct entData * ed = NULL;
  if (e == NULL || e->entData == NULL) {
    fprintf (stderr, "%s: entity nonexistant or had no class registry; something's up (%p, \"%s\")\n", __FUNCTION__, e, c);
    return NULL;
  }
  ed = listSearch (e->entData, c, entdata_search);
  if (ed == NULL) {
    // this would get printed for every single entity created since _addClassData checks to make sure it's not overwriting already existant class data.
    //fprintf (stderr, "%s: looking for data \"%s\" in entity %p; no such data\n", __FUNCTION__, c, e);
    return NULL;
  }
  return ed->data;
}

bool entity_rmClassData (ENTITY * e, const char * c) {
  struct entData * ed = listSearch (e->entData, c, entdata_search);
  if (ed == NULL) {
    return FALSE;
  }
  listRemove (e->entData, ed);
  listSort (e->entData, entdata_sort);
  xph_free (ed);
  return TRUE;
}


bool entity_isa (const ENTITY * e, const ENTCLASS * ec) {
  ENTCLASS * t = e->class;
  while (t != NULL) {
    if (t == ec) {
      return TRUE;
    }
    t = t->parent;
  }
  return FALSE;
}

bool entity_isaS (const ENTITY * e, const char * c) {
  ENTCLASS * ec = entclass_get (c);
  if (ec == NULL)
    return FALSE;
  return entity_isa (e, ec);
}

bool entity_isChildOf (const ENTITY * c, const ENTITY * p) {
  ENTITY * t = c->parent;
  if (p == NULL) {
    return FALSE;
  }
  while (t != NULL) {
    //printf ("%p->parent: %p\n", t, t->parent);
    if (t == p) {
      return TRUE;
    }
    t = t->parent;
  }
  return FALSE;
}

bool entity_areSiblings (const ENTITY * e, const ENTITY * f) {
#ifdef ENTITY_VERIFY
  unsigned short i = 0;
  ENTITY * t = NULL;
  if (e->parent == NULL || f->parent == NULL) {
    return FALSE;
  }
  t = e->parent->firstChild;
  while (t != NULL) {
    //printf ("%p, looking for %p) %p->nextSibling: %p\n", e, f, t, t->nextSibling);
    if (t == e) {
      i += 2;
    } else if (t == f) {
      i += 3;
    }
    t = t->nextSibling;
  }
  switch (i) {
    case 5:
      return TRUE;
    case 2:
    case 3:
      return FALSE;
    case 0:
      fprintf (stderr, "Malformed entity hierarchy: %p is a child of %p, but isn't in its list of children.\n", e, e->parent);
      exit (1);
    default:
      fprintf (stderr, "Malformed entity hierarchy: duplicate entries of %p or %p in %p's list of children\n", e, f, e->parent);
      exit (1);
  }
  return FALSE;
#else /* ENTITY_VERIFY */
  if (e->parent == NULL || f->parent == NULL) {
    return FALSE;
  }
  return (e->parent == f->parent)
    ? TRUE
    : FALSE;
#endif /* ENTITY_VERIFY */
}

int entity_siblingCount (const ENTITY * e) {
  if (e->parent == NULL) {
    return 0;
  }
  return entity_childrenCount (e->parent) - 1;
}

int entity_childrenCount (const ENTITY * e) {
  ENTITY * t = e->firstChild;
  int i = 0;
  while (t != NULL) {
    i++;
    t = t->nextSibling;
  }
  return i;
}

void entity_chparent (ENTITY * e, ENTITY * newp) {
  entity_rmChild (e->parent, e);
  entity_addChild (newp, e);
}


static int entdata_sort (const void * a, const void * b) {
  return strcmp ((*(struct entData **)a)->ref->name, (*(struct entData **)b)->ref->name);
}

static int entdata_search (const void * k, const void * d) {
  return strcmp (k, (*(struct entData **)d)->ref->name);
}

/***
 * all of these functions need to push the registers on new call. the way to do this is to check at the beginning of each function to see if 1) the call stack is empty (in which case the call is new and needs initialized registers anyway or 2) if the top call on the stack is EC_INHANDLER (in which case the function was called from inside a message handler and the existing registers need to be pushed and reinitialized, then popped at the end). this can be achieved by adding an ifcheck that tests the above near the start that pushes the registers and flips a "clear" bool, and an ifcheck at the end that pops the registers if "clear" is set. so. do that.
 ***/

int entity_message (ENTITY * e, eMessage msg, void * a, void * b) {
  int r = 0;
  bool directMessage = FALSE;

  if (e == NULL) {
    return 0;
  }

  if (entity_matchTopCallStatus (EC_INHANDLER)) {
    // entity_message was called anew from inside a message handler
    entity_resetRegisters ();
  }
  if (!entity_matchTopCall (e, msg, EC_STARTED)) {
    directMessage = TRUE;
    entity_pushCallstack (e, msg, a, b, __FUNCTION__);
  }
  entity_switchTopCallmode (EC_INHANDLER);
  //printf ("%s (%p [:\"%s\"], %d, %p, %p)\n", __FUNCTION__, e, e->class->name, msg, a, b);
  r = e->class->handler (e, msg, a, b);
  entity_switchTopCallmode (EC_COMPLETED);
  if (directMessage) {
    entity_popCallstack (e);
  }

  return r;
}

int entity_messageSiblings (ENTITY * e, eMessage msg, void * a, void * b) {
  ENTITY
   * t = NULL,
   * u = NULL;
  int r = 0;
  if (e->parent == NULL) {
    return 0;
  }
  t = e->parent->firstChild;
  while (t != NULL) {
    u = t->nextSibling;
    if (t == e) {
      t = u;
      continue;
    }
    r = entity_message (t, msg, a, b);
    t = u;
  }
  entity_resetRegisters ();
  return r;
}

int entity_messageChildren (ENTITY * e, eMessage msg, void * a, void * b) {
  ENTITY
    * t = NULL,
    * u = NULL;
  int r = 0;
  t = e->firstChild;
  while (t != NULL) {
    u = t->nextSibling;
    r = entity_message (t, msg, a, b);
    t = u;
  }
  entity_resetRegisters ();
  return r;
}

int entity_messageParents (ENTITY * e, eMessage msg, void * a, void * b) {
  ENTITY
    * t = NULL,
    * u = NULL;
  int r = 0;
  t = e->parent;
  while (t != NULL) {
    u = t->parent;
    r = entity_message (t, msg, a, b);
    t = u;
  }
  entity_resetRegisters ();
  return r;
}

int entity_messagePre (ENTITY * e, eMessage msg, void * a, void * b) {
  int r = 0;
  ENTITY
    * firstChild = NULL,
    * nextSibling = NULL;
  if (e == NULL) {
    return 0;
  }
  // record these here in case this message causes e to destroy itself
  firstChild = e->firstChild;
  nextSibling = e->nextSibling;
  //printf ("%s (%p [:\"%s\"], ...): firstchild: %p; nextsibling: %p\n", __FUNCTION__, e, e->class->name, firstChild, nextSibling);
  if (listItemCount (entCallstack) == 0 || entity_matchTopCallStatus (EC_INHANDLER)) {
    entity_resetRegisters ();
  }
  entity_pushCallstack (e, msg, a, b, __FUNCTION__);
  r = entity_message (e, msg, a, b);
  if (!EntitySkipChildren && !EntityHaltMessaging) {
    r = entity_messagePre (firstChild, msg, a, b);
  }
  if (!EntityHaltMessaging) {
    r = entity_messagePre (nextSibling, msg, a, b);
  }
  entity_popCallstack (e);
  return r;
}

int entity_messagePost (ENTITY * e, eMessage msg, void * a, void * b) {
  int r = 0;
  if (e == NULL) {
    return 0;
  }
  if (listItemCount (entCallstack) == 0 || entity_matchTopCallStatus (EC_INHANDLER)) {
    entity_resetRegisters ();
  }
  entity_pushCallstack (e, msg, a, b, __FUNCTION__);
  r = entity_messagePost (e->nextSibling, msg, a, b);
  if (!EntityHaltMessaging) {
    r = entity_messagePost (e->firstChild, msg, a, b);
  }
  if (!EntityHaltMessaging) {
    r = entity_message (e, msg, a, b);
  }
  entity_popCallstack (e);
  return r;
}

int entity_messageIn (ENTITY * e, eMessage msg, void * a, void * b) {
  int r = 0;
  ENTITY
    * nextSibling = NULL;
  if (e == NULL) {
    return 0;
  }
  // record this here in case this message makes e destroy itself
  nextSibling = e->nextSibling;
  if (listItemCount (entCallstack) == 0 || entity_matchTopCallStatus (EC_INHANDLER)) {
    entity_resetRegisters ();
  }
  entity_pushCallstack (e, msg, a, b, __FUNCTION__);
  r = entity_messageIn (e->firstChild, msg, a, b);
  r = entity_message (e, msg, a, b);
  r = entity_messageIn (nextSibling, msg, a, b);
  entity_popCallstack (e);
  return r;
}


struct entMessage * entity_createMessage (ENTITY * e, eMessage msg, void * a, void * b) {
  struct entMessage * em = xph_alloc (sizeof (struct entMessage), "struct entMessage");
  em->e = e;
  em->msg = msg;
  em->a = a;
  em->b = b;
  return em;
}

void entity_destroyMessage (struct entMessage * em) {
  xph_free (em);
}

int entity_sendMessage (const struct entMessage * em) {
  return entity_message (em->e, em->msg, em->a, em->b);
}

// this fails. if, during the ->handler call, entity_pass is called again (either manually or if the message is unhandled) this will enter an infinite recursive loop, with entity_pass() calling a handler that calls entity_pass again. the solution is probably something to do with the callstack-- add in a new call for the pass, and then use that to get parents and grandparents somehow. the difficulty is that we need to always pass the same child entity no matter the handler, so we'd have to... make a new EC_PASS call on top of its existing EC_INHANDER call, and then the next entity_pass call should see the EC_PASS and use that entity's parent handler with the entity value of the most recent non-EC_PASS call. this means we have to delve through the call stack.
int entity_pass () {
  struct entCall
    * base = NULL,
    * mostRecent = NULL;
  ENTCLASS
    * try = NULL;
  int
    i = 0,
    r = 0;
  //printf ("%s: OH GOD, WE'RE IN ENTITY_PASS()\n", __FUNCTION__);
  if (entCallstack == NULL || (i = listItemCount (entCallstack)) == 0) {
    fprintf (stderr, "Don't call %s outside of an entity handler >:(\n", __FUNCTION__);
    return -1;
    //fprintf (stderr, "%s: OFF TO A GREAT START: THE CALLSTACK DOESN'T EXIST\n", __FUNCTION__);
    //entCallstack = listCreate (4, sizeof (struct entCall *));
  }
/*
  i = listItemCount (entCallstack);
  if (i == 0) {
    fprintf (stderr, "Don't call %s outside of an entity handler >:(\n", __FUNCTION__);
    //fprintf (stderr, "%s: callstack is empty. this is probably passing's fault somehow.", __FUNCTION__);
    return -1;
  }
*/
  //printf ("%s: LET'S EXAMINE THE CALLSTACK!\n", __FUNCTION__);
  mostRecent = listIndex (entCallstack, i - 1);
  while ((base = listIndex (entCallstack, --i))->stage == EC_PASS) {
    if (i == 0) {
      fprintf (stderr, "%s: the callstack is in a bad state: all registered calls are pass invocations, which should be impossible (but apparently isn't)\n", __FUNCTION__);
      exit (1);
    }
  }
  if (base == mostRecent) {
    try = base->e->class->parent;
  } else {
    try = ((ENTCLASS *)mostRecent->e)->parent;
  }
  //printf ("%s: LET'S ASSUME WE HAVE THE BASE CALL (%p) AS WELL AS THE LAST PASS CLASS TO BE TRIED (%p) AT THIS POINT. THIS GIVES US OUR NEXT TRY OF %p\n", __FUNCTION__, base, mostRecent, try);
  if (try == NULL) {
    // this message can't be passed.
    //printf ("%s: THIS MESSAGE CAN'T BE HANDLED BY ANYTHING. (%p)\n", __FUNCTION__, base->e);
    return EXIT_FAILURE;
  }
  entity_pushCallstack ((ENTITY *)try, base->msg, base->a, base->b, __FUNCTION__);
  entity_switchTopCallmode (EC_PASS);
  //printf ("%s: TRYING HANDLER \"%s\" (%p)\n", __FUNCTION__, try->name, base->e);
  r = try->handler (base->e, base->msg, base->a, base->b);
  entity_popCallstack ((ENTITY *)try);
  return r;
}

int entity_skipchildren () {
  EntitySkipChildren = TRUE;
  return 0;
}

int entity_halt () {
  EntityHaltMessaging = TRUE;
  return 0;
}

static int entityRegisters[4] = {0, 0, 0, 0};

int entity_getRegister (int r) {
  if (r < 0 || r > 3) {
    return -1;
  }
  return entityRegisters[r];
}

void entity_setRegister (int r, int val) {
  if (r < 0 || r > 3) {
    return;
  }
  entityRegisters[r] = val;
}

void entity_resetRegisters () {
  int i = 0;
  while (i < 4) {
    entityRegisters[i++] = 0;
  }
}

static int entity_pushCallstack (ENTITY * e, eMessage msg, void * a, void * b, const char * func) {
  struct entCall * call = NULL;
  int l = strlen (func);
  if (entCallstack == NULL) {
    entCallstack = listCreate (4, sizeof (struct entCall *));
  }
  call = xph_alloc (sizeof (struct entCall), "struct entCall");
  call->e = e;
  call->msg = msg;
  call->a = a;
  call->b = b;
  call->func = xph_alloc (l + 1, "entCall->func");
  strncpy (call->func, func, l + 1);
  call->stage = EC_STARTED;
  listPush (entCallstack, call);
  // I think that by putting this here we can ensure that kids will only be skipped for one message iteration at max, so that only the kids of an entity which calls entity_skipchildren() will be skipped, and no others.
  EntitySkipChildren = FALSE;
  return listItemCount (entCallstack);
}

static int entity_popCallstack (const ENTITY * e) {
  struct entCall * call = NULL;
  if (entCallstack == NULL) {
    entCallstack = listCreate (4, sizeof (struct entCall *));
  }
  call = listPop (entCallstack);
  if (call == NULL) {
    fprintf (stderr, "%s: was expecting %p, got (NO CALLSTACK)\n", __FUNCTION__, e);
    exit (1);
  } else if (call->e != e) {
    fprintf (stderr, "%s: was expecting %p, got %p\n", __FUNCTION__, e, call->e);
    exit (1);
  }
  xph_free (call->func);
  xph_free (call);
  if (listItemCount (entCallstack) == 0) {
    EntityHaltMessaging = FALSE;
    EntitySkipChildren = FALSE;
  }
  return listItemCount (entCallstack);
}

static int entity_switchTopCallmode (enum entcalls c) {
  struct entCall * call = NULL;
  int i = 0;
  if (entCallstack == NULL) {
    entCallstack = listCreate (4, sizeof (struct entCall *));
  }
  if ((i = listItemCount (entCallstack)) == 0) {
    return 0;
  }
  call = listIndex (entCallstack, i - 1);
  call->stage = c;
  return i;
}

static bool entity_matchTopCall (const ENTITY * e, eMessage msg, enum entcalls c) {
  struct entCall * call = NULL;
  int i = 0;
  if (entCallstack == NULL) {
    entCallstack = listCreate (4, sizeof (struct entCall *));
  }
  if ((i = listItemCount (entCallstack)) == 0) {
    return FALSE;
  }
  call = listIndex (entCallstack, i - 1);
  if (call->e == e && call->msg == msg && call->stage == c) {
    return TRUE;
  }
  return FALSE;
}

static bool entity_matchTopCallStatus (enum entcalls c) {
  struct entCall * call = NULL;
  int i = 0;
  if (entCallstack == NULL) {
    entCallstack = listCreate (4, sizeof (struct entCall *));
  }
  if ((i = listItemCount (entCallstack)) == 0) {
    return FALSE;
  }
  call = listIndex (entCallstack, i - 1);
  if (call->stage == c) {
    return TRUE;
  }
  return FALSE;
}

const ENTITY * entcallstack_activeEntity () {
  struct entCall * call = NULL;
  int i = 0;
  if (entCallstack == NULL || (i = listItemCount (entCallstack)) == 0) {
    fprintf (stderr, "don't call %s outside of an entity handler >:(\n", __FUNCTION__);
    return NULL;
  }
  while ((call = listIndex (entCallstack, --i)) != NULL) {
    switch (call->stage) {
      case EC_INHANDLER:
        return call->e;
      case EC_PASS:
        break;
      case EC_STARTED:
      case EC_COMPLETED:
      default:
        fprintf (stderr, "something wrong with the entity call stack :/\n");
        return NULL;
    }
  }
  return NULL;
}

const ENTCLASS * entcallstack_activeEntclass () {
  struct entCall * call = NULL;
  int i = 0;
  if (entCallstack == NULL || (i = listItemCount (entCallstack)) == 0) {
    fprintf (stderr, "don't call %s outside of an entity handler >:(\n", __FUNCTION__);
    return NULL;
  }
  call = listIndex (entCallstack, i - 1);
  if (call->stage == EC_INHANDLER) {
    return call->e->class;
  } else if (call->stage == EC_PASS) {
    return (ENTCLASS *)call->e;
  } else {
    fprintf (stderr, "something wrong with the entity call stack :/\n");
    return NULL;
  }
}