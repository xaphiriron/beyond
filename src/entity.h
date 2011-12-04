#ifndef XPH_ENTITY_H
#define XPH_ENTITY_H

#include <stdarg.h>
#include <stdbool.h>
#include "timer.h"
#include "dynarr.h"
#include "object.h"

#define COMPNAMELENGTH 32

typedef struct entity * Entity;
typedef struct ent_system * EntSystem;
typedef struct ent_component * EntComponent;

typedef void (compFunc) (EntComponent, void *);
typedef void (sysFunc) (const Dynarr);

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


/***
 * COMPONENTS
 */

bool component_register (const char * comp_name, objHandler objFunc, compFunc classInit);
void component_destroy (const char * comp_name);

bool component_instantiate (const char * comp_name, Entity e);
bool component_remove (const char * comp_name, Entity e);

EntComponent entity_getAs (Entity e, const char * comp_name);
Entity component_entityAttached (EntComponent c);

bool component_setData (EntComponent c, void * data);
void * component_getData (EntComponent c);
bool component_clearData (EntComponent c);

bool component_registerResponse (const char * comp_name, const char * message, compFunc * function);
bool component_clearResponses (const char * comp_name, const char * message);

Dynarr entity_getEntitiesWithComponent (int n, ...);


/***
 * ENTITY SYSTEM
 */

void entity_purgeDestroyed (TIMER t);

void entity_destroyEverything ();


bool entitySubsystem_store (const char * comp_name);
bool entitySubsystem_unstore (const char * comp_name);
bool entitySubsystem_runOnStored (objMsg);
void entitySubsystem_clearStored ();



#endif /* XPH_ENTITY_H */
