#ifndef INPUT_H
#define INPUT_H

#include <stdarg.h>
#include <SDL/SDL.h>
#include "object.h"
#include "system.h"
#include "timer.h"

/* wow this is a jumble. all this code needs to be condensed and reworked so that half the functions are not named control_ and half are named input_. functions which are not actually used should be removed; functions which don't do what they say they do should be rewritten or renamed. Most of the time "system state" is ignored because it's not really well-documented how it'll work yet, but sometimes it's not.

most importantly right now: the order of the inputResponses as children of ControlEntity determines the order they are tested against input in. The first response hit is the one returned. This is a problem for keys with AND responses-- generally, if there's a response that requires [x]+[y] as well as a response that requires [x], if both keys are pressed the response returned is based entirely on which comes before the other.

the solution probably has something to do with cycling through every event on a keydown and giving precidence to the responses with the most complex triggers or sorting the responses so the a response with a key set comes before any responses with a subset of its keys.
 */

extern Object * ControlObject;

enum xph_extrakeys {
  EK_SHIFT = SDLK_LAST + 1,
  EK_CTRL,
  EK_ALT,
  EK_META,
  EK_SUPER
};

typedef struct control {
  SDL_Event event;
  Uint8 * keystate;
  // other data (mouse coords, etc) here

  Object * controlledObject;
  Vector * focusedObjects;

  TIMER * lastInput;
  float inputDelay;

  // we need to put "continuous" events somewhere (timed events, events that
  // start and keep going for a while, etc), but I'm not sure where. not even
  // sure if we still need them conceptually, or if all hypothetical
  // functionality would just replicate things we can already do with entities
  // updating.

} CONTROL;

/*
typedef struct inputFrame {
  char * name;
  bool active;
} INPUTFRAME;
*/

enum input_type {
  TI_KEYBOARD = 1,
  TI_JOYSTICK,
  TI_SYSTEM
};

enum input_keytype {
  INPUT_SDLKEY = FALSE,
  INPUT_UNICODE = TRUE
};

/* DISREGARD THIS; ALL MOUSE INPUT WILL BE HANDLED BY THE ENTITIES THEMSELVES
struct mouseInput {
  // (i guess the idea here is that the mouse input can trigger either with a
  // click on/near (???) the given x,y position, or by having a hit on the
  // names stack that matches. there are some obvious problems with that; most
  // obviously that if we want, for example, clicking on any mob to do
  // something related to it, we have to make a seperate mouse input handler
  // for each one.
  // (in practice if I wanted to do "click on object to highlight it, focus camera on it, and if an aggressive monster, attack it" I'd map the GL names to ranges, and make it possible to look up an object/monster by its guid (which would also be used as its GL name), so that to handle an event like that I'd just have to make a single handler that is all "nearest name is between 5000 and 10000, which means it's an object or a monster, so look it up and do camera focus etc etc"
  int x, y;

  int namecount;
  GLuint * names;
  bool repeat;
};
*/

struct keyInput {
  enum input_type type; /* always TI_KEYBOARD */
  bool keytype; /* uses enum input_keytype */
  SDLKey key;
  bool repeat; /* re-check keystate every cycle, i.e., this is an event you can trigger many times by holding down the given key */
  Uint32 unicode;
};

struct joyInput {
  enum input_type type; /* always TI_JOYSTICK */
  Uint8 which;
  Uint8 button;
};

struct sysInput {
  enum input_type type; /* always TI_SYSTEM */
  Uint8 event;		/* one of: SDL_VIDEOEXPOSE, SDL_QUIT */
};

typedef struct inputTrigger {
  union {
    enum input_type type;
    struct keyInput key;
    struct joyInput joy;
    struct sysInput sys;
  } t;
} inputTrigger;

enum trigger_req {
  TR_OR = FALSE,
  TR_AND = TRUE
};

typedef struct inputResponse {
  Vector * triggers;
  enum trigger_req type;

  enum input_responses {
    IR_AVATAR_MOVE_FORWARD = 1,
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

    IR_QUIT
  } response;
  unsigned int activeMask;
} inputResponse;

CONTROL * control_create ();
void control_destroy (CONTROL *);

enum input_responses control_getResponseForSDLKey (const SDLKey);
enum input_responses control_getResponse (const SDL_Event * e, const Uint8 * k);

bool control_loadConfigSettings (Object * o, char * configPath);
void control_loadDefaultSettings (Object * o);

/*
INPUTFRAME * input_createFrame (char * name);
void input_destroyFrame (INPUTFRAME *);
*/

inputResponse * input_createResponse (Vector * triggers, enum trigger_req type, enum input_responses response, unsigned int activeMask);
void input_destroyResponse (inputResponse *);

void inputResponse_setTriggerMode (Object * o, enum trigger_req);
void inputResponse_setActiveMask (Object * o, unsigned int mask);

bool inputResponse_triggeredByKeyAndState (const inputResponse * r, const SDLKey key, const unsigned int state);
bool inputResponse_matches (const inputResponse * r, const SDL_Event * e, const Uint8 * k);
/*
bool input_matchResponse (const INPUTRESPONSE * r, const SDL_Event * e, const Uint8 * k);
*/

inputTrigger * input_createTrigger (enum input_type type, ...);
Vector * input_createTriggerVector (int n, ...);
void input_destroyTrigger (inputTrigger *);

bool inputTrigger_matchesKey (const inputTrigger * t, const SDLKey key);
bool inputTrigger_matchesKeystate (const inputTrigger * t, const Uint8 * k);
bool inputTrigger_matchesEvent (const inputTrigger * t, const SDL_Event * e);

/*
bool input_matchTriggerToEvent (const INPUTTRIGGER * t, const SDL_Event * e);
bool input_matchTriggerToKeystate (const INPUTTRIGGER * t, const Uint8 * k);
*/



int control_handler (Object * o, objMsg msg, void * a, void * b);
/*
int inputFrame_handler (ENTITY * e, eMessage msg, void * a, void * b);
*/
int inputResponse_handler (Object * o, objMsg msg, void * a, void * b);

#endif /* INPUT_H */