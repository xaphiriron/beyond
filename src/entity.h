#ifndef XPH_ENTITY_H
#define XPH_ENTITY_H

#include <stdarg.h>
#include <stdbool.h>
#include "timer.h"
#include "dynarr.h"

typedef struct entity * Entity;
typedef struct ent_component * EntComponent;
typedef struct entity_speech * EntSpeech;

typedef void (compFunc) (EntComponent, EntSpeech);
typedef void (sysFunc) (const Dynarr);

struct entity_speech
{
	Entity
		from;
	EntComponent
		to;
	unsigned int
		fromGUID;
	char
		* message;
	void
		* arg;
	signed int
		references;
};

/***
 * ENTITIES
 */

Entity entity_create ();
void entity_destroy (Entity e);

unsigned int entity_GUID (const Entity e);

bool entity_exists (unsigned int guid);
Entity entity_get (unsigned int guid);

bool entity_name (Entity e, const char * name);
Entity entity_getByName (const char * name);

bool entity_addToGroup (Entity e, const char * groupName);
const Dynarr entity_getGroup (const char * group);
void entity_messageGroup (const char * group, Entity from, char * message, void * arg);

/* call after adding or removing components; this updates the system lists the entity is on - xph 2011 10 27 */
void entity_refresh (Entity e);

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

bool component_register (const char * comp_name, compFunc classInit);
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

Dynarr entity_getWithv (int n, va_list comps);
Dynarr entity_getWith (int n, ...);


/***
 * ENTITY SYSTEM
 */

void entity_purgeDestroyed (TIMER t);
void entity_destroyEverything ();

void entitySystem_register (const char * name, sysFunc updateFunc, int components, ...);

void entitySystem_disableMessages (const char * system);
void entitySystem_message (const char * sys_name, Entity from, const char * message, void * arg);
EntSpeech entitySystem_dequeueMessage (const char * system);

void entitySystem_update (const char * name);


#endif /* XPH_ENTITY_H */
