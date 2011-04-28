#ifndef XPH_ENTITY_H
#define XPH_ENTITY_H

#include <stdarg.h>
#include "timer.h"
#include "dynarr.h"
#include "object.h"

typedef struct entity * Entity;
typedef struct ent_system * EntSystem;
typedef struct ent_component * EntComponent;

struct comp_message {
  EntComponent
    from,
    to;
  char * message;
};

Entity entity_create ();
Entity entity_get (unsigned int guid);
void entity_purgeDestroyed (TIMER t);
void entity_destroy (Entity e);
bool entity_exists (unsigned int guid);

bool entity_message (Entity e, char * message, void * arg);

unsigned int entity_GUID (const Entity e);
EntComponent entity_getAs (Entity e, const char * comp_name);

bool entity_registerComponentAndSystem (objHandler func);
Dynarr entity_getEntitiesWithComponent (int n, ...);
EntSystem entity_getSystemByName (const char * comp_name);

void entity_destroySystem (const char * comp_name);
void entity_destroyEverything ();

void * component_getData (EntComponent c);
const char * component_getName (const EntComponent c);
Entity component_entityAttached (EntComponent c);

bool component_instantiateOnEntity (const char * comp_name, Entity e);
bool component_removeFromEntity (const char * comp_name, Entity e);
bool component_messageEntity (EntComponent c, char * message, void * arg);
bool component_messageSystem (EntComponent c, char * message, void * arg);

bool entitySubsystem_message (const char * comp_name, enum object_messages message, void * a, void * b);
bool entitySubsystem_store (const char * comp_name);
bool entitySubsystem_unstore (const char * comp_name);
bool entitySubsystem_runOnStored (objMsg);
void entitySubsystem_clearStored ();



void component_setLoadGoal (EntComponent c, unsigned int m);
void component_updateLoadAmount (EntComponent c, unsigned int v);
void component_setLoadComplete (EntComponent c);
bool component_isFullyLoaded (const EntComponent c);
void component_dropLoad (EntComponent c);

void component_reweigh (EntComponent c);
void component_forceLoaderResort ();

void component_setAsLoadable (EntComponent c);
bool component_isLoaderActive ();
void component_forceRunLoader (unsigned int load);
void component_runLoader (const TIMER t);


typedef void (compFunc) (EntComponent);

bool entitySubsystem_registerMessageResponse (const char * comp_name, const char * message, compFunc * function);
bool entitySubsystem_clearMessageResponses (const char * comp_name, const char * message);
void component_sendMessage (const char * message, EntComponent c);

#endif /* XPH_ENTITY_H */
