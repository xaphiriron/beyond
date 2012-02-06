#ifndef XPH_COMPONENT_INPUT_H
#define XPH_COMPONENT_INPUT_H

#include <limits.h>
#include <SDL/SDL.h>

#include "entity.h"

#include "xph_memory.h"
#include "dynarr.h"

struct xph_input
{
	bool
		hasFocus;
};

bool input_hasFocus (Entity e);

enum input_control_types
{
	INPUT_CONTROLLED = 1,
	INPUT_FOCUSED,
};

enum input_responses {
	IR_NOTHING,

	IR_FREEMOVE_MOVE_FORWARD,
	IR_FREEMOVE_MOVE_BACKWARD,
	IR_FREEMOVE_MOVE_LEFT,
	IR_FREEMOVE_MOVE_RIGHT,

	IR_FREEMOVE_AUTOMOVE,

	IR_MOUSEMOVE,
	IR_MOUSECLICK,
	IR_TEXT,

	IR_CAMERA_MODE_SWITCH,


	IR_VIEW_WIREFRAME_SWITCH,
	IR_WORLDMAP_SWITCH,
	IR_DEBUG_SWITCH,

	IR_UI_MENU_INDEX_UP,
	IR_UI_MENU_INDEX_DOWN,
	IR_UI_CANCEL,
	IR_UI_CONFIRM,

	IR_UI_MODE_SWITCH,

	IR_WORLD_PLACEARCH,

	IR_WORLDGEN,
	IR_OPTIONS,
	IR_QUIT,

	IR_FINAL
};

typedef struct input_event
{
	enum input_responses
		code;
	bool
		active;
	SDL_Event
		* event;
} inputEvent;

typedef struct input * INPUT;


struct input * input_create ();
void input_destroy (struct input *);

bool input_addEntity (Entity e, enum input_control_types t);
bool input_rmEntity (Entity e, enum input_control_types t);

void input_sendAction (enum input_responses);
void input_sendGameEventMessage (const struct input_event * ie);

void input_define (EntComponent inputComponent, EntSpeech speech);
void input_system (Dynarr entities);

#endif /* XPH_COMPONENT_INPUT_H */