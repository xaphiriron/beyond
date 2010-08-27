#ifndef XPH_COMPONENT_DEBUG_H
#define XPH_COMPONENT_DEBUG_H

#include "../src/object.h"
#include "../src/entity.h"

int component_name_obj_func (Object * o, objMsg msg, void * a, void * b);
int component_name_2_obj_func (Object * o, objMsg msg, void * a, void * b);

int component_message_tracker_obj_func (Object * o, objMsg msg, void * a, void * b);

#endif /* XPH_COMPONENT_DEBUG_H */