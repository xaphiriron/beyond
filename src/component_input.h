#ifndef XPH_COMPONENT_INPUT_H
#define XPH_COMPONENT_INPUT_H

#include <limits.h>
#include <SDL/SDL.h>

#include "entity.h"

#include "xph_memory.h"
#include "dynarr.h"

enum input_control_types
{
	INPUT_CONTROLLED = 1,
	INPUT_FOCUSED,
};

enum input_responses {
	IR_NOTHING					= 0,

	IR_FREEMOVE_MOVE_FORWARD	= 1,
	IR_FREEMOVE_MOVE_BACKWARD	= 2,
	IR_FREEMOVE_MOVE_LEFT		= 3,
	IR_FREEMOVE_MOVE_RIGHT		= 4,

	IR_FREEMOVE_AUTOMOVE		= 5,
	IR_FREEMOVE_LOOK			= 6,

	IR_FREEMOVE_MOUSEMOVE		= 7,
	IR_FREEMOVE_MOUSECLICK		= 8,

	IR_CAMERA_MOVE_BACK			= 16,
	IR_CAMERA_MOVE_FORWARD		= 17,

	IR_CAMERA_MODE_SWITCH		= 18,


	IR_VIEW_WIREFRAME_SWITCH,
	IR_WORLDMAP_SWITCH,
	IR_DEBUG_SWITCH,

	IR_UI_WORLDMAP_SCALE_UP,
	IR_UI_WORLDMAP_SCALE_DOWN,
	IR_UI_MENU_INDEX_UP,
	IR_UI_MENU_INDEX_DOWN,
	IR_UI_CANCEL,
	IR_UI_CONFIRM,

	IR_UI_MOUSEMOVE,
	IR_UI_MOUSECLICK,


	IR_WORLDGEN,
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

void input_sendAction (enum input_responses);
void input_sendGameEventMessage (const struct input_event * ie);

Entity input_getPlayerEntity ();

void input_define (EntComponent inputComponent, EntSpeech speech);

#endif /* XPH_COMPONENT_INPUT_H */