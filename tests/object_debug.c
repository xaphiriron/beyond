#include "object_debug.h"

static struct object_debug * ObjectDebugDefaults = NULL;
static int MessageOrder = 0;

struct object_debug * object_debug_create (int init, char * name) {
  struct object_debug * o = xph_alloc (sizeof (struct object_debug), "struct object_debug");
  int l = 0 ;
  if (init != 0) {
    o->initialized = init;
  } else {
    o->initialized = 1;
  }
  if (name != NULL) {
    l = strlen (name);
    o->name = xph_alloc (l + 1, "struct object_debug->name");
    strncpy (o->name, name, l + 1);
  } else {
    o->name = xph_alloc (8, "struct object_debug->name");
    strncpy (o->name, "default", 8);
  }
  o->messaged = 0;
  o->order = 0;
  o->pass = 0;
  return o;
}

void object_debug_destroy (struct object_debug * o) {
  xph_free (o->name);
  xph_free (o);
}

const char * object_debug_name (Object * o) {
  struct object_debug * od = obj_getClassData (o, "debug");
  if (od == NULL) {
    printf ("%s: object %p doesn't have debug data\n", __FUNCTION__, o);
    return NULL;
  }
  return od->name;
}

int object_debug_initialized (Object * o) {
  struct object_debug * od = obj_getClassData (o, "debug");
  if (od == NULL) {
    printf ("%s: object %p doesn't have debug data\n", __FUNCTION__, o);
    return -1;
  }
  return od->initialized;
}

int object_debug_messageCount (Object * o) {
  struct object_debug * od = obj_getClassData (o, "debug");
  if (od == NULL) {
    printf ("%s: object %p doesn't have debug data\n", __FUNCTION__, o);
    return -1;
  }
  return od->messaged;
}

int object_debug_order (Object * o) {
  struct object_debug * od = obj_getClassData (o, "debug");
  if (od == NULL) {
    printf ("%s: object %p doesn't have debug data\n", __FUNCTION__, o);
    return -1;
  }
  return od->order;
}

enum object_debug_pass object_debug_pass (Object * o) {
  struct object_debug * od = obj_getClassData (o, "debug");
  if (od == NULL) {
    printf ("%s: object %p doesn't have debug data\n", __FUNCTION__, o);
    return -1;
  }
  return od->pass;
}


int object_debug_handler (Object * o, objMsg msg, void * a, void * b) {
  struct object_debug * od = NULL;
  switch (msg) {
    case OM_CLSNAME:
      strncpy (a, "debug", 32);
      return EXIT_SUCCESS;
    case OM_CLSINIT:
      ObjectDebugDefaults = object_debug_create ((int)a, b);
      MessageOrder = 0;
      return EXIT_SUCCESS;
    case OM_CLSFREE:
      object_debug_destroy (ObjectDebugDefaults);
      ObjectDebugDefaults = NULL;
      return EXIT_SUCCESS;
    case OM_CLSVARS:
      return EXIT_FAILURE;
    case OM_CREATE:
      //printf ("OM_CREATE!!!!\n");
      od = object_debug_create (
        (int)a != 0 ? (int)a : ObjectDebugDefaults->initialized,
        b != NULL ? b : ObjectDebugDefaults->name
      );
      obj_addClassData (o, "debug", od);
      return EXIT_SUCCESS;
    default:
      break;
  }
  od = obj_getClassData (o, "debug");
  switch (msg) {
    case OM_DESTROY:
      object_debug_destroy (od);
      obj_rmClassData (o, "debug");
      obj_destroy (o);
      return EXIT_SUCCESS;

    case OM_MESSAGE:
      od->messaged++;
      return EXIT_SUCCESS;

    case OM_MESSAGEORDER:
      od->order = ++MessageOrder;
      return EXIT_SUCCESS;

    case OM_MESSAGEORDERRESET:
      MessageOrder = 0;
      return EXIT_SUCCESS;

    case OM_PASSPOST:
    case OM_PASSPRE:
      od->pass = OBJ_DEBUG_PASS_PARENT;
      return EXIT_SUCCESS;

    case OM_SKIPONORDERVALUE:
      //printf ("obj. %p doing OM_SKIPONORDERVALUE w/ value %d and count %d(+1)\n", o, (int)a, od->messaged);
      od->messaged++;
      if ((int)a == od->order) {
        obj_skipchildren ();
      }
      return EXIT_SUCCESS;

    case OM_HALTONORDERVALUE:
      //printf ("obj. %p doing OM_HALTONORDERVALUE w/ value %d and count %d(+1)\n", o, (int)a, od->messaged);
      od->messaged++;
      if ((int)a == od->order) {
        obj_halt ();
      }
      return EXIT_SUCCESS;

    default:
      break;
  }
  return EXIT_FAILURE;
}


int object_debug_child_handler (Object * o, objMsg msg, void * a, void * b) {
  struct object_debug * od = NULL;
  switch (msg) {
    case OM_CLSNAME:
      strncpy (a, "debug child", 32);
      return EXIT_SUCCESS;
    case OM_CLSINIT:
    case OM_CLSFREE:
    case OM_CLSVARS:
      return EXIT_FAILURE;
    case OM_CREATE:
      return EXIT_FAILURE;
    default:
      break;
  }
  od = obj_getClassData (o, "debug");
  assert (od != NULL);
  switch (msg) {
    // you really don't want to pass destroy messages, so they're defined even though they don't do anything.
    case OM_SHUTDOWN:
    case OM_DESTROY:
      return EXIT_FAILURE;

    case OM_PASSPOST:
      od->pass = OBJ_DEBUG_PASS_CHILD;
      obj_pass ();
      return EXIT_SUCCESS;

    case OM_PASSPRE:
      obj_pass ();
      od->pass = OBJ_DEBUG_PASS_CHILD;
      return EXIT_SUCCESS;

    default:
      return obj_pass ();
  }
  return EXIT_FAILURE;
}

int object_debug_broken_handler (Object * o, objMsg msg, void * a, void * b) {
  return EXIT_FAILURE;
}