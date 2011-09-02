#include "component_input.h"

#include "video.h"
#include "system.h"

#include "component_ui.h"

struct input
{
	SDL_Event
		event;
	Uint8
		* keystate;

	Dynarr
		keysPressed,		// (SDLKey) all keys currently pressed, in order of
							// their press (i.e., the most recently pressed
							// key at the highest index, the oldest pressed
							// key at index 0)
		controlMap,			// (struct input_keys *) which keys are mapped to
							// which commands. test with input_controlActive
							// ({CONTROL})
		activeEvents,		// (enum input_responses) commands which have been
							// triggered by a keydown that haven't yet had
							// their key come back up (e.g., holding down a
							// key to move)
		overriddenEvents,	// (enum input_responses) commands which have been
							// triggered, but have stopped due to a key
							// collision (e.g., a command triggered by X when
							// Shift+X is held down)
		controlledEntities,	// entities to message with CONTROL_INPUT
		focusedEntities;	// entities to message with FOCUS_INPUT
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
  while (i < dynarr_size (Input->keysPressed)) {
    pressed = *(SDLKey *)dynarr_at (Input->keysPressed, i);
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
	k = *(struct input_keys **)dynarr_at (Input->controlMap, ir);
	if (k == NULL)
		return FALSE;
	return keysPressed (k) > 0
		? TRUE
		: FALSE;
}


struct input * input_create ()
{
	struct input * i = xph_alloc (sizeof (struct input));
	i->keysPressed = dynarr_create (4, sizeof (SDLKey));
	i->controlMap = dynarr_create (1, sizeof (struct input_keys *));
	i->activeEvents = dynarr_create (4, sizeof (enum input_responses));
	i->overriddenEvents = dynarr_create (4, sizeof (enum input_responses));
	i->controlledEntities = dynarr_create (2, sizeof (Entity *));
	i->focusedEntities = dynarr_create (2, sizeof (Entity *));

	// fill out the control map here or elsewhere (probably elsewhere in the config/defaults function), but it needs to be populated with the actual key data before any SDL events are checked.
	dynarr_assign (i->controlMap, IR_QUIT, keys_create (1, SDLK_ESCAPE));
	dynarr_assign (i->controlMap, IR_VIEW_WIREFRAME_SWITCH, keys_create (1, SDLK_w));
	dynarr_assign (i->controlMap, IR_AVATAR_MOVE_LEFT, keys_create (1, SDLK_LEFT));
	dynarr_assign (i->controlMap, IR_AVATAR_MOVE_RIGHT, keys_create (1, SDLK_RIGHT));
	dynarr_assign (i->controlMap, IR_AVATAR_MOVE_FORWARD, keys_create (1, SDLK_UP));
	dynarr_assign (i->controlMap, IR_AVATAR_MOVE_BACKWARD, keys_create (1, SDLK_DOWN));
	dynarr_assign (i->controlMap, IR_AVATAR_AUTOMOVE, keys_create (1, SDLK_q));
	dynarr_assign (i->controlMap, IR_CAMERA_MODE_SWITCH, keys_create (1, SDLK_TAB));
	dynarr_assign (i->controlMap, IR_WORLDMAP_SWITCH, keys_create (1, SDLK_SLASH));

	dynarr_assign (i->controlMap, IR_DEBUG_SWITCH, keys_create (1, SDLK_F3));
	return i;
}

void input_destroy (struct input * i)
{
	dynarr_wipe (i->controlMap, (void (*)(void *))keys_destroy);
/*
	dynarr_wipe (i->keysPressed, xph_free);
	vector_wipe (i->activeEvents, (void (*)(void *))input_destroyEvent);
	vector_wipe (i->overriddenEvents, (void (*)(void *))input_destroyEvent);
*/
	dynarr_destroy (i->keysPressed);
	dynarr_destroy (i->controlMap);
	dynarr_destroy (i->activeEvents);
	dynarr_destroy (i->overriddenEvents);
	dynarr_destroy (i->controlledEntities);
	dynarr_destroy (i->focusedEntities);
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
		if (in_dynarr (Input->controlledEntities, e) == -1)
			dynarr_push (Input->controlledEntities, e);
		return TRUE;
	}
	else if (t == INPUT_FOCUSED)
	{
		if (in_dynarr (Input->focusedEntities, e) == -1)
			dynarr_push (Input->focusedEntities, e);
		return TRUE;
	}
	return FALSE;
}

bool input_rmEntity (Entity e, enum input_control_types t)
{
	assert (Input != NULL);
	if (t == INPUT_CONTROLLED)
	{
		dynarr_remove_condense (Input->controlledEntities, e);
		return TRUE;
	}
	else if (t == INPUT_FOCUSED)
	{
		dynarr_remove_condense (Input->focusedEntities, e);
		return TRUE;
	}
	return FALSE;
}


void input_sendGameEventMessage (const struct input_event * ie)
{
	int
		i = 0;
	Entity
		e = NULL,
		created = NULL;
	struct comp_message
		* msg = NULL;
	// CATCH AND HANDLE EVENTS THAT HAVE SYSTEM-WIDE REPERCUSSIONS
	//DEBUG ("GOT INPUTEVENT TYPE %d", ie->ir);
	/* this has become the home of the UI switching; this isn't a good thing. i don't know how to break it apart (presumably to be handled by the ui component??) but it's something that should be done. in the mean time, try to avoid tying the ui code with the input code any further.
	 *  - xph 2011 08 28
	 */
	switch (ie->ir)
	{
		case IR_QUIT:
			system_message (OM_SHUTDOWN, NULL, NULL);
			break;
		case IR_VIEW_WIREFRAME_SWITCH:
			msg = xph_alloc (sizeof (struct comp_message));
			msg->from = NULL;
			msg->message = xph_alloc (17);
			strcpy (msg->message, "WIREFRAME_SWITCH");
			obj_message (VideoObject, OM_SYSTEM_RECEIVE_MESSAGE, msg, msg->message);
			xph_free (msg->message);
			xph_free (msg);
			msg = NULL;
			break;
		case IR_WORLDMAP_SWITCH:
			if (systemState() == STATE_UI &&
				systemTopUIPanelType () == UI_WORLDMAP)
			{
				entity_destroy (systemPopUI ());
				systemPopState ();
			}
			else if (systemState() == STATE_FREEVIEW)
			{
				created = entity_create ();
				component_instantiate ("ui", created);
				entity_message (created, NULL, "setType", (void *)UI_WORLDMAP);
				systemPushUI (created);
				created = NULL;
				systemPushState (STATE_UI);
			}
			break;
		case IR_DEBUG_SWITCH:
			if (systemAttr (SYS_DEBUG) &&
				systemTopUIPanelType () == UI_DEBUG_OVERLAY)
			{
				entity_destroy (systemPopUI ());
				systemToggleAttr (SYS_DEBUG);
			}
			else if (systemState() == STATE_FREEVIEW)
			{
				created = entity_create ();
				component_instantiate ("ui", created);
				entity_message (created, NULL, "setType", (void *)UI_DEBUG_OVERLAY);
				systemPushUI (created);
				created = NULL;
				systemToggleAttr (SYS_DEBUG);
			}
			break;
		default:
			break;
	}
	// SEND MESSAGES OFF TO WORLD ENTITIES
	i = 0;
	while ((e = *(Entity *)dynarr_at (Input->controlledEntities, i++)) != NULL)
	{
		entity_message (e, NULL, "CONTROL_INPUT", (void *)ie);
	}
	i = 0;
	while ((e = *(Entity *)dynarr_at (Input->focusedEntities, i++)) != NULL)
	{
		entity_message (e, NULL, "FOCUS_INPUT", (void *)ie);
	}
}

// FIXME: if there is only ever going to be one controlled entity at one time, there's no need for the array. if there may be more than one controlled entity at one time, this code won't always work.
Entity input_getPlayerEntity ()
{
	if (Input == NULL || dynarr_isEmpty (Input->controlledEntities))
		return NULL;
	return *(Entity *)dynarr_front (Input->controlledEntities);
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
	DynIterator
		it;
	//DEBUG ("INPUT UPDATE HAPPENING NOW\n", NULL);
	while (SDL_PollEvent (&Input->event))
	{
		input_event.ir = IR_NOTHING;
		input_event.event = &Input->event;
		switch (Input->event.type)
		{
			case SDL_KEYDOWN:
				if (in_dynarr (Input->keysPressed, Input->event.key.keysym.sym) >= 0)
					continue;
				dynarr_push (Input->keysPressed, Input->event.key.keysym.sym);
				command = 0;
				activeFinal = dynarr_size (Input->activeEvents) - 1;
				overriddenFinal = dynarr_size (Input->overriddenEvents) - 1;
				it = dynIterator_create (Input->controlMap);
				while (!dynIterator_done (it))
				{
					command = dynIterator_nextIndex (it);
					if (input_controlActive (command) && in_dynarr (Input->activeEvents, command) < 0)
					{
						//printf ("pushing %d onto activeevents\n", command);
						dynarr_push (Input->activeEvents, command);
					}
				}
				dynIterator_destroy (it);
				it = NULL;
				// - active commands iterate to see if they've been overridden
				
				// - overridden commands send ~response messages
				i = overriddenFinal;
				while (++i < dynarr_size (Input->overriddenEvents))
				{
					input_event.ir = *(enum input_responses *)dynarr_at (Input->overriddenEvents, i);
					input_event.ir = ~input_event.ir;
					input_sendGameEventMessage (&input_event);
				}
				
				// - activating commands send response messages
				i = activeFinal;
				while (++i < dynarr_size (Input->activeEvents))
				{
					input_event.ir = *(enum input_responses *)dynarr_at (Input->activeEvents, i);
					//printf ("sending game message from keydown (%d)\n", input_event.ir);
					input_sendGameEventMessage (&input_event);
				}
				break;
			case SDL_KEYUP:
				if (in_dynarr (Input->keysPressed, Input->event.key.keysym.sym) < 0)
					continue;
				dynarr_remove_condense (Input->keysPressed, Input->event.key.keysym.sym);
				
				// - active commands iterate to see if they've been deactivated
				i = 0;
				while (i < dynarr_size (Input->activeEvents))
				{
					command = *(enum input_responses *)dynarr_at (Input->activeEvents, i);
					//printf ("keyup: is %d active?\n", command);
					if (!input_controlActive (command))
					{
						dynarr_unset (Input->activeEvents, i);
						dynarr_condense (Input->activeEvents);
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
					it = dynIterator_create (Input->activeEvents);
					while (!dynIterator_done (it))
					{
						command = *(enum input_responses *)dynIterator_next (it);
						input_event.ir = ~command;
						input_sendGameEventMessage (&input_event);
					}
					dynarr_clear (Input->activeEvents);
					dynarr_clear (Input->overriddenEvents);
					dynarr_clear (Input->keysPressed);
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

int component_input (Object * o, objMsg msg, void * a, void * b)
{
	Entity
		e = NULL;
	char
		* message = NULL;
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

	switch (msg)
	{
		case OM_SHUTDOWN:
		case OM_DESTROY:
			obj_destroy (o);
			return EXIT_SUCCESS;

		case OM_UPDATE:
			input_update (o);
			return EXIT_SUCCESS;
		case OM_COMPONENT_RECEIVE_MESSAGE:
			message = ((struct comp_message *)a)->message;
			e = component_entityAttached (((struct comp_message *)a)->to);

			return EXIT_FAILURE;
		default:
			return obj_pass ();
	}
	return EXIT_FAILURE;
}
