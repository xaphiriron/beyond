#include "component_input.h"

#include "system.h"

struct input
{
	SDL_Event
		event;
	Uint8
		* keystate;

	Vector
		* keysPressed,		// (SDLKey) all keys currently pressed, in order of
							// their press (i.e., the most recently pressed
							// key at the highest index, the oldest pressed
							// key at index 0)
		* controlMap,		// (struct input_keys *) which keys are mapped to
							// which commands. test with input_controlActive
							// ({CONTROL})
		* activeEvents,		// (enum input_responses) commands which have been
							// triggered by a keydown that haven't yet had
							// their key come back up (e.g., holding down a
							// key to move)
		* overriddenEvents,	// (enum input_responses) commands which have been
							// triggered, but have stopped due to a key
							// collision (e.g., a command triggered by X when
							// Shift+X is held down)
		* controlledEntities,	// entities to message with CONTROL_INPUT
		* focusedEntities;		// entities to message with FOCUS_INPUT
};

static INPUT Input = NULL;

int keysPressed (const struct input_keys * k) {
  int
    max = 0,
    len = 0,
    hit = 0,
    i = 0;
  SDLKey pressed = 0;
  const struct input_keys * t = k;
  //printf ("%s: ( ", __FUNCTION__);
  while (t != NULL) {
    //printf ("%d ", t->key);
    len++;
    t = t->next;
  }
  //printf (") vs ( ");
  t = k;
  while (i < vector_size (Input->keysPressed)) {
    vector_at (pressed, Input->keysPressed, i);
    //printf ("%d ", pressed);
    while (t != NULL) {
      if (t->key == pressed) {
        max = i;
        hit++;
        break;
      }
      t = t->next;
    }
    i++;
    t = k;
  }
  //printf ("); returning %d (hit: %d, len: %d)\n", hit == len ? max+1 : 0, hit, len);
  return hit == len ? max+1 : 0;
}

bool input_controlActive (const enum input_responses ir)
{
	struct input_keys
		* k;
	if (Input == NULL)
		return FALSE;
	vector_at (k, Input->controlMap, ir);
	if (k == NULL)
		return FALSE;
	return keysPressed (k) > 0
		? TRUE
		: FALSE;
}


struct input * input_create ()
{
	struct input * i = xph_alloc (sizeof (struct input));
	i->keysPressed = vector_create (4, sizeof (SDLKey));
	i->controlMap = vector_create (1, sizeof (struct input_keys *));
	i->activeEvents = vector_create (4, sizeof (enum input_responses));
	i->overriddenEvents = vector_create (4, sizeof (enum input_responses));
	i->controlledEntities = vector_create (2, sizeof (Entity *));
	i->focusedEntities = vector_create (2, sizeof (Entity *));

	// fill out the control map here or elsewhere (probably elsewhere in the config/defaults function), but it needs to be populated with the actual key data before any SDL events are checked.
	vector_assign (i->controlMap, IR_QUIT, keys_create (1, SDLK_ESCAPE));
	vector_assign (i->controlMap, IR_AVATAR_MOVE_LEFT, keys_create (1, SDLK_LEFT));
	vector_assign (i->controlMap, IR_AVATAR_MOVE_RIGHT, keys_create (1, SDLK_RIGHT));
	vector_assign (i->controlMap, IR_AVATAR_MOVE_FORWARD, keys_create (1, SDLK_UP));
	vector_assign (i->controlMap, IR_AVATAR_MOVE_BACKWARD, keys_create (1, SDLK_DOWN));
	return i;
}

void input_destroy (struct input * i)
{
	vector_wipe (i->keysPressed, xph_free);
	vector_wipe (i->controlMap, (void (*)(void *))keys_destroy);
/*
	vector_wipe (i->activeEvents, (void (*)(void *))input_destroyEvent);
	vector_wipe (i->overriddenEvents, (void (*)(void *))input_destroyEvent);
*/
	vector_destroy (i->keysPressed);
	vector_destroy (i->controlMap);
	vector_destroy (i->activeEvents);
	vector_destroy (i->overriddenEvents);
	vector_destroy (i->controlledEntities);
	vector_destroy (i->focusedEntities);
	xph_free (i);
}

struct input_keys * keys_create (int n, ...) {
  struct input_keys
    * root = NULL,
    * t = NULL,
    * u = NULL;
  SDLKey k = 0;
  va_list args;
  va_start (args, n);
  while (n > 0) {
    u = xph_alloc (sizeof (struct input_keys));
    k = (SDLKey)va_arg (args, unsigned int);
    u->key = k;
    //printf ("(got %d/%d)\n", k, u->key);
    if (root == NULL) {
      root = u;
    } else if (t != NULL) {
      t->next = u;
    }
    t = u;
    u = NULL;
    n--;
  }
  va_end (args);
  return root;
}

void keys_destroy (struct input_keys * k) {
  if (k->next != NULL) {
    keys_destroy (k->next);
  }
  xph_free (k);
}

bool input_addEntity (Entity e, enum input_control_types t)
{
	assert (Input != NULL);
	if (t == INPUT_CONTROLLED)
	{
		if (in_vector (Input->controlledEntities, &e) == -1)
			vector_push_back (Input->controlledEntities, e);
		return TRUE;
	}
	else if (t == INPUT_FOCUSED)
	{
		if (in_vector (Input->focusedEntities, &e) == -1)
			vector_push_back (Input->focusedEntities, e);
		return TRUE;
	}
	return FALSE;
}

bool input_rmEntity (Entity e, enum input_control_types t)
{
	assert (Input != NULL);
	if (t == INPUT_CONTROLLED)
	{
		vector_remove (Input->controlledEntities, e);
		return TRUE;
	}
	else if (t == INPUT_FOCUSED)
	{
		vector_remove (Input->focusedEntities, e);
		return TRUE;
	}
	return FALSE;
}


void input_sendGameEventMessage (const struct input_event * ie) {
	int i = 0;
	Entity e = NULL;
	Component c = NULL;
	// CATCH AND HANDLE EVENTS THAT HAVE SYSTEM-WIDE REPERCUSSIONS
	switch (ie->ir)
	{
		case IR_QUIT:
			obj_message (SystemObject, OM_SHUTDOWN, NULL, NULL);
			break;
		default:
			break;
	}
	// SEND MESSAGES OFF TO WORLD ENTITIES
	i = 0;
	while (vector_at (e, Input->controlledEntities, i++) != NULL)
	{
		c = entity_getAs (e, "input");
		if (c == NULL)
			continue;
		component_messageEntity (c, "CONTROL_INPUT", (void *)ie);
	}
	i = 0;
	while (vector_at (e, Input->focusedEntities, i++) != NULL)
	{
		c = entity_getAs (e, "input");
		if (c == NULL)
			continue;
		component_messageEntity (c, "FOCUS_INPUT", (void *)ie);
	}
}



void input_update (Object * d)
{
	int
		i,
		activeFinal,
		overriddenFinal;
	enum input_responses
		command = 0;
	struct input_event
		input_event;
	struct input_keys
		* keys;
	while (SDL_PollEvent (&Input->event))
	{
		input_event.ir = IR_NOTHING;
		input_event.event = &Input->event;
		switch (Input->event.type)
		{
			case SDL_KEYDOWN:
				if (in_vector (Input->keysPressed, &Input->event.key.keysym.sym) >= 0)
					continue;
				vector_push_back (Input->keysPressed, Input->event.key.keysym.sym);
				command = 0;
				activeFinal = vector_size (Input->activeEvents) - 1;
				overriddenFinal = vector_size (Input->overriddenEvents) - 1;
				// - inactive commands iterate to see if they've been activated
				while (command <= vector_index_last (Input->controlMap))
				{
					vector_at (keys, Input->controlMap, command);
					if (keys == NULL)
					{
						command++;
						continue;
					}
					if (input_controlActive (command) && in_vector (Input->activeEvents, &command) < 0)
					{
						//printf ("pushing %d onto activeevents\n", command);
						vector_push_back (Input->activeEvents, command);
					}
					command++;
				}
				// - active commands iterate to see if they've been overridden
				
				// - overridden commands send ~response messages
				i = overriddenFinal;
				while (++i < vector_size (Input->overriddenEvents))
				{
					vector_at (input_event.ir, Input->overriddenEvents, i);
					input_event.ir = ~input_event.ir;
					input_sendGameEventMessage (&input_event);
				}
				
				// - activating commands send response messages
				i = activeFinal;
				while (++i < vector_size (Input->activeEvents))
				{
					vector_at (input_event.ir, Input->activeEvents, i);
					//printf ("sending game message from keydown (%d)\n", input_event.ir);
					input_sendGameEventMessage (&input_event);
				}
				break;
			case SDL_KEYUP:
				if (in_vector (Input->keysPressed, &Input->event.key.keysym.sym) < 0)
					continue;
				vector_remove (Input->keysPressed, Input->event.key.keysym.sym);
				
				// - active commands iterate to see if they've been deactivated
				i = 0;
				while (i < vector_size (Input->activeEvents))
				{
					vector_at (command, Input->activeEvents, i);
					//printf ("keyup: is %d active?\n", command);
					if (!input_controlActive (command))
					{
						vector_erase (Input->activeEvents, i);
						// - deactivating commands send ~response messages
						//printf ("sending game message from keyup (~%d)\n", command);
						input_event.ir = ~command;
						input_sendGameEventMessage (&input_event);
						continue;
					}
					i++;
				}
				// - overridden commands iterate to see if they've stopped being overridden or if they can be dropped entirely
				// - activating commands send response messages
				
				break;
			case SDL_ACTIVEEVENT:
				if (Input->event.active.state & SDL_APPINPUTFOCUS)
				{
					SDL_ShowCursor (Input->event.active.gain ? SDL_DISABLE : SDL_ENABLE);
					while (vector_pop_back (command, Input->activeEvents) != IR_NOTHING)
					{
						input_event.ir = ~command;
						input_sendGameEventMessage (&input_event);
					}
					vector_clear (Input->activeEvents);
					vector_clear (Input->overriddenEvents);
					vector_clear (Input->keysPressed);
				}
				break;
			default:
				if ((SDL_GetAppState () & (SDL_APPMOUSEFOCUS | SDL_APPINPUTFOCUS)) == (SDL_APPMOUSEFOCUS | SDL_APPINPUTFOCUS))
				{
					input_event.ir = IR_NOTHING;
					input_sendGameEventMessage (&input_event);
				}
				break;
		}
	}
}


int component_input (Object * o, objMsg msg, void * a, void * b) {
  Component t = NULL;
  char * comp_msg = NULL;
  switch (msg) {
    case OM_CLSNAME:
      strncpy (a, "input", 32);
      return EXIT_SUCCESS;
    case OM_CLSINIT:
      Input = input_create ();
      return EXIT_SUCCESS;
    case OM_CLSFREE:
      input_destroy (Input);
      Input = NULL;
      return EXIT_SUCCESS;
    case OM_CLSVARS:
      return EXIT_FAILURE;
    case OM_CREATE:
      return EXIT_SUCCESS;
    default:
      break;
  }

  switch (msg) {
    case OM_SHUTDOWN:
    case OM_DESTROY:
      obj_destroy (o);
      return EXIT_SUCCESS;

    case OM_UPDATE:
      input_update (o);
      return EXIT_SUCCESS;
    case OM_COMPONENT_RECEIVE_MESSAGE:
      t = ((struct comp_message *)a)->to;
      comp_msg = ((struct comp_message *)a)->message;
      return EXIT_FAILURE;
    default:
      return obj_pass ();
  }
  return EXIT_FAILURE;
}
