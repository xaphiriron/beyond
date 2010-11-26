#include "component_input.h"

#include "system.h"

#define BLOCKSIZE	IR_FINAL / CHAR_BIT + 1

static char blockedInputCodes[BLOCKSIZE];

static struct input * Input = NULL;

static int priority_sort (const void * a, const void * b);


bool blocked (enum input_responses i) {
  unsigned int
    o = i / CHAR_BIT,
    m = i % CHAR_BIT;
  if (i > IR_FINAL) {
   return FALSE;
  }
  return blockedInputCodes[o] & (0x01 << m) ? TRUE : FALSE;
}

void block (enum input_responses i) {
  unsigned int
    o = i / CHAR_BIT,
    m = i % CHAR_BIT;
  if (i > IR_FINAL) {
    return;
  }
  blockedInputCodes[o] |= (0x01 << m);
}

void unblock (enum input_responses i) {
  unsigned int
    o = i / CHAR_BIT,
    m = i % CHAR_BIT;
  if (i > IR_FINAL) {
    return;
  }
  blockedInputCodes[o] &= ~(0x01 << m);
}


void clearBlocks () {
  memset (blockedInputCodes, '\0', BLOCKSIZE);
}

void blockConflicts (enum input_responses i) {
  if (i > IR_FINAL) {
    return;
  }
  switch (i) {
    case IR_AVATAR_MOVE_FORWARD:
      block (IR_AVATAR_MOVE_BACKWARD);
      break;
    case IR_AVATAR_MOVE_BACKWARD:
      block (IR_AVATAR_MOVE_FORWARD);
      break;
    case IR_AVATAR_MOVE_LEFT:
      block (IR_AVATAR_MOVE_RIGHT);
      break;
    case IR_AVATAR_MOVE_RIGHT:
      block (IR_AVATAR_MOVE_LEFT);
      break;
    default:
      break;
  }
}

void blockSubsets (const struct inputmatch * m) {
  // get the keycombo from the inputmatch through Input->keymap[m->input] and then iterate through ALL OTHER keymaps and block them if their keycombo is a proper subset of the given keycombo. it sure would be nice if we could build a index for this, so that we can do this in linear time (and regenerate the indices in, uh, n*logn?, but whatever, that would only happen when the keybindings are changed) since we'll be doing this every time we get any kind of key input.
}

int keysPressed (const struct keycombo * k) {
  int
    max = 0,
    len = 0,
    hit = 0,
    i = 0;
  SDLKey pressed = 0;
  const struct keycombo * t = k;
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

static int priority_sort (const void * a, const void * b) {
  return
    (*(const struct inputmatch **)a)->priority -
    (*(const struct inputmatch **)b)->priority;
}


struct input * input_create () {
  new (struct input, i);
  i->keysPressed = vector_create (4, sizeof (SDLKey));
  i->eventsHeld = vector_create (4, sizeof (struct inputmatch *));

  i->controlledEntities = vector_create (2, sizeof (Entity *));
  i->focusedEntities = vector_create (2, sizeof (Entity *));
  i->potentialResponses = vector_create (4, sizeof (struct inputmatch *));

  i->keymap = vector_create (1, sizeof (struct keycombo *));
  // fill out the keymap here or elsewhere (probably elsewhere in the config/defaults function), but it needs to be populated with the actual key data before any SDL events are checked.
  vector_assign (i->keymap, IR_QUIT, keycombo_create (1, SDLK_ESCAPE));
  vector_assign (i->keymap, IR_AVATAR_MOVE_LEFT, keycombo_create (1, SDLK_LEFT));
  vector_assign (i->keymap, IR_AVATAR_MOVE_RIGHT, keycombo_create (1, SDLK_RIGHT));
  vector_assign (i->keymap, IR_AVATAR_MOVE_FORWARD, keycombo_create (1, SDLK_UP));
  vector_assign (i->keymap, IR_AVATAR_MOVE_BACKWARD, keycombo_create (1, SDLK_DOWN));
  return i;
}

void input_destroy (struct input * i) {
  struct keycombo * k = NULL;
  struct inputmatch * im = NULL;
  int
    t = i->keymap->a;
  while (t-- > 0) {
    vector_at (k, i->keymap, t);
    //printf ("%s: got %p at offset %d\n", __FUNCTION__, k, t);
    if (k != NULL) {
      keycombo_destroy (k);
    }
  }
  vector_destroy (i->keymap);

  t = i->eventsHeld->a;
  while (t-- > 0) {
    vector_at (im, i->eventsHeld, t);
    if (im != NULL) {
      inputmatch_destroy (im);
    }
  }
  vector_destroy (i->eventsHeld);
  vector_destroy (i->keysPressed);
  vector_destroy (i->controlledEntities);
  vector_destroy (i->focusedEntities);
  vector_destroy (i->potentialResponses);
  xph_free (i);
}

struct inputmatch * inputmatch_create (enum input_responses i, int priority) {
  new (struct inputmatch, match);
  match->input = i;
  match->priority = priority;
  return match;
}

void inputmatch_destroy (struct inputmatch * im) {
  xph_free (im);
}

struct keycombo * keycombo_create (int n, ...) {
  struct keycombo
    * root = NULL,
    * t = NULL,
    * u = NULL;
  SDLKey k = 0;
  va_list args;
  va_start (args, n);
  while (n > 0) {
    u = xph_alloc (sizeof (struct keycombo));
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

void keycombo_destroy (struct keycombo * k) {
  if (k->next != NULL) {
    keycombo_destroy (k->next);
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
		i = 0,
		priority = 0,
		fill = 0;
	struct keycombo
		* key = NULL;
	struct inputmatch
		* inputmatch = NULL,
		* conflict = NULL;
	struct input_event
		input_event;

	fill = vector_index_last (Input->eventsHeld) + 1;
	while (SDL_PollEvent (&Input->event))
	{
		input_event.ir = IR_NOTHING;
		input_event.event = &Input->event;
		switch (Input->event.type)
		{
			/* the purpose of the keysPressed array is so that if multiple conflicting keys are pressed (e.g., forward and backward) the key most recently held down will dominate. (the matter of how "conflicting keys" are detected is still up in the air) it doesn't do that yet though. */

      case SDL_KEYDOWN:
        if (in_vector (Input->keysPressed, &Input->event.key.keysym.sym) == -1) {
          //printf ("adding key %d to keysPressed (at index %d)\n", Input->event.key.keysym.sym, vector_index_last (Input->keysPressed) + 1);
          vector_push_back (Input->keysPressed, Input->event.key.keysym.sym);
        }
        break;
      case SDL_KEYUP:
        if ((i = in_vector (Input->keysPressed, &Input->event.key.keysym.sym)) == -1) {
          continue;
        }
        //printf ("removing key %d from keysPressed (at index %d)\n", Input->event.key.keysym.sym, i);
        vector_at (inputmatch, Input->eventsHeld, i);
        if (inputmatch != NULL) {
          //printf ("held event: %d at index %d (%p)\n", inputmatch->input, i, inputmatch);
          vector_remove (Input->eventsHeld, inputmatch);
          input_event.ir = ~inputmatch->input;
          input_sendGameEventMessage (&input_event);
          //unblockConflicts (inputmatch->input);
          inputmatch_destroy (inputmatch);
          inputmatch = NULL;
        } else {
          //printf ("no held event\n");
        }
        vector_remove (Input->keysPressed, Input->event.key.keysym.sym);
        i = 0;
        break;

      case SDL_ACTIVEEVENT:
        if (Input->event.active.state & SDL_APPINPUTFOCUS) {
          SDL_ShowCursor (Input->event.active.gain ? SDL_DISABLE : SDL_ENABLE);
          while (!vector_empty (Input->eventsHeld)) {
            vector_pop_back (inputmatch, Input->eventsHeld);
            input_event.ir = ~inputmatch->input;
            input_sendGameEventMessage (&input_event);
            inputmatch_destroy (inputmatch);
          }
          vector_clear (Input->keysPressed);
        }
        break;

      default:
        break;
    }

    // HANDLE SYSTEM/UI/WHATEVER EVENTS
    // (basically check all hardcoded input/event maps and see if we have the keys pressed for them)
    // this whole thing seems overly complicated. When a new key is pressed, what we want to do is check to see what, if any, system key triggers can be completed with the current keys pressed. We want to run only the ones which 1) don't conflict with other keys held down (e.g., holding down left or right should do only one action, not vibrate the avatar back and forth) 2) aren't triggered by subsets of other patterns (e.g, if there are responses for shift+left and left, pressing shift+left should only trigger the former, not the latter. Whatever system we choose for this has to additionally deal with the key commands being set to arbitrary keys and being up to three or four keys long.
    // honestly due to the small size of this (maybe a few hundred input responses, max; only four keys active at any one time) the efficiency of the algorithm probably doesn't matter. I'd like for the code to be /short/, though. ...shorter than this.
/**/
    if (Input->event.type != SDL_KEYUP && Input->event.type != SDL_KEYDOWN && (SDL_GetAppState () & (SDL_APPMOUSEFOCUS | SDL_APPINPUTFOCUS)) == (SDL_APPMOUSEFOCUS | SDL_APPINPUTFOCUS)) {
      input_event.ir = IR_NOTHING;
      input_sendGameEventMessage (&input_event);
    }
    i = 0;
    while (i < Input->keymap->a) {
      vector_at (key, Input->keymap, i);
      if (key == NULL) {
        i++;
        continue;
      }
      if ((priority = keysPressed (key)) > 0) {
        //printf ("maybe we have ir #%d @ priority %d\n", i, priority);
        vector_push_back (Input->potentialResponses, inputmatch_create (i, priority));
      }
      i++;
    }
    vector_sort (Input->potentialResponses, priority_sort);
    clearBlocks ();
    while (vector_pop_back (inputmatch, Input->potentialResponses) != NULL) {
      if (blocked (inputmatch->input)) {
        xph_free (inputmatch);
        continue;
      }
      //printf ("HAVE AN INPUT RESPONSE #%d (%p)\n", inputmatch->input, inputmatch);
      input_event.ir = inputmatch->input;
      input_sendGameEventMessage (&input_event);
      blockConflicts (inputmatch->input);
      blockSubsets (inputmatch);
      //printf ("holding event %d at index %d (%p)\n", inputmatch->input, inputmatch->priority - 1, inputmatch);
      if (vector_at (conflict, Input->eventsHeld, inputmatch->priority - 1) != NULL) {
        printf ("\033[33mINPUT EVENT PRIORITY CONFLICT (\033[1;32m%p\033[0;33m)\33[0m\n", conflict);
      }
      vector_assign (Input->eventsHeld, inputmatch->priority - 1, inputmatch);
    }
//*/
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
