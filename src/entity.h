#ifndef XPH_ENTITY_H
#define XPH_ENTITY_H

#include <stdarg.h>
#include "cpv.h"
#include "object.h"

typedef struct entity * Entity;
typedef struct ent_system * System;
typedef struct ent_component * Component;

struct comp_message {
  Component
    from,
    to;
  char * message;
};

Entity entity_create ();
void entity_destroy (Entity e);
bool entity_exists (unsigned int guid);

unsigned int entity_GUID (const Entity e);
Component entity_getAs (Entity e, const char * comp_name);

bool entity_registerComponentAndSystem (objHandler func);
Vector * entity_getEntitiesWithComponent (int n, ...);
System entity_getSystemByName (const char * comp_name);

void entity_destroySystem (const char * comp_name);
void entity_destroyEverything ();

void * component_getData (Component c);
Entity component_entityAttached (Component c);

bool component_instantiateOnEntity (const char * comp_name, Entity e);
bool component_removeFromEntity (const char * comp_name, Entity e);
bool component_messageEntity (Component c, char * message, void * arg);
bool component_messageSystem (Component c, char * message, void * arg);

bool entitySubsystem_store (const char * comp_name);
bool entitySubsystem_unstore (const char * comp_name);
bool entitySubsystem_runOnStored (objMsg);
void entitySubsystem_clearStored ();

#endif /* XPH_ENTITY_H */