#ifndef SYSTEM_H
#define SYSTEM_H

#include "accumulator.h"
#include "object.h"
#include "ui.h"

#include "map.h"

extern Object * SystemObject;

enum system_states
{
	STATE_ERROR				= 0x0000,
	STATE_LOADING			= 0x0001,
	STATE_FREEVIEW			= 0x0002,

	STATE_UI				= 0x0010,
	STATE_TYPING			= 0x0020,
	// ...
	STATE_QUIT				= 0x8000
};

typedef struct system
{
	Dynarr
		updateFuncs,
		uiPanels,
		state;			// stores enum system_states
	float
		timer_mult,
		timestep;
	CLOCK
		* clock;
	ACCUMULATOR
		* acc;

	bool
		quit;
} SYSTEM;

SYSTEM * system_create ();
void system_destroy (SYSTEM *);

const TIMER system_getTimer ();

/* TODO: states that are more useful. by which I mean, an interface that lets
 * them be used more easily. What I'm thinking of specifically here is a
 * callback function for STATE_LOADING specifically, to allow for actual
 * loading /screens/. But what would also be useful, probably, is something to
 * make state transitions non-binary. So there could be some universal struct
 * instance that things could reference to see, like the previous state, the
 * current state, and what % of switched it was. this could be used for menu
 * transitions or, idk, camera control when entering or exiting cutscenes. or
 * other things. but mostly i just want fancy loading screens with nice
 * transitions.
 * (what would maybe be useful is a special SET STATE TO LOADING + SET
 * CALLBACK FUNC function that also automatically pops the state off when
 * loading finishes... or calls a second "finished" callback func. or both. it
 * would presumably heavily use the syster_registerTimedFunction code to
 * actually do the background loading. [which means i have to get the 'timed'
 * part of that working right, sigh])
 * - xph 2011-05-22
 */
enum system_states systemGetState ();
bool systemPushState (enum system_states state);
enum system_states systemPopState ();	// returns the new current state
bool systemClearStates ();				// entirely wipes the state stack

UIPANEL systemPopUI ();
void systemPushUI (UIPANEL p);
enum uiPanelTypes systemTopUIPanelType ();

void system_registerTimedFunction (void (*func)(TIMER), unsigned char weight);
void system_removeTimedFunction (void (*func)(TIMER));

void systemPlacePlayerAt (const SUBHEX subhex);
void systemRender ();

int system_handler (Object * o, objMsg msg, void * a, void * b);

#endif /* SYSTEM_H */