#ifndef XPH_OBJECT_DEBUG_H
#define XPH_OBJECT_DEBUG_H

#include "../src/object.h"

enum object_debug_extra_messages {
  OM_MESSAGE = OM_FINAL + 1,
  OM_MESSAGEORDER,
  OM_MESSAGEORDERRESET,
  OM_PASSPRE,
  OM_PASSPOST,
  OM_SKIPONORDERVALUE,
  OM_HALTONORDERVALUE
};

struct object_debug {
  int initialized;
  char * name;

  int messaged;
  int order;
  enum object_debug_pass {
    OBJ_DEBUG_PASS_CHILD = 6,
    OBJ_DEBUG_PASS_PARENT
  } pass;
};

struct object_debug * object_debug_create (int init, char * name);
void object_debug_destroy (struct object_debug * o);

const char * object_debug_name (Object * o);
int object_debug_initialized (Object * o);
int object_debug_messageCount (Object * o);
int object_debug_order (Object * o);
enum object_debug_pass object_debug_pass (Object * o);


int object_debug_handler (Object * o, objMsg msg, void * a, void * b);
int object_debug_child_handler (Object * o, objMsg msg, void * a, void * b);

int object_debug_broken_handler (Object * o, objMsg msg, void * a, void * b);

#endif /* XPH_OBJECT_DEBUG_H */