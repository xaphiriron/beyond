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
	case OM_COMPONENT_INIT_DATA:
		if (entity_getAs (b, "COMPONENT_MESSAGE_TRACKER") == NULL)
			component_instantiateOnEntity ("COMPONENT_MESSAGE_TRACKER", b);
		obj_pass ();
		return EXIT_SUCCESS;
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
	case OM_COMPONENT_INIT_DATA:
		if (entity_getAs (b, "COMPONENT_MESSAGE_TRACKER") == NULL)
			component_instantiateOnEntity ("COMPONENT_MESSAGE_TRACKER", b);
		obj_pass ();
		return EXIT_SUCCESS;
    default:
      return obj_pass ();
  }
  return EXIT_FAILURE;
}

int component_message_tracker_obj_func (Object * o, objMsg msg, void * a, void * b) {
  struct comp_message * comp_msg = NULL;
  char * message = NULL;
  Component
    from = NULL,
    to = NULL/*,
    * debug = NULL*/;
	struct dummy_struct
		* data = NULL;
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
	switch (msg)
	{
		// arguments: (void **, Entity) [we want to allocate component data to *a (it's &Component->comp_data)]
		case OM_COMPONENT_INIT_DATA:
			data = xph_alloc (sizeof (struct dummy_struct));
			*(void **)a = data;
			data->lastMessage = NULL;
			
			break;

    // arguments: (void **, Entity) [we want to free component data from *a] (it's &Component->comp_data)
    case OM_COMPONENT_DESTROY_DATA:
			data = *(void **)a;
			if (data->lastMessage != NULL)
				xph_free (data->lastMessage);
			xph_free (data);
			*(void **)a = NULL;
      break;

    // arguments: (struct comp_message *, optional void * ptr dependant on message)
    case OM_COMPONENT_RECEIVE_MESSAGE:
      comp_msg = a;
      from = comp_msg->from;
      to = comp_msg->to;
      message = comp_msg->message;
			data = *(void **)a;
			if (data->lastMessage != NULL)
			{
				xph_free (data->lastMessage);
			}
			data->lastMessage = xph_alloc (strlen (message) + 1);
			printf ("setting lastMessage to \"%s\"", message);
			strcpy (data->lastMessage, message);

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

bool debugComponent_messageReceived (Component c, char * message) {
	struct dummy_struct
		* data;
	Entity
		e = component_entityAttached (c);
	Component
		tracker = entity_getAs (e, "COMPONENT_MESSAGE_TRACKER");
	if (tracker == NULL)
		return FALSE;
	data = component_getData (tracker);
	if (data->lastMessage == NULL)
		return FALSE;
	printf ("data: %p; lastMessage: \"%s\"\n", data, data->lastMessage);
	return (strcmp (data->lastMessage, message) == 0);
}
