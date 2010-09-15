#include "component_debug.h"

int component_name_obj_func (Object * o, objMsg msg, void * a, void * b) {
  switch (msg) {
    case OM_CLSNAME:
      strncpy (a, "COMPONENT_NAME", 32);
      return EXIT_SUCCESS;
    case OM_CLSINIT:
    case OM_CLSFREE:
    case OM_CLSVARS:
    case OM_CREATE:
      return EXIT_FAILURE;
    default:
      break;
  }
  switch (msg) {
    default:
      return obj_pass ();
  }
  return EXIT_FAILURE;
}


int component_name_2_obj_func (Object * o, objMsg msg, void * a, void * b) {
  switch (msg) {
    case OM_CLSNAME:
      strncpy (a, "COMPONENT_NAME_2", 32);
      return EXIT_SUCCESS;
    case OM_CLSINIT:
    case OM_CLSFREE:
    case OM_CLSVARS:
    case OM_CREATE:
      return EXIT_FAILURE;
    default:
      break;
  }
  switch (msg) {
    default:
      return obj_pass ();
  }
  return EXIT_FAILURE;
}

int component_message_tracker_obj_func (Object * o, objMsg msg, void * a, void * b) {
  struct comp_message * comp_msg = NULL;
  char * message = NULL;
  Component
    * from = NULL,
    * to = NULL/*,
    * debug = NULL*/;
  switch (msg) {
    case OM_CLSNAME:
      strncpy (a, "COMPONENT_MESSAGE_TRACKER", 32);
      return EXIT_SUCCESS;
    case OM_CLSINIT:
    case OM_CLSFREE:
    case OM_CLSVARS:
    case OM_CREATE:
      return EXIT_FAILURE;
    default:
      break;
  }
  switch (msg) {

    // arguments: (void **, NULL) [we want to allocate component data to *a]
    case OM_COMPONENT_INIT_DATA:
      break;

    // arguments: (void **, NULL) [we want to free component data from *a]
    case OM_COMPONENT_DESTROY_DATA:
      break;

    // arguments: (struct comp_message *, optional void * ptr dependant on message)
    case OM_COMPONENT_RECEIVE_MESSAGE:
      comp_msg = a;
      from = comp_msg->from;
      to = comp_msg->to;
      message = comp_msg->message;

      break;

    // arguments: (Component * from, char * message)
    case OM_SYSTEM_RECEIVE_MESSAGE:
      from = a;
      message = b;

      break;

    // arguments: (NULL, NULL)
    case OM_UPDATE:
      break;

    default:
      return obj_pass ();
  }
  return EXIT_FAILURE;
}
