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

	bool
		active;
};

static INPUT
	Input = NULL;

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
		return false;
	k = *(struct input_keys **)dynarr_at (Input->controlMap, ir);
	if (k == NULL)
		return false;
	return keysPressed (k) > 0
		? true
		: false;
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

	i->active = true;

	// fill out the control map here or elsewhere (probably elsewhere in the config/defaults function), but it needs to be populated with the actual key data before any SDL events are checked.
	dynarr_assign (i->controlMap, IR_QUIT, keys_create (1, SDLK_ESCAPE));
	dynarr_assign (i->controlMap, IR_VIEW_WIREFRAME_SWITCH, keys_create (1, SDLK_w));
	dynarr_assign (i->controlMap, IR_FREEMOVE_MOVE_LEFT, keys_create (1, SDLK_LEFT));
	dynarr_assign (i->controlMap, IR_FREEMOVE_MOVE_RIGHT, keys_create (1, SDLK_RIGHT));
	dynarr_assign (i->controlMap, IR_FREEMOVE_MOVE_FORWARD, keys_create (1, SDLK_UP));
	dynarr_assign (i->controlMap, IR_FREEMOVE_MOVE_BACKWARD, keys_create (1, SDLK_DOWN));
	dynarr_assign (i->controlMap, IR_FREEMOVE_AUTOMOVE, keys_create (1, SDLK_q));
	dynarr_assign (i->controlMap, IR_CAMERA_MODE_SWITCH, keys_create (1, SDLK_TAB));
	dynarr_assign (i->controlMap, IR_WORLDMAP_SWITCH, keys_create (1, SDLK_SLASH));

	dynarr_assign (i->controlMap, IR_UI_WORLDMAP_SCALE_UP, keys_create (1, SDLK_UP));
	dynarr_assign (i->controlMap, IR_UI_WORLDMAP_SCALE_DOWN, keys_create (1, SDLK_DOWN));

	dynarr_assign (i->controlMap, IR_UI_MENU_INDEX_UP, keys_create (1, SDLK_UP));
	dynarr_assign (i->controlMap, IR_UI_MENU_INDEX_DOWN, keys_create (1, SDLK_DOWN));
	dynarr_assign (i->controlMap, IR_UI_CONFIRM, keys_create (1, SDLK_RETURN));

	dynarr_assign (i->controlMap, IR_DEBUG_SWITCH, keys_create (1, SDLK_F3));
	return i;
}

void input_destroy (struct input * i)
{
	dynarr_map (i->controlMap, (void (*)(void *))keys_destroy);
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
		return true;
	}
	else if (t == INPUT_FOCUSED)
	{
		if (in_dynarr (Input->focusedEntities, e) == -1)
			dynarr_push (Input->focusedEntities, e);
		return true;
	}
	return false;
}

bool input_rmEntity (Entity e, enum input_control_types t)
{
	assert (Input != NULL);
	if (t == INPUT_CONTROLLED)
	{
		dynarr_remove_condense (Input->controlledEntities, e);
		return true;
	}
	else if (t == INPUT_FOCUSED)
	{
		dynarr_remove_condense (Input->focusedEntities, e);
		return true;
	}
	return false;
}

void input_sendAction (enum input_responses action)
{
	/* FIXME: currently you can't (shouldn't) send any kind of "continued"
	 * input (like the start of a walk sequence) because the inverse "stop"
	 * input when the key is released won't be automatically sent (see
	 * input_update for how it works usually) and that means it's possible to
	 * completely mess up the input system using this function. this isn't
	 * /totally/ the fault of this function, the whole inverse-event-code
	 * concept is kind of... terrible. just keep in mind that this function
	 * should only be used for event codes that 1) don't require an sdl event
	 * and 2) don't require a turn-off signal.
	 *  - xph 2011 09 27
	 */
	struct input_event
		event;
	event.ir = action;
	event.event = NULL;
	input_sendGameEventMessage (&event);
}

void input_sendGameEventMessage (const struct input_event * ie)
{
	int
		i = 0;
	Entity
		e = NULL,
		newUI = NULL;
	// CATCH AND HANDLE EVENTS THAT HAVE SYSTEM-WIDE REPERCUSSIONS
	//DEBUG ("GOT INPUTEVENT TYPE %d", ie->ir);
	/* this has become the home of the UI switching; this isn't a good thing. i don't know how to break it apart (presumably to be handled by the ui component??) but it's something that should be done. in the mean time, try to avoid tying the ui code with the input code any further.
	 *  - xph 2011 08 28
	 */
	if (!Input->active)
		return;
	switch (ie->ir)
	{
		case IR_QUIT:
			system_message (OM_SHUTDOWN, NULL, NULL);
			break;
		case IR_WORLDGEN:
			system_message (OM_FORCEWORLDGEN, NULL, NULL);
			break;
		case IR_VIEW_WIREFRAME_SWITCH:
			video_wireframeSwitch ();
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
				newUI = entity_create ();
				component_instantiate ("ui", newUI);
				entity_message (newUI, NULL, "setType", (void *)UI_WORLDMAP);
				component_instantiate ("input", newUI);
				input_addEntity (newUI, INPUT_FOCUSED);

				systemPushUI (newUI);
				systemPushState (STATE_UI);
				newUI = NULL;
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
				newUI = entity_create ();
				component_instantiate ("ui", newUI);
				entity_message (newUI, NULL, "setType", (void *)UI_DEBUG_OVERLAY);

				systemPushUI (newUI);
				systemToggleAttr (SYS_DEBUG);
				newUI = NULL;
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



static void input_classDestroy (EntComponent comp, EntSpeech speech);
static void input_componentDestroy (EntComponent inputComponent, EntSpeech speech);
static void input_update (EntComponent inputComponent, EntSpeech speech);

void input_define (EntComponent inputComponent, EntSpeech speech)
{
	component_registerResponse ("input", "__classDestroy", input_classDestroy);

	component_registerResponse ("input", "__destroy", input_componentDestroy);

	component_registerResponse ("input", "__update", input_update);

	Input = input_create ();
}

static void input_classDestroy (EntComponent comp, EntSpeech speech)
{
	input_destroy (Input);
	Input = NULL;
}

static void input_componentDestroy (EntComponent inputComponent, EntSpeech speech)
{
	Entity
		inputEntity = component_entityAttached (inputComponent);

	input_rmEntity (inputEntity, INPUT_CONTROLLED);
	input_rmEntity (inputEntity, INPUT_FOCUSED);
}


static void input_update (EntComponent inputComponent, EntSpeech speech)
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
			case SDL_MOUSEMOTION:
				if (systemState () == STATE_FREEVIEW)
				{
					input_event.ir = IR_FREEMOVE_MOUSEMOVE;
					input_sendGameEventMessage (&input_event);
				}
				else if (systemState () == STATE_UI)
				{
					input_event.ir = IR_UI_MOUSEMOVE;
					input_sendGameEventMessage (&input_event);
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
				if (systemState () == STATE_FREEVIEW)
				{
					input_event.ir = IR_FREEMOVE_MOUSECLICK;
					input_sendGameEventMessage (&input_event);
				}
				else if (systemState () == STATE_UI)
				{
					input_event.ir = IR_UI_MOUSECLICK;
					input_sendGameEventMessage (&input_event);
				}
				break;

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
					if (Input->event.active.gain)
					{
						Input->active = true;
						if (systemState () == STATE_FREEVIEW)
							SDL_ShowCursor (SDL_DISABLE);
					}
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
					if (!Input->event.active.gain)
					{
						Input->active = false;
						if (systemState () == STATE_FREEVIEW)
							SDL_ShowCursor (SDL_ENABLE);
					}
				}
				break;
			case SDL_QUIT:
				system_message (OM_SHUTDOWN, NULL, NULL);
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
