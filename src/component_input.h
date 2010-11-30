#ifndef XPH_COMPONENT_INPUT_H
#define XPH_COMPONENT_INPUT_H

#include <limits.h>
#include <SDL/SDL.h>

#include "entity.h"
#include "object.h"

#include "xph_memory.h"
#include "dynarr.h"

enum input_control_types
{
	INPUT_CONTROLLED = 1,
	INPUT_FOCUSED,
};

enum input_responses {
  IR_NOTHING = 0,

  IR_AVATAR_MOVE_FORWARD,
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

struct input_event
{
	enum input_responses ir;
	SDL_Event * event;
};

typedef struct input * INPUT;

struct input_keys {
  SDLKey key;
  struct input_keys * next;
};


/* returns a value 0-n+1, where n is the number of entries in Input->keysPressed.
 * 0 means at least one of the keys in the keycombo was not pressed; any other
 * value is the index+1 of the highest (most recently pressed) key in the
 * keycombo, and should be considered the combo's "priority"-- if two
 * conflicting keycombos are pressed, the one with higher priority wins. (and
 * if there are two conflicting keycombos with the same priority, the longer
 * one wins)
 */
int keysPressed (const struct input_keys *);
bool input_controlActive (const enum input_responses ir);


struct input * input_create ();
void input_destroy (struct input *);

struct input_keys * keys_create (int n, ...);
void keys_destroy (struct input_keys * k);

bool input_addEntity (Entity e, enum input_control_types t);
bool input_rmEntity (Entity e, enum input_control_types t);

void input_sendGameEventMessage (const struct input_event * ie);

void input_update ();

int component_input (Object * o, objMsg msg, void * a, void * b);

#endif /* XPH_COMPONENT_INPUT_H */