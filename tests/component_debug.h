#ifndef XPH_COMPONENT_DEBUG_H
#define XPH_COMPONENT_DEBUG_H

#include "../src/object.h"
#include "../src/entity.h"

struct dummy_struct {
	char * lastMessage;
};

int component_name_obj_func (Object * o, objMsg msg, void * a, void * b);
int component_name_2_obj_func (Object * o, objMsg msg, void * a, void * b);

int component_message_tracker_obj_func (Object * o, objMsg msg, void * a, void * b);

bool debugComponent_messageReceived (Component c, char * message);

#endif /* XPH_COMPONENT_DEBUG_H */