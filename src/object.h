#ifndef XPH_OBJECT_H
#define XPH_OBJECT_H

#include "xph_memory.h"
#include "cpv.h"

typedef enum object_messages {
  OM_CLSNAME = 1,
  OM_CLSINIT,
  OM_CLSFREE,
  OM_CLSVARS,

  OM_CREATE,
  OM_START,
  OM_SUSPEND,
  OM_SHUTDOWN,
  OM_DESTROY,

  OM_UPDATE,
  OM_POSTUPDATE,
  OM_INPUT,

  OM_PRERENDER,
  OM_RENDER,
  OM_POSTRENDER,

  OM_COMPONENT_INIT_DATA,
  OM_COMPONENT_DESTROY_DATA,
  OM_COMPONENT_RECEIVE_MESSAGE,
  OM_SYSTEM_RECEIVE_MESSAGE,

  OM_FINAL
} objMsg;

typedef struct object {
  struct objClass * class;
  Vector * objData;
  unsigned int guid;

  struct object
    * parent,
    * firstChild,
    * nextSibling;
} Object;

typedef int (objHandler)(Object *, objMsg, void *, void *);

typedef struct objClass {
  char name[32];
  objHandler * handler;
  int instances;

  struct objClass
    * parent,
    * firstChild,
    * nextSibling;
} ObjClass;

struct objData {
  ObjClass * ref;
  void * data;
};

enum object_calls {
  OC_INHANDLER = 1,
  OC_STARTED,
  OC_COMPLETED,
  OC_PASS
};

struct objCall {
  void * objectOrClass;	// Object * normally, ObjClass * iff stage == OC_PASS
  objMsg msg;
  void
    * a,
    * b;
  char * function;
  enum object_calls stage;
};

void objects_destroyEverything ();

ObjClass * objClass_init (objHandler h, const char * pn, void * a, void * b);
ObjClass * objClass_get (const char * n);
bool objClass_destroy (const char * n);
bool objClass_destroyP (ObjClass *);

bool objClass_addChild (ObjClass * p, ObjClass * c);
bool objClass_rmChild (ObjClass * p, ObjClass * c);

bool objClass_inheritsP (ObjClass * p, ObjClass * c);
bool objClass_inherits (const char * pn, const char * cn);

bool objClass_chparentP (ObjClass * p, ObjClass * c);
bool objClass_chparent (const char * pn, const char * cn);


Object * obj_create (const char * c, Object * p, void * a, void * b);
void obj_destroy (Object * o);

bool obj_addChild (Object * p, Object * c);
bool obj_rmChild (Object * p, Object * c);
void obj_chparent (Object * p, Object * c);

const char * obj_getClassName (const Object * o);

bool obj_isa (const Object * o, const char * c);
bool obj_isaP (const Object * o, const ObjClass * c);

bool obj_addClassData (Object * o, const char * c, void * d);
void * obj_getClassData (Object * o, const char * c);
bool obj_rmClassData (Object * o, const char * c);

bool obj_isChild (const Object * p, const Object * c);
bool obj_areSiblings (const Object * i, const Object * j);
int obj_childCount (const Object * o);
int obj_siblingCount (const Object * o);


bool obj_addClassData (Object * o, const char * c, void * d);
void * obj_getClassData (Object * o, const char * c);
bool obj_rmClassData (Object * o, const char * c);

int obj_message (Object * o, objMsg, void *, void *);
int obj_messageSiblings (Object * o, objMsg, void *, void *);
int obj_messageChildren (Object * o, objMsg, void *, void *);
int obj_messageParents (Object * o, objMsg, void *, void *);

int obj_messagePre (Object * o, objMsg, void *, void *);
int obj_messageIn (Object * o, objMsg, void *, void *);
int obj_messagePost (Object * o, objMsg, void *, void *);

int obj_pass ();
int obj_halt ();
int obj_skipchildren ();

#endif /* XPH_OBJECT_H */