#include "object.h"

static Dynarr ObjectClasses = NULL;
static void objClass_initVector ();
static void objClass_addToVector (ObjClass * c);
static void objClass_rmFromVector (ObjClass * c);

static int objClass_search (const void * k, const void * d);
static int objClass_sort (const void * a, const void * b);

static int objData_search (const void * k, const void * d);
static int objData_sort (const void * a, const void * b);

static unsigned int ObjectGUID = 0;

static bool ObjectHaltMessaging = FALSE;
static bool ObjectSkipChildren = FALSE;
static Dynarr ObjectMessageCallstack = NULL;

static void obj_recordMessageChildren (Dynarr v, Object * o);
static void obj_recordMessageSiblings (Dynarr v, Object * o);
static void obj_recordMessageParents (Dynarr v, Object * o);

static void obj_recordMessagePre (Dynarr v, Object * o);
static void obj_recordMessageIn (Dynarr v, Object * o);
static void obj_recordMessagePost (Dynarr v, Object * o);

static void obj_pushMessageCallstack (void * o, const char * func, objMsg msg, void * a, void * b);
static void obj_setCallType (enum object_calls t);
static void obj_popMessageCallstack (void * o);

static int obj_messageBACKEND (Object * o, const char * func, objMsg msg, void * a, void * b);

void objects_destroyEverything () {
  ObjClass * c = NULL;
  struct objCall * m = NULL;
  while ((c = *(ObjClass **)dynarr_pop (ObjectClasses)) != NULL) {
    if (c->instances != 0) {
      fprintf (stderr, "%s: destroying class \"%s\", which has %d instance%s. expect this to crash everything.\n", __FUNCTION__, c->name, c->instances, (c->instances == 1 ? "" : "s"));
    }
    objClass_destroyP (c);
  }
  dynarr_destroy (ObjectClasses);
  ObjectClasses = NULL;
  if (ObjectMessageCallstack != NULL) {
    if (!dynarr_isEmpty (ObjectMessageCallstack)) {
      fprintf (stderr, "%s: destroying the object message callstack even through it has entries in it. Wow, is this a bad idea.\n", __FUNCTION__);
      while ((m = *(struct objCall **)dynarr_pop (ObjectMessageCallstack)) != NULL) {
        xph_free (m->function);
        xph_free (m);
      }
    }
    dynarr_destroy (ObjectMessageCallstack);
    ObjectMessageCallstack = NULL;
  }
}

ObjClass * objClass_init (objHandler h, const char * pn, void * a, void * b) {
  //printf ("%s()...\n", __FUNCTION__);
  ObjClass
    * c = NULL,
    * p = NULL;
#ifdef MEM_DEBUG
  char
    classname[32],
    memstr[64];
  memset (classname, '\0', 32);
  h (NULL, OM_CLSNAME, &classname, NULL);
  snprintf (memstr, 64, "ObjClass:%s", classname);
  c = xph_alloc_name (sizeof (ObjClass), memstr);
  strncpy (c->name, classname, 32);
#else /* MEM_DEBUG */
  c = xph_alloc (sizeof (ObjClass));
  memset (c->name, '\0', 32);
  h (NULL, OM_CLSNAME, &c->name, NULL);
#endif /* MEM_DEBUG */
  if (strlen (c->name) == 0) {
    fprintf (stderr, "%s: Class handler (%p) is incompetent; we can't make this object class.\n", __FUNCTION__, h);
    xph_free (c);
    return NULL;
  }
  c->parent = NULL;
  c->firstChild = NULL;
  c->nextSibling = NULL;
  if (pn != NULL) {
    p = objClass_get (pn);
    if (NULL == p) {
      fprintf (stderr, "Request for non-existant parent class (of \"%s\")\n", pn);
      xph_free (c);
      return NULL;
    }
    objClass_addChild (p, c);
  }
  c->instances = 0;
  c->handler = h;
  objClass_addToVector (c);
  h (NULL, OM_CLSINIT, a, b);
  //printf ("...%s()\n", __FUNCTION__);
  return c;
}

ObjClass * objClass_get (const char * n) {
  //printf ("%s(\"%s\")...\n", __FUNCTION__, n);
  ObjClass * c = NULL;
  if (ObjectClasses == NULL) {
    objClass_initVector ();
  }
  if (n == NULL) {
    return NULL;
  }
  c = *(ObjClass **)dynarr_search (ObjectClasses, objClass_search, n);
  //printf ("...%s(%p)\n", __FUNCTION__, c);
  return c;
}

bool objClass_destroy (const char * n) {
  ObjClass * c = objClass_get (n);
  if (c == NULL) {
    return FALSE;
  }
  return objClass_destroyP (c);
}

bool objClass_destroyP (ObjClass * c) {
  if (c->instances > 0) {
    return FALSE;
  }
  c->handler (NULL, OM_CLSFREE, NULL, NULL);
  objClass_rmFromVector (c);
  // does rmChild remove and unlink its kids, too?
  objClass_rmChild (c->parent, c);
  xph_free (c);
  return TRUE;
}

bool objClass_addChild (ObjClass * p, ObjClass * c) {
  ObjClass * t = NULL;
  //printf ("%s (%p, %p)\n", __FUNCTION__, p, c);
  if (objClass_inheritsP (c, p) == TRUE) {
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

bool objClass_rmChild (ObjClass * p, ObjClass * c) {
  ObjClass * t = NULL;
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
  fprintf (stderr, "%s (%p, %p): Malformed class hierarchy: %p is a child of %p, but wasn't in its list of children\n", __FUNCTION__, p, c, c, p);
  return FALSE;
}

bool objClass_inheritsP (ObjClass * p, ObjClass * c) {
#ifdef OBJECT_VERIFY
  const ObjClass
    * t = c,
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
        fprintf (stderr, "%s (%p, %p): Malformed class heirarchy: %p is a child of %p, but isn't in its list of children.\n", __FUNCTION__, c, p, t, t->parent);
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
#else /* OBJECT_VERIFY */
  const ObjClass * t = c;
  while (t != NULL) {
    if (t == p) {
      return TRUE;
    }
    t = t->parent;
  }
  return FALSE;
#endif /* OBJECT_VERIFY */
}

bool objClass_inherits (const char * pn, const char * cn) {
  ObjClass
    * c = objClass_get (cn),
    * p = objClass_get (pn);
  if (c == NULL || p == NULL) {
    return FALSE;
  }
  return objClass_inheritsP (p, c);
}

bool objClass_chparentP (ObjClass * p, ObjClass * c) {
  if (c->parent == NULL)
    return objClass_addChild (p, c);
  return (objClass_rmChild (c->parent, c)
           && objClass_addChild (p, c))
             ? TRUE
             : FALSE;
}

bool objClass_chparent (const char * pn, const char * cn) {
  ObjClass
    * c = objClass_get (cn),
    * p = objClass_get (pn);
  if (c == NULL || (p != NULL && pn == NULL)) {
    return FALSE;
  }
  //printf ("%s (%s, %s): doing real chparent\n", __FUNCTION__, c, p);
  return objClass_chparentP (p, c);
}





static void objClass_initVector () {
  if (ObjectClasses != NULL) {
    return;
  }
  ObjectClasses = dynarr_create (6, sizeof (ObjClass *));
}

static void objClass_addToVector (ObjClass * c) {
  if (ObjectClasses == NULL) {
    objClass_initVector ();
  }
  //printf ("adding class %p (%s) to vector\n", c, c->name);
  dynarr_push (ObjectClasses, c);
  dynarr_sort (ObjectClasses, objClass_sort);
}

static void objClass_rmFromVector (ObjClass * c) {
  int index;
  if (ObjectClasses == NULL) {
    objClass_initVector ();
    return;
  }
  index = in_dynarr (ObjectClasses, c);
  if (index >= 0) {
    dynarr_unset (ObjectClasses, index);
    dynarr_condense (ObjectClasses);
  }
}

// I do not trust how this apparently works upon pointers. My understanding of qsort/bsearch was that if you iterated over, say, int i[10] it would send &i[0], &i[1], ... &i[9], so that you would be working with const int * in these functions. Since the object classes are stored as ObjClass * in the vector to begin with, this should pass in a memory address inside the vector list, i.e., v->l + n, which is a pointer to that ObjClass *, i.e., ObjClass **. The fact that it only seems to require one deference is suspicious.
// (as it turns out I was right to be suspicious, but note left intact to explain just why these are pointers to pointers)
static int objClass_search (const void * k, const void * d) {
  //printf ("%s: got vals %p and %p\n", __FUNCTION__, k, d);
  //printf ("%s: names: \"%s\" and \"%s\"\n", __FUNCTION__, *(const char **)k, (*(const ObjClass **)d)->name);
  return strcmp (*(const char **)k, (*(const ObjClass **)d)->name);
}

static int objClass_sort (const void * a, const void * b) {
  //printf ("%s: got vals %p and %p\n", __FUNCTION__, a, b);
  //printf ("%s: names: \"%s\" and \"%s\"\n", __FUNCTION__, (*(const ObjClass **)a)->name, (*(const ObjClass **)b)->name);
  return strcmp ((*(const ObjClass **)a)->name, (*(const ObjClass **)b)->name);
}


Object * obj_create (const char * n, Object * p, void * a, void * b) {
  //printf ("%s()...\n", __FUNCTION__);
  Object * o = NULL;
  ObjClass
    * c = objClass_get (n),
    * t = NULL;
  Dynarr v = NULL;
#ifdef MEM_DEBUG
  char
    classname[32],
    memstr[64];
#endif /* MEM_DEBUG */
  if (c == NULL) {
    fprintf (stderr, "%s (%s, %p, [...]): Can't make object with non-existant class \"%s\"\n", __FUNCTION__, n, p, n);
    return NULL;
  }
#ifdef MEM_DEBUG
  c->handler (NULL, OM_CLSNAME, &classname, NULL);
  snprintf (memstr, 64, "OBJECT:%s", classname);
  o = xph_alloc_name (sizeof (Object), memstr);
  o->class = c;
#else /* MEM_DEBUG */
  o = xph_alloc (sizeof (Object));
  o->class = c;
#endif /* MEM_DEBUG */
  o->objData = dynarr_create (1, sizeof (struct objData *));
  o->guid = ++ObjectGUID;
  if (o->guid == 0) {
    fprintf (stderr, "entity GUIDs have overflowed at entity %p. Now, anything is possible! Expect everything to come crashing down in about a tick or two\n", o);
    o->guid = ++ObjectGUID;
  }
  o->parent = NULL;
  o->firstChild = NULL;
  o->nextSibling = NULL;
  if (p != NULL) {
    obj_addChild (p, o);
  }
  v = dynarr_create (2, sizeof (ObjClass *));
  t = c;
  while (t != NULL) {
    t->instances++;
    dynarr_push (v, t);
    t = t->parent;
  }
  while ((t = *(ObjClass **)dynarr_pop (v)) != NULL) {
    t->handler (o, OM_CREATE, a, b);
  }
  dynarr_destroy (v);
  //printf ("...%s()\n", __FUNCTION__);
  return o;
}

void obj_destroy (Object * o) {
  ObjClass
    * c = NULL;
  Object
    * t = NULL,
    * u = NULL;
  struct objData * od = NULL;
  //printf ("%s (%p)\n", __FUNCTION__, o);
  if (o == NULL || o->class == NULL) {
    fprintf (stderr, "%s (%p): got a NULL object or an object with a NULL class\n", __FUNCTION__, o);
  }
  c = o->class;
  while (c != NULL) {
    //printf ("\"%s\" w/ %d instance%s (now %d)\n", c->name, c->instances, (c->instances == 1 ? "" : "s"), c->instances - 1);
    c->instances--;
    c = c->parent;
  }
  while (!dynarr_isEmpty (o->objData)) {
    od = *(struct objData **)dynarr_pop (o->objData);
    fprintf (stderr, "%s (%p): un-destroyed \"%s\" class data (%p) at %p (this is going to cause a memory leak)\n", __FUNCTION__, o, od->ref->name, od->ref, od->data);
    xph_free (od);
  }
  dynarr_destroy (o->objData);
  o->objData = NULL;
  t = o->firstChild;
  while (t != NULL) {
    //printf ("t: %p\n", t);
    u = t->nextSibling;
    //printf ("  u: %p; o: %p; o->parent: %p\n", u, o, o->parent);
    obj_chparent (o->parent, t);
    t = u;
  }
  obj_rmChild (o->parent, o);
  xph_free (o);
  //printf ("...%s (%p)\n", __FUNCTION__, o);
}

bool obj_addChild (Object * p, Object * c) {
  Object * t = NULL;
  //printf ("%s (%p, %p) ... \n", __FUNCTION__, p, c);
  if (c == NULL) {
    fprintf (stderr, "%s: (%p, %p): invalid NULL object as child\n", __FUNCTION__, p, c);
    return FALSE;
  } else if (obj_isChild (c, p) == TRUE) {
    fprintf (stderr, "%s (%p, %p): Making %p the child of %p would create a hierarchal loop, which is forbidden.\n", __FUNCTION__, p, c, c, p);
    return FALSE;
  } else if (c->parent == p) {
    //fprintf (stderr, "%s (%p, %p): Entity %p is already the child of %p.\n", __FUNCTION__, p, c, c, p);
    return FALSE;
  }
  //printf ("past first child/parent check\n");
  if (c->parent != NULL) {
    fprintf (stderr, "%s (%p, %p): child (%p) alreay has a parent (%p); this could get tangled... (so we're disconnecting it from its old parent)\n", __FUNCTION__, p, c, c, p);
    obj_rmChild (c->parent, c);
  }
  if (p == NULL) {
    c->parent = NULL;
    //printf (" ... %s (TRUE; NULL PARENT)\n", __FUNCTION__);
    return TRUE;
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

bool obj_rmChild (Object * p, Object * c) {
  Object * t = NULL;
  //printf ("%s (%p, %p) ... \n", __FUNCTION__, p, c);
  if (p == NULL || c == NULL) {
    // this happens so often it's not worth warning for. all it means is that this was a useless function call, not anything bad
    //fprintf (stderr, "%s (%p, %p): got invalid null object\n", __FUNCTION__, p, c, c);
    return FALSE;
  } else if (c->parent != p) {
    fprintf (stderr, "%s (%p, %p): %p is no child of %p\n", __FUNCTION__, p, c, c, p);
    return FALSE;
  }
  c->parent = NULL;
  if (p->firstChild == c) {
    p->firstChild = c->nextSibling;
    c->nextSibling = NULL;
    //printf (" ... %s (TRUE; FIRST CHILD)\n", __FUNCTION__);
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
  fprintf (stderr, "%s (%p, %p): Malformed object hierarchy: %p is a child of %p, but wasn't in its list of children\n", __FUNCTION__, p, c, c, p);
  c->nextSibling = NULL;
  return FALSE;
}

void obj_chparent (Object * p, Object * c) {
  if (c == NULL) {
    fprintf (stderr, "%s (%p, %p): passed an NULL object as a child; nothing to do.\n", __FUNCTION__, p, c);
    return;
  }
  obj_rmChild (c->parent, c);
  obj_addChild (p, c);
}

static int objData_search (const void * k, const void * d) {
  //printf ("%s: names: \"%s\" and \"%s\"\n", __FUNCTION__, *(char **)k, (*(const struct objData **)d)->ref->name);
  return strcmp (*(char **)k, (*(const struct objData **)d)->ref->name);
}

static int objData_sort (const void * a, const void * b) {
  //printf ("%s: names: \"%s\" and \"%s\"\n", __FUNCTION__, (*(const struct objData **)a)->ref->name, (*(const struct objData **)b)->ref->name);
  return
    strcmp (
      (*(const struct objData **)a)->ref->name,
      (*(const struct objData **)b)->ref->name
    )
  ;
}

const char * obj_getClassName (const Object * o) {
  if (o == NULL || o->class == NULL) {
    fprintf (stderr, "%s: given NULL object or object with NULL class.\n", __FUNCTION__);
    return NULL;
  }
  return o->class->name;
}

bool obj_isa (const Object * o, const char * cn) {
  const ObjClass * c = objClass_get (cn);
  if (c == NULL)
    return FALSE;
  return obj_isaP (o, c);
}

bool obj_isaP (const Object * o, const ObjClass * c) {
  ObjClass * t = o->class;
  while (t != NULL) {
    if (t == c) {
      return TRUE;
    }
    t = t->parent;
  }
  return FALSE;
}






bool obj_isChild (const Object * p, const Object * c) {
  Object * t = NULL;
  if (p == NULL || c == NULL) {
    return FALSE;
  }
  t = c->parent;
  while (t != NULL) {
    //printf ("%p->parent: %p\n", t, t->parent);
    if (t == p) {
      return TRUE;
    }
    t = t->parent;
  }
  return FALSE;
}

bool obj_areSiblings (const Object * i, const Object * j) {
#ifdef OBJECT_VERIFY
  unsigned short c = 0;
  Object * t = NULL;
#endif /* OBJECT_VERIFY */
  if (i == NULL || j == NULL) {
    fprintf (stderr, "%s (%p, %p): received NULL object.\n", __FUNCTION__, i, j);
    return FALSE;
  }
  if (i->parent == NULL || j->parent == NULL) {
    return FALSE;
  }
#ifdef OBJECT_VERIFY
  t = i->parent->firstChild;
  while (t != NULL) {
    //printf ("%p, looking for %p) %p->nextSibling: %p\n", e, f, t, t->nextSibling);
    if (t == i) {
      c += 2;
    } else if (t == f) {
      c += 3;
    }
    t = t->nextSibling;
  }
  switch (c) {
    case 5:
      return TRUE;
    case 2:
    case 3:
      return FALSE;
    case 0:
      fprintf (stderr, "Malformed entity hierarchy: %p is a child of %p, but isn't in its list of children.\n", i, i->parent);
      exit (1);
    default:
      fprintf (stderr, "Malformed entity hierarchy: duplicate entries of %p or %p in %p's list of children\n", i, j, i->parent);
      exit (1);
  }
  return FALSE;
#else /* OBJECT_VERIFY */
  return (i->parent == j->parent)
    ? TRUE
    : FALSE;
#endif /* OBJECT_VERIFY */
}

int obj_childCount (const Object * o) {
  Object * t = NULL;
  int i = 0;
  if (o == NULL) {
    fprintf (stderr, "%s (%p): received NULL object.\n", __FUNCTION__, o);
    return -1;
  }
  t = o->firstChild;
  while (t != NULL) {
    i++;
    t = t->nextSibling;
  }
  return i;
}

int obj_siblingCount (const Object * o) {
  if (o == NULL) {
    fprintf (stderr, "%s (%p): received NULL object.\n", __FUNCTION__, o);
    return -1;
  } else if (o->parent == NULL) {
    return 0;
  }
  return obj_childCount (o->parent) - 1;
}


bool obj_addClassData (Object * o, const char * c, void * d) {
  struct objData * od = xph_alloc (sizeof (struct objData));
  od->ref = objClass_get (c);
  od->data = d;
  if (od->ref == NULL) {
    fprintf (stderr, "%s: Can't set data for non-existant class \"%s\".", __FUNCTION__, c);
    return FALSE;
  }
  dynarr_push (o->objData, od);
  dynarr_sort (o->objData, objData_sort);
  return TRUE;
}

void * obj_getClassData (Object * o, const char * c) {
  struct objData * od = NULL;
  if (o == NULL || c == NULL) {
    return NULL;
  }
  od = *(struct objData **)dynarr_search (o->objData, objData_search, c);
  if (od == NULL) {
    return NULL;
  }
  return od->data;
}

bool obj_rmClassData (Object * o, const char * c) {
  struct objData * od = *(struct objData **)dynarr_search (o->objData, objData_search, c);
  int index;
  if (od == NULL) {
    return FALSE;
  }
  index = in_dynarr (o->objData, od);
  if (index >= 0) {
    dynarr_unset (o->objData, index);
    dynarr_condense (o->objData);
  }
  xph_free (od);
  return TRUE;
}



/**
 *  OBJECT MESSAGING FUNCTIONS
 */



int obj_message (Object * o, objMsg msg, void * a, void * b) {
  int r = 0;
  if (o == NULL || o->class == NULL) {
    fprintf (stderr, "%s (%p ...): recieved NULL object or object with NULL class.\n", __FUNCTION__, o);
    return 0;
  }
  if (ObjectMessageCallstack == NULL) {
    ObjectMessageCallstack = dynarr_create (4, sizeof (struct objCall *));
  }
  obj_pushMessageCallstack (o, __FUNCTION__, msg, a, b);
  r = o->class->handler (o, msg, a, b);
  obj_popMessageCallstack (o);
  return r;
}

int obj_messageSiblings (Object * o, objMsg msg, void * a, void * b) {
  return obj_messageBACKEND (o, __FUNCTION__, msg, a, b);
}

int obj_messageChildren (Object * o, objMsg msg, void * a, void * b) {
  return obj_messageBACKEND (o, __FUNCTION__, msg, a, b);
}

int obj_messageParents (Object * o, objMsg msg, void * a, void * b) {
  return obj_messageBACKEND (o, __FUNCTION__, msg, a, b);
}

int obj_messagePre (Object * o, objMsg msg, void * a, void * b) {
  return obj_messageBACKEND (o, __FUNCTION__, msg, a, b);
}

int obj_messageIn (Object * o, objMsg msg, void * a, void * b) {
  return obj_messageBACKEND (o, __FUNCTION__, msg, a, b);
}

int obj_messagePost (Object * o, objMsg msg, void * a, void * b) {
  return obj_messageBACKEND (o, __FUNCTION__, msg, a, b);
}


static void obj_recordMessageChildren (Dynarr v, Object * o) {
  Object * t = NULL;
  if (o == NULL) {
    return;
  }
  t = o->firstChild;
  while (t != NULL) {
    dynarr_push (v, t);
    t = t->nextSibling;
  }
}

static void obj_recordMessageSiblings (Dynarr v, Object * o) {
  Object * t = NULL;
  if (o == NULL || o->parent == NULL) {
    return;
  }
  t = o->parent->firstChild;
  while (t != NULL) {
    if (t != o) {
      dynarr_push (v, t);
    }
    t = t->nextSibling;
  }
}

static void obj_recordMessageParents (Dynarr v, Object * o) {
  Object * t = NULL;
  if (o == NULL) {
    return;
  }
  t = o->parent;
  while (t != NULL) {
    dynarr_push (v, t);
    t = t->parent;
  }
}

// TODO: this is not LISP, recursion is not the way to go here. (this goes for
//  all of the record functions below here)
static void obj_recordMessagePre (Dynarr v, Object * o) {
  if (o == NULL) {
    return;
  }
  dynarr_push (v, o);
  obj_recordMessagePre (v, o->firstChild);
  obj_recordMessagePre (v, o->nextSibling);
}

static void obj_recordMessageIn (Dynarr v, Object * o) {
  if (o == NULL) {
    return;
  }
  obj_recordMessageIn (v, o->firstChild);
  dynarr_push (v, o);
  obj_recordMessageIn (v, o->nextSibling);
}

static void obj_recordMessagePost (Dynarr v, Object * o) {
  if (o == NULL) {
    return;
  }
  obj_recordMessagePost (v, o->nextSibling);
  obj_recordMessagePost (v, o->firstChild);
  dynarr_push (v, o);
}


static bool objPassState = TRUE;
bool objPassEnable (bool o)
{
	objPassState = o;
	return objPassState;
}
static bool objPassEnabled ()
{
	return objPassState;
}

// IMPORTANT: THESE FUNCTIONS ARE COMMENTED OUT TO AVOID A BUNCH OF USELESS MEMORY THRESHING. IF THE CALLSTACK -- OR obj_pass() -- IS EVER ACTUALLY USED, THESE ARE VERY IMPORTANT. BUT SINCE THEY ARE NOT RIGHT NOW AND PROBABLY NEVER WILL BE, THEY DO NOTHING.
static void obj_pushMessageCallstack (void * o, const char * func, objMsg msg, void * a, void * b) {
  struct objCall * n = xph_alloc (sizeof (struct objCall));
  int len = strlen (func);
  n->objectOrClass = o;
  n->function = xph_alloc_name (len + 1, "struct objCall->function");
  strncpy (n->function, func, len + 1);
  n->msg = msg;
  n->a = a;
  n->b = b;
  n->stage = OC_INHANDLER;
  if (ObjectMessageCallstack == NULL) {
    ObjectMessageCallstack = dynarr_create (4, sizeof (struct objCall *));
  }
	if (dynarr_size (ObjectMessageCallstack) >= 32)
	{
		fprintf (stderr, "Callstack hit 32 entries. I'm going to guess this is an infinite recursion that I should put a stop to.");
		abort ();
	}
  dynarr_push (ObjectMessageCallstack, n);
}

static void obj_setCallType (enum object_calls t) {
  struct objCall * c = NULL;
  if (ObjectMessageCallstack == NULL ||
      dynarr_isEmpty (ObjectMessageCallstack)) {
    // let's just pretend this never happened, okay?
    return;
  }
  c = *(struct objCall **)dynarr_back (ObjectMessageCallstack);
  c->stage = t;
}

static void obj_popMessageCallstack (void * o) {
  struct objCall * c = NULL;
  c = *(struct objCall **)dynarr_pop (ObjectMessageCallstack);
  if (c->objectOrClass != o) {
    // FIXME: this is a random exit code. we don't even have to exit here, just set a THE OBJECT CALLSTACK HAS BEEN RUINED flag and hope for the best. ...I guess.
    exit (12);
  }
  xph_free (c->function);
  xph_free (c);
}

static int obj_messageBACKEND (Object * o, const char * func, objMsg msg, void * a, void * b) {
  Dynarr
    v = dynarr_create (4, sizeof (Object *)),
    p = NULL;
  Object * t = NULL;
  int
    i = 0,
    r = 0;

  // TODO: make a fancy macro wrapper for this that actually hashes the
  // strings and then makes a real switch out of this using the string hash
  // values (so all the string comparison takes place at compile time, instead
  // of every time we send a message)
  if (strcmp (func, "obj_messageSiblings") == 0) {
    obj_recordMessageSiblings (v, o);
  } else if (strcmp (func, "obj_messageChildren") == 0) {
    obj_recordMessageChildren (v, o);
  } else if (strcmp (func, "obj_messageParents") == 0) {
    obj_recordMessageParents (v, o);
  } else if (strcmp (func, "obj_messagePre") == 0) {
    obj_recordMessagePre (v, o);
  } else if (strcmp (func, "obj_messageIn") == 0) {
    obj_recordMessageIn (v, o);
  } else if (strcmp (func, "obj_messagePost") == 0) {
    obj_recordMessagePost (v, o);
  } else {
    fprintf (stderr, "%s: called from an invalid function, \"%s\"\n", __FUNCTION__, func);
    return EXIT_FAILURE;
  }

  //printf ("vector: %d indices filled, %d capacity\n", vector_size (v), vector_capacity (v));
  while ((t = *(Object **)dynarr_at (v, i++)) != NULL) {
    //printf ("got #%d (of %d/%d), %p\n", i, vector_size (v), vector_capacity (v), t);
    //printf ("%s: messaging object %p (%d)\n", __FUNCTION__, t, i);
    r = obj_message (t, msg, a, b);
    //printf ("...done (%d)\n", i);
    if (ObjectSkipChildren == TRUE) {
      //printf ("okay, object %p chose to skip children (at offset %d)\n", t, i);
      //printf ("omg we're in the skip children check\n");
      p = dynarr_create (4, sizeof (Object *));
      dynarr_push (p, t);
      //printf ("got a vector with one entry: %p\n", t);
      while ((t = *(Object **)dynarr_at (v, i++)) != NULL && in_dynarr (p, t->parent) >= 0) {
        //printf ("continuing to loop (%d): current object is %p w/ parent %p\n", i, t, t->parent);
        //printf ("  %p is in the vector at offset %d\n", t->parent, in_vector (p, &t->parent));
        dynarr_push (p, t);
      }
      //printf ("we're finished looping (on object %p w/ parent %p)\n", t, t == NULL ? NULL : t->parent);
      dynarr_destroy (p);
      p = NULL;
      ObjectSkipChildren = FALSE;
    }
    if (ObjectHaltMessaging == TRUE) {
      break;
    }
  }
  //printf ("%s: done.\n", __FUNCTION__);
  ObjectHaltMessaging = FALSE;

  dynarr_destroy (v);
  return r;
}

int obj_pass () {
	if (objPassEnabled () == FALSE)
		return EXIT_FAILURE;
  //printf ("%s: OH GOD, WE'RE IN OBJ_PASS()\n", __FUNCTION__);
  int
    i = 0,
    r = 0;
  struct objCall
    * lastCall = NULL,
    * base = NULL;
  ObjClass
    * try = NULL;
  if (
    ObjectMessageCallstack == NULL ||
    (i = dynarr_size (ObjectMessageCallstack)) == 0) {
    //fprintf (stderr, "Don't call %s outside of an object handler >:(\n", __FUNCTION__);
    return -1;
  }
  lastCall = *(struct objCall **)dynarr_at (ObjectMessageCallstack, i - 1);
  while ((base = *(struct objCall **)dynarr_at (ObjectMessageCallstack, --i))->stage == OC_PASS) {
    if (i == 0) {
      fprintf (stderr, "%s: the callstack is in a bad state: all registered calls are pass invocations, which should be impossible (but apparently isn't). we're going to blow everything up in the hopes you notice this, sorry.\n", __FUNCTION__);
      exit (1);
    }
  }
  if (base == lastCall) {
    try = ((Object *)(base->objectOrClass))->class->parent;
  } else {
    try = ((ObjClass *)(lastCall->objectOrClass))->parent;
  }
  if (try == NULL) {
    //printf ("Nowhere to pass to.\n");
    return EXIT_FAILURE;
  }
  obj_pushMessageCallstack (try, base->function, base->msg, base->a, base->b);
  obj_setCallType (OC_PASS);
  r = try->handler (base->objectOrClass, base->msg, base->a, base->b);
  obj_popMessageCallstack (try);
  return r;
}

int obj_halt () {
  ObjectHaltMessaging = TRUE;
  return 0;
}

int obj_skipchildren () {
  ObjectSkipChildren = TRUE;
  return 0;
}
