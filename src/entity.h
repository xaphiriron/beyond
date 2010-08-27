#ifndef XPH_ENTITY_H
#define XPH_ENTITY_H

#include <stdarg.h>
#include "cpv.h"
#include "object.h"

typedef struct entity {
  unsigned int guid;

  // this is a local variable to make fetching components from a specific entity faster. It stores the same data as a System->entities vector, which is to say Components (something todo: this is named "components" whereas the system vector is named "entities". this is confusing and dumb.)
  Vector * components;
} Entity;

typedef struct sys_reg {
  Object * system;
  const char * comp_name;
  Vector * entities;
} System;

typedef struct comp_map {
  Entity * e;
  System * reg;
  void * comp_data;
  unsigned int comp_guid;
} Component;

struct comp_message {
  Component
    * from,
    * to;
  char * message;
};


Entity * entity_create ();
void entity_destroy (Entity * e);
bool entity_exists (unsigned int guid);

Component * entity_getAs (Entity * e, const char * comp_name);


bool entity_registerComponentAndSystem (objHandler func);
Vector * entity_getEntitiesWithComponent (int n, ...);
System * entity_getSystemByName (const char * comp_name);

bool component_instantiateOnEntity (const char * comp_name, Entity * e);
bool component_removeFromEntity (const char * comp_name, Entity * e);

bool component_messageEntity (Component * c, char * message);
bool component_messageSystem (Component * c, char * message);

void entitySubsystem_update (const char * comp_name);
void entitySubsystem_postupdate (const char * comp_name);

bool debugComponent_messageReceived (Component * c, char * message);

#endif /* XPH_ENTITY_H */