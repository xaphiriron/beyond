#include "input.h"

ENTITY * ControlEntity = NULL;

CONTROL * control_create () {
  CONTROL * c = xph_alloc (sizeof (CONTROL), "CONTROL");
  memset (c, '\0', sizeof (CONTROL));
  c->controlledEntity = NULL;
  c->focusedEntities = listCreate (4, sizeof (ENTITY));
  return c;
}

void control_destroy (CONTROL * c) {
  listDestroy (c->focusedEntities);
  xph_free (c);
}

/*
enum input_responses control_getResponseForSDLKey (const SDLKey key) {
  const SYSTEM * sys = entity_getClassData (SystemEntity, "SYSTEM");
  unsigned int state = sys->state;
  ENTITY * ire = ControlEntity->firstChild;
  inputResponse * ir = NULL;
  while (ire != NULL && (ir = entity_getClassData (ire, "inputResponse")) != NULL) {
    if (inputResponse_triggeredByKeyAndState (ir, key, state)) {
      return ir->response;
    }
    ire = ire->nextSibling;
  }
  return 0;
}
*/

enum input_responses control_getResponse (const SDL_Event * e, const Uint8 * k) {
  ENTITY * ire = ControlEntity->firstChild;
  inputResponse * ir = NULL;
  while (ire != NULL && (ir = entity_getClassData (ire, "inputResponse")) != NULL) {
    //printf ("checking inputResponse data of %p/%p for a hit\n", ire, ir);
    if (inputResponse_matches (ir, e, k)) {
      //printf (" got it! (%d)\n", ir->response);
      return ir->response;
    }
    ire = ire->nextSibling;
  }
  return 0;
}

bool control_loadConfigSettings (ENTITY * ec, char * configPath) {
  control_loadDefaultSettings (ec);
  return FALSE;
}

void control_loadDefaultSettings (ENTITY * ec) {
  ENTITY * key = NULL;

  entity_create ("inputResponse", ControlEntity,
    input_createTriggerList (2,
      input_createTrigger (TI_KEYBOARD, INPUT_SDLKEY, SDLK_ESCAPE),
      input_createTrigger (TI_SYSTEM, SDL_QUIT)
    ),
    (void *)IR_QUIT
  );
  entity_create ("inputResponse", ControlEntity,
    input_createTriggerList (1,
      input_createTrigger (TI_KEYBOARD, INPUT_SDLKEY, SDLK_UP)
    ),
    (void *)IR_AVATAR_MOVE_FORWARD
  );
  entity_create ("inputResponse", ControlEntity,
    input_createTriggerList (1,
      input_createTrigger (TI_KEYBOARD, INPUT_SDLKEY, SDLK_DOWN)
    ),
    (void *)IR_AVATAR_MOVE_BACKWARD
  );

  key = entity_create ("inputResponse", ControlEntity,
    input_createTriggerList (2,
      input_createTrigger (TI_KEYBOARD, INPUT_SDLKEY, SDLK_LEFT),
      input_createTrigger (TI_KEYBOARD, INPUT_SDLKEY, EK_SHIFT)
    ),
    (void *)IR_AVATAR_PAN_LEFT
  );
  inputResponse_setTriggerMode (key, TR_AND);
//  inputResponse_setActiveMask (key, STATE_FIRSTPERSONVIEW);

  key = entity_create ("inputResponse", ControlEntity,
    input_createTriggerList (2,
      input_createTrigger (TI_KEYBOARD, INPUT_SDLKEY, SDLK_RIGHT),
      input_createTrigger (TI_KEYBOARD, INPUT_SDLKEY, EK_SHIFT)
    ),
    (void *)IR_AVATAR_PAN_RIGHT
  );
  inputResponse_setTriggerMode (key, TR_AND);
//  inputResponse_setActiveMask (key, STATE_FIRSTPERSONVIEW);

  entity_create ("inputResponse", ControlEntity,
    input_createTriggerList (1,
      input_createTrigger (TI_KEYBOARD, INPUT_SDLKEY, SDLK_LEFT)
    ),
    (void *)IR_AVATAR_MOVE_LEFT
  );

  entity_create ("inputResponse", ControlEntity,
    input_createTriggerList (1,
      input_createTrigger (TI_KEYBOARD, INPUT_SDLKEY, SDLK_RIGHT)
    ),
    (void *)IR_AVATAR_MOVE_RIGHT
  );

  entity_create ("inputResponse", ControlEntity,
    input_createTriggerList (1,
      input_createTrigger (TI_KEYBOARD, INPUT_SDLKEY, SDLK_TAB)
    ),
    (void *)IR_CAMERA_MODE_SWITCH
  );

/*
  entity_create ("inputResponse", ControlEntity,
    input_createTriggerList (1,
      input_createTrigger ()
    ),
    (void *)IR_
  );
*/
}

inputResponse * input_createResponse (struct list * triggers, enum trigger_req type, enum input_responses response, unsigned int activeMask) {
  inputResponse * r = xph_alloc (sizeof (inputResponse), "inputResponse");
  r->triggers = triggers;
  r->type = type;
  r->response = response;
  r->activeMask = activeMask;
  return r;
}

void input_destroyResponse (inputResponse * r) {
  while (listItemCount (r->triggers) > 0) {
    input_destroyTrigger (listPop (r->triggers));
  }
  listDestroy (r->triggers);
  xph_free (r);
}

void inputResponse_setTriggerMode (ENTITY * e, enum trigger_req trigger) {
  inputResponse * r = entity_getClassData (e, "inputResponse");
  if (r == NULL) {
    return;
  }
  r->type = trigger;
}

void inputResponse_setActiveMask (ENTITY * e, unsigned int mask) {
  inputResponse * r = entity_getClassData (e, "inputResponse");
  if (r == NULL) {
    return;
  }
  r->activeMask = mask;
}


bool inputResponse_triggeredByKeyAndState (const inputResponse * r, const SDLKey key, const unsigned int state) {
  const inputTrigger * t = NULL;
  int i = 0;
  if ((state & r->activeMask) == 0) {
    return FALSE;
  }
  while ((t = listIndex (r->triggers, i++)) != NULL) {
    if (inputTrigger_matchesKey (t, key)) {
      if (TR_OR == r->type) {
        return TRUE;
      }
    } else {
      if (TR_AND == r->type) {
        return FALSE;
      }
    }
  }
  return (TR_AND == r->type)
    ? TRUE
    : FALSE;
}

bool inputResponse_matches (const inputResponse * r, const SDL_Event * e, const Uint8 * k) {
  inputTrigger * t = NULL;
  int i = 0;
/*
  if ((state & r->activeMask) == 0) {
    return FALSE;
  }
*/
  while ((t = listIndex (r->triggers, i++)) != NULL) {
    if ((e != NULL && inputTrigger_matchesEvent (t, e)) ||
        (k != NULL && inputTrigger_matchesKeystate (t, k))) {
      if (TR_OR == r->type) {
        return TRUE;
      }
    } else {
      if (TR_AND == r->type) {
        return FALSE;
      }
    }
  }
  return (TR_AND == r->type)
    ? TRUE
    : FALSE;
}


inputTrigger * input_createTrigger (enum input_type type, ...) {
  inputTrigger * it = xph_alloc (sizeof (inputTrigger), "inputTrigger");
  va_list args;
  it->t.type = type;
  va_start (args, type);
  switch (type) {
    case TI_KEYBOARD:
      it->t.key.keytype = va_arg (args, bool);
      switch (it->t.key.keytype) {
        case INPUT_SDLKEY:
          it->t.key.key = va_arg (args, SDLKey);
          break;
        case INPUT_UNICODE:
          it->t.key.unicode = va_arg (args, Uint32);
          break;
        default:
          fprintf (stderr, "invalid key type (%d). Can't create map.", it->t.key.keytype);
          xph_free (it);
          va_end (args);
          return NULL;
      }
      break;
    case TI_JOYSTICK:
      it->t.joy.which = (Uint8)va_arg (args, int);
      it->t.joy.button = (Uint8)va_arg (args, int);
      break;
    case TI_SYSTEM:
      it->t.sys.event = (Uint8)va_arg (args, int);
      break;
    default:
      fprintf (stderr, "invalid input type (%d) requested; can't create trigger\n", type);
      xph_free (it);
      va_end (args);
      return NULL;
  }
  va_end (args);
  return it;
}

struct list * input_createTriggerList (int n, ...) {
  struct list * l = listCreate (n + 1, sizeof (inputTrigger *));
  va_list args;
  va_start (args, n);
  while (n-- > 0) {
    listAdd (l, va_arg (args, inputTrigger *));
  }
  va_end (args);
  return l;
}

void input_destroyTrigger (inputTrigger * it) {
  xph_free (it);
}


bool inputTrigger_matchesKey (const inputTrigger * t, const SDLKey key) {
  if (t->t.type != TI_KEYBOARD ||
      t->t.key.keytype != INPUT_SDLKEY) {
    return FALSE;
  }

  if (t->t.key.key > SDLK_LAST) {
    switch (t->t.key.key) {
      case EK_SHIFT:
        return key == SDLK_LSHIFT || key == SDLK_RSHIFT;
      case EK_CTRL:
        return key == SDLK_LCTRL || key == SDLK_RCTRL;
      case EK_ALT:
        return key == SDLK_LALT || key == SDLK_RALT;
      case EK_META:
        return key == SDLK_LMETA || key == SDLK_RMETA;
      case EK_SUPER:
        return key == SDLK_LSUPER || key == SDLK_RSUPER;
      default:
        return FALSE;
    }
  }
  return t->t.key.key == key;
}

bool inputTrigger_matchesKeystate (const inputTrigger * t, const Uint8 * k) {
  if (t->t.type != TI_KEYBOARD || t->t.key.keytype != INPUT_SDLKEY) {
    return FALSE;
  }
  if (t->t.key.key > SDLK_LAST) {
    switch (t->t.key.key) {
      case EK_SHIFT:
        return k[SDLK_LSHIFT] | k[SDLK_RSHIFT];
      case EK_CTRL:
        return k[SDLK_LCTRL] | k[SDLK_RCTRL];
      case EK_ALT:
        return k[SDLK_LALT] | k[SDLK_RALT];
      case EK_META:
        return k[SDLK_LMETA] | k[SDLK_RMETA];
      case EK_SUPER:
        return k[SDLK_LSUPER] | k[SDLK_RSUPER];
      default:
        return FALSE;
    }
  }
  if (k[t->t.key.key]) {
    //printf ("JEGUS IT IS THE EXACT SAME THING: K[%d] IS %d\n", t->t.key.key, k[t->t.key.key]);
  }
  return k[t->t.key.key]
    ? TRUE
    : FALSE;
}

bool inputTrigger_matchesEvent (const inputTrigger * t, const SDL_Event * e) {
  switch (e->type) {
    case SDL_KEYDOWN:
      if (TI_KEYBOARD != t->t.type) {
        return FALSE;
      }
      //printf ("EVENT IS AN SDL_KEYDOWN AND THIS IS A KEYBOARD TRIGGER\n");
      if (t->t.key.keytype == INPUT_UNICODE &&
          t->t.key.unicode == e->key.keysym.unicode) {
        return TRUE;
      }
      return inputTrigger_matchesKey (t, e->key.keysym.sym);
      break;

    case SDL_JOYBUTTONDOWN:
      if (TI_JOYSTICK != t->t.type) {
        return FALSE;
      }
      //printf ("EVENT IS AN SDL_JOYBUTTONDOWN AND THIS IS A JOYSTICK TRIGGER\n");
      if (t->t.joy.which == e->jbutton.which &&
          t->t.joy.button == e->jbutton.button) {
        return TRUE;
      }
      break;
    case SDL_QUIT:
    case SDL_VIDEOEXPOSE:
      if (TI_SYSTEM == t->t.type && t->t.sys.event == e->type) {
        //printf ("EVENT IS SDL_QUIT/SDL_VIDEOEXPOSE AND THIS IS AN EQUAL SYSTEM TRIGGER\n");
        return TRUE;
      }
  }
  return FALSE;
}


int control_handler (ENTITY * e, eMessage msg, void * a, void * b) {
  CONTROL * c = NULL;

  ENTITY * focus = NULL;
//  inputResponse * continuous = NULL;
  int i = 0;
  enum input_responses responseType = 0;

  switch (msg) {
    case EM_CLSNAME:
      strncpy (a, "control", 32);
      return EXIT_SUCCESS;
    case EM_CLSINIT:
      entclass_init (inputResponse_handler, NULL, NULL, NULL);
      return EXIT_SUCCESS;
    case EM_CLSFREE:
      entclass_destroyS ("inputResponse");
      return EXIT_SUCCESS;
    case EM_CLSVARS:
      return EXIT_FAILURE;
    case EM_CREATE:
      if (ControlEntity != NULL) {
        entity_destroy (e);
        return EXIT_FAILURE;
      }
      c = control_create ();
      ControlEntity = e;
      if (a != NULL) {
        control_loadConfigSettings (e, a);
      } else {
        control_loadDefaultSettings (e);
      }
      entity_addClassData (e, "control", c);
      return EXIT_SUCCESS;
    default:
      break;
  }
  c = entity_getClassData (e, "control");
  assert (c != NULL);
  switch (msg) {
    case EM_SHUTDOWN:
    case EM_DESTROY:
      ControlEntity = NULL;
      control_destroy (c);
      entity_rmClassData (e, "control");
      entity_destroy (e);
      return EXIT_SUCCESS;

    case EM_START:
      if (SDL_WasInit (SDL_INIT_JOYSTICK) == 0) {
        SDL_InitSubSystem (SDL_INIT_JOYSTICK);
      }
      return EXIT_SUCCESS;

    case EM_SUSPEND:
      return EXIT_FAILURE;


    case EM_UPDATE:
      c->keystate = SDL_GetKeyState (NULL);
      while (SDL_PollEvent (&c->event)) {
        //printf ("ControlEntity:EM_UPDATE: we got an event\n");
        entity_message (e, EM_INPUT, &c->event, c->keystate);
/*
        if (c->event.type == SDL_KEYDOWN || c->event.type == SDL_KEYUP) {
        } else if (c->event.type == SDL_[whatever]) {
        } else {
          entity_message (e, EM_SYSINPUT, &c->event, c->keystate);
        }
*/
        i = 0;
        while ((focus = listIndex (c->focusedEntities, i++)) != NULL) {
          entity_message (focus, EM_INPUT, &c->event, c->keystate);
        }
      }
      return EXIT_SUCCESS;

    case EM_INPUT:
      //printf ("ControlEntity:EM_INPUT: omg handling input\n");
      responseType = control_getResponse (a, b);
      if (responseType == 0) {
        printf ("no response\n");
        return EXIT_FAILURE;
      }
      printf ("got response: %d\n", responseType);
      switch (responseType) {
        // ... imagine a big list here
        case IR_QUIT:
          entity_message (SystemEntity, EM_SHUTDOWN, NULL, NULL);
          return EXIT_SUCCESS;

        default:
          return EXIT_FAILURE;
      }
      return EXIT_SUCCESS;

    default:
      return entity_pass ();
  }
  return EXIT_FAILURE;
}

int inputResponse_handler (ENTITY * e, eMessage msg, void * a, void * b) {
  inputResponse * i = NULL;
  switch (msg) {
    case EM_CLSNAME:
      strncpy (a, "inputResponse", 32);
      return EXIT_SUCCESS;
    case EM_CLSINIT:
    case EM_CLSFREE:
      return EXIT_FAILURE;
    case EM_CLSVARS:
      return EXIT_FAILURE;

    case EM_CREATE:
      i = input_createResponse (a, TR_OR, (enum input_responses)b, 0xffff);
      entity_addClassData (e, "inputResponse", i);
      return EXIT_SUCCESS;

    default:
      break;
  }
  i = entity_getClassData (e, "inputResponse");
  switch (msg) {
    case EM_SHUTDOWN:
    case EM_DESTROY:
      input_destroyResponse (i);
      entity_rmClassData (e, "inputResponse");
      entity_destroy (e);
      return EXIT_SUCCESS;

/* I don't know if these will even response to EM_INPUT-- they're not really made to be messaged, just to store data. which probably means they shouldn't be entities, but whatever. (it might be a good idea though to do the polling for responses through entity messages, though. entity_messageChildren is kind of the perfect function for it
*/

    default:
      return entity_pass ();
  }
  return EXIT_FAILURE;
}
