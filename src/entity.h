
#ifndef ENTITY_H
#define ENTITY_H
#include <assert.h>
#include <stdio.h>
#include "bool.h"
#include "xph_memory.h"
#include "list.h"

typedef enum entityMessages {
  EM_CLSINIT,	// called automatically on class creation with a NULL entity
  EM_CLSFREE,	// called automatically on class destruction with a NULL entity
  EM_CLSNAME,	// strncpy's the class name into the first argument
  EM_CLSVARS,	// (reserved, unused)

  EM_CREATE,	// called automatically on entity creation
  EM_START,	// start processing, usually start handling update messages
  EM_SUSPEND,	// stop processing, usually start ignoring update messages
  EM_UPDATE,
  EM_POSTUPDATE,
  EM_SHUTDOWN,	// destroy entity gracefully
  EM_DESTROY,	// destroy entity instantly

  EM_TESTINPUT,

  EM_PRERENDER,
  EM_RENDER,
  EM_POSTRENDER,

  EM_SETTARGET,
  EM_SETSOURCE,
  EM_SETPLAYER,

  EM_INPUT,	// arguments: (const SDL_Event *, const Uint8 * keystate)
                //  (sdl event is keydown/up)
  // I don't even know if I want to do these: it would require doing picking
  // literally every frame, since the mouse or the game world entities could
  // move around, and that seems like it could be a lot of work. If it's really
  // something that would be useful or something that adds a lot to the feel to
  // have hover effects, then I'll try it, but... it really does seem like it
  // could cause a lot of slowdown, since (iirc) picking requires its own
  // entire screen render, which means it might halve the fps.
  EM_MOUSEOVER,	// arguments: (const SDL_Event *, ...???)
                //  (sdl event is SDL_MOUSEMOTION...? [since this can be triggered by entities moving around on the screen instead of actual mouse movement, there might not always be an applicable sdl event to send.])
  EM_MOUSEAWAY,	// arguments: (const SDL_Event *, ...???)
                //  (sdl event is SDL_MOUSEMOTION...?; see above note wrt sdl events)
  // ...

  EM_LAST	// (reserved, unused, don't use it)

} eMessage;

typedef struct entity {
  struct entityClass * class;
  struct list * entData;
  unsigned int guid;

  struct entity
    * parent,
    * firstChild,
    * nextSibling;
} ENTITY;

typedef int (eHandler)(ENTITY *, eMessage, void *, void *);

typedef struct entityClass {
  char name[32];
  eHandler * handler;
  int instances;

  struct entityClass
    * parent,
    * firstChild,
    * nextSibling;
} ENTCLASS;

struct entData {
  ENTCLASS * ref;
  void * data;
};

struct entMessage {
  ENTITY * e;
  eMessage msg;
  void
   * a,
   * b;
};

enum entcalls {
  EC_INHANDLER = 1,
  EC_STARTED,
  EC_COMPLETED,
  EC_PASS
};

struct entCall {
  ENTITY * e;
  eMessage msg;
  void
    * a,
    * b;
  char * func;
  enum entcalls stage;
};

ENTCLASS * entclass_init (eHandler h, const char * parent, void * a, void * b);
ENTCLASS * entclass_get (const char * c);
bool entclass_destroyS (const char * c);
bool entclass_destroy (ENTCLASS * e);
void entclass_destroyAll ();

bool entclass_chparent (ENTCLASS * c, ENTCLASS * p);
bool entclass_chparentS (char * c, char * p);

bool entclass_inheritsFrom (const ENTCLASS * ecc, const ENTCLASS * ecp);
bool entclass_inheritsFromS (const char * c, const char * p);

bool entclass_addChild (ENTCLASS * p, ENTCLASS * c);
bool entclass_rmChild (ENTCLASS * p, ENTCLASS * c);


ENTITY * entity_create (const char * entclass, ENTITY * p, void * a, void * b);
void entity_destroy (ENTITY * e);

bool entity_addChild (ENTITY * p, ENTITY * c);
bool entity_rmChild (ENTITY * p, ENTITY * c);

bool entity_addClassData (ENTITY * e, const char * c, void * d);
void * entity_getClassData (const ENTITY * e, const char * c);
bool entity_rmClassData (ENTITY * e, const char * c);

bool entity_isa (const ENTITY * e, const ENTCLASS * ec);
bool entity_isaS (const ENTITY * e, const char * c);

bool entity_isChildOf (const ENTITY * c, const ENTITY * p);
bool entity_areSiblings (const ENTITY * e, const ENTITY * f);
int entity_siblingCount (const ENTITY * e);
int entity_childrenCount (const ENTITY * e);
void entity_chparent (ENTITY * e, ENTITY * newp);

int entity_message (ENTITY * e, eMessage msg, void * a, void * b);
int entity_messageSiblings (ENTITY * e, eMessage msg, void * a, void * b);
int entity_messageChildren (ENTITY * e, eMessage msg, void * a, void * b);
int entity_messageParents (ENTITY * e, eMessage msg, void * a, void * b);

int entity_messagePre (ENTITY * e, eMessage msg, void * a, void * b);
int entity_messageIn (ENTITY * e, eMessage msg, void * a, void * b);
int entity_messagePost (ENTITY * e, eMessage msg, void * a, void * b);

struct entMessage * entity_createMessage (ENTITY * e, eMessage msg, void * a, void * b);
void entity_destroyMessage (struct entMessage *);
int entity_sendMessage (const struct entMessage * em);

int entity_pass ();
int entity_skipchildren ();
int entity_halt ();

int entity_getRegister (int r);
void entity_setRegister (int r, int val);
void entity_resetRegisters ();


const ENTITY * entcallstack_activeEntity ();
const ENTCLASS * entcallstack_activeEntclass ();

/*
typedef enum entityMessages {
  EM_CLSINIT,
  EM_CLSFREE,
  EM_CLSNAME,

  EM_CREATE,
  EM_START,
  EM_SHUTDOWN,
  EM_DESTROY,

  // ...

  EM_LAST

} eMessage;

typedef struct entity {
  struct entityClass * class;
  void * data;
  int guid;

  struct entity
    * parent,
    * firstChild,
    * nextSibling;
} ENTITY;

typedef int (eHandler)(ENTITY *, eMessage, void *, void *);

typedef struct entityClass {
  char name[32];
  eHandler * handler;
  struct entityClass * parent;
  int instances;
} ENTCLASS;

ENTITY * entity_create (char * className, struct entity * parent, void * a, void * b);
bool entity_destroy (ENTITY *);

ENTCLASS * entity_createClass (eHandler messagebus, char * parent, void * a, void * b);
bool entity_destroyClass (char * name);

ENTCLASS * entity_getClass (const char * entityClass);
struct list * entity_classParents (const char * n);
void entity_chparentClass (const char * c, const char * p);

int entclass_search (const void *, const void *);
int entclass_sort (const void *, const void *);

char * entity_getClassName (const ENTITY * e);

bool entity_isChildOf (const ENTITY * c, const ENTITY * p);
bool entity_areSiblings (const ENTITY * a, const ENTITY * b);
int entity_childrenCount (const ENTITY * e);
int entity_siblingCount (const ENTITY * e);

void entity_chparent (ENTITY * c, ENTITY * p);

int entity_message (ENTITY * e, eMessage m, void * a, void * b);
int entity_messagePre (ENTITY * e, eMessage m, void * a, void * b);
int entity_messagePost (ENTITY * e, eMessage m, void * a, void * b);
int entity_messageChildren (ENTITY * e, eMessage m, void * a, void * b);
int entity_messageParents (ENTITY * e, eMessage m, void * a, void * b);
int entity_messageSiblings (ENTITY * e, eMessage m, void * a, void * b);

int entity_pass ();

// everything below here is super tentative
*
enum field_format {
  FORMAT_CHAR,
  FORMAT_INT,
  FORMAT_FLOAT,
  FORMAT_DOUBLE,
  FORMAT_STRING,
  FORMAT_POINTER
};

struct entityField {
  char * label;
  ptrdiff_t offset; // entity->data + offset == the start of this field
  size_t size;
  enum field_format format;
};
*/
/*
int entity_message (ENTITY *, eMessage m, int args, ...);

// would these functions even be useful?
// number of entities which are also direct childen of the entity's parent
int entity_siblingCount (const struct entity * e);
// number of entities which are at some remove parented to this entity 
int entity_parentCount (const struct entity * e);
int entity_parentCount (const struct entity * e) {
  return (e->parent == NULL)
    ? 0
    : entity_parentCount (e->parent) + 1;
}

struct attr {
  enum attr_type {
    ATTR_BOOL,
    ATTR_INT,
    ATTR_FLOAT,
    ATTR_DOUBLE,
    ATTR_CHAR,
    ATTR_PTR
  } type;
  union {
    bool b;
    int i;
    float f;
    double d;
    char * s;
    void * p;
  } a;
};

struct attr entity_getAttribute (const struct entity * e, char * attr);

// OR

bool entity_getbAttribute (const struct entity *, char * attr);
int entity_getiAttribute (const struct entity *, char * attr);
float entity_getfAttribute (const struct entity *, char * attr);
double entity_getdAttribute (const struct entity *, char * attr);
char * entity_getAttrS (const struct entity *, char * attr);
char * entity_getStringAttribute (const struct entity *, char * attr);
void * entity_getAttrP (const struct entity *, char * attr);

// pros: you don't have to manipulate an awkward struct just to get the values
// cons: you have to know what type the attr is, so that _getAttributeNames is
//       almost useless.
// further alternatives:
//   entity_getAttr (const struct entity *, char * attrName, void * attr),
//   where attr is a pointer to the data type of the attr. Still requires knowing what the data type of a given attrName is beforehand, and additionally destroys type safety.


bool entity_setAttr (struct entity * e, char * attr, ...);
*/

#endif /* ENTITY_H */