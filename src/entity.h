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
	Entity
		entFrom;
  EntComponent
    from,
    to;
  char * message;
};

/***
 * ENTITIES
 */

Entity entity_create ();
void entity_destroy (Entity e);

unsigned int entity_GUID (const Entity e);

bool entity_exists (unsigned int guid);
Entity entity_get (unsigned int guid);

/***
 * MESSAGING
 */

void entity_subscribe (Entity listener, Entity target);
void entity_unsubscribe (Entity listener, Entity target);

/* vocalize a status change; messages any entities subscribed */
void entity_speak (const Entity speaker, char * message, void * arg);

/* message the entity specified */
bool entity_message (Entity e, Entity from, char * message, void * arg);



void entity_purgeDestroyed (TIMER t);

EntComponent entity_getAs (Entity e, const char * comp_name);

bool entity_registerComponentAndSystem (objHandler func);
Dynarr entity_getEntitiesWithComponent (int n, ...);
EntSystem entity_getSystemByName (const char * comp_name);

void entity_destroySystem (const char * comp_name);
void entity_destroyEverything ();

void * component_getData (EntComponent c);
Entity component_entityAttached (EntComponent c);

bool component_instantiateOnEntity (const char * comp_name, Entity e);
bool component_removeFromEntity (const char * comp_name, Entity e);

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
