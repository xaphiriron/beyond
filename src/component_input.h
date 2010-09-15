#ifndef XPH_COMPONENT_INPUT_H
#define XPH_COMPONENT_INPUT_H

#include <limits.h>
#include <SDL/SDL.h>

#include "entity.h"
#include "object.h"

#include "xph_memory.h"
#include "cpv.h"

enum input_responses {
  IR_AVATAR_MOVE_FORWARD = 0,
  IR_AVATAR_MOVE_BACKWARD,
  IR_AVATAR_MOVE_LEFT,
  IR_AVATAR_MOVE_RIGHT,

  IR_AVATAR_PAN_UP,
  IR_AVATAR_PAN_DOWN,
  IR_AVATAR_PAN_LEFT,
  IR_AVATAR_PAN_RIGHT,

  IR_CAMERA_MOVE_BACK,
  IR_CAMERA_MOVE_FORWARD,

  IR_CAMERA_PAN_UP,
  IR_CAMERA_PAN_DOWN,
  IR_CAMERA_PAN_LEFT,
  IR_CAMERA_PAN_RIGHT,

  IR_CAMERA_MODE_SWITCH,

  IR_QUIT,

  IR_FINAL
};

struct input {
  Vector
    * keysPressed,	// all keys currently held down, with most recently-pushed on top
    * eventsHeld;	// the input events triggered by keys which have been pressed but haven't been released yet
  SDL_Event event;
  Uint8 * keystate;

  Vector		// which keys are mapped to which in-game commands;
    * keymap;		// e.g., keymap[IR_QUIT] = (struct keycombo *)keys,
			// which is tested by keysPressed (keymap[IR_QUIT])

  Vector
    * potentialResponses,
    * controlledEntities,
    * focusedEntities;
};

struct keycombo {
  SDLKey key;
  struct keycombo * next;
};

struct inputmatch {
  enum input_responses input;
  int priority;
};

bool blocked (enum input_responses i);
void block (enum input_responses i);
void clearBlocks ();
void blockConflicts (enum input_responses i);
void blockSubsets (const struct inputmatch *);

/* returns a value 0-n+1, where n is the number of entries in Input->keysPressed.
 * 0 means at least one of the keys in the keycombo was not pressed; any other
 * value is the index+1 of the highest (most recently pressed) key in the
 * keycombo, and should be considered the combo's "priority"-- if two
 * conflicting keycombos are pressed, the one with higher priority wins. (and
 * if there are two conflicting keycombos with the same priority, the longer
 * one wins)
 */
int keysPressed (const struct keycombo *);


struct input * input_create ();
void input_destroy (struct input *);

struct inputmatch * inputmatch_create (enum input_responses i, int priority);
void inputmatch_destroy (struct inputmatch *);

struct keycombo * keycombo_create (int n, ...);
void keycombo_destroy (struct keycombo * k);

void input_addControlledEntity (Entity * e);
void input_rmControlledEntity (Entity * e);

void input_addFocusedEntity (Entity * e);
void input_rmFocusedEntity (Entity * e);

void input_sendGameEventMessage (enum input_responses);

void input_update ();

int component_input (Object * o, objMsg msg, void * a, void * b);

#endif /* XPH_COMPONENT_INPUT_H */