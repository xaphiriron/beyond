#ifndef SYSTEM_H
#define SYSTEM_H

#include "accumulator.h"
#include "object.h"
#include "ui.h"

extern Object * SystemObject;

enum system_states
{
	STATE_ERROR				= 0x0000,
	STATE_INIT 				= 0x0001,
	STATE_WORLDGEN			= 0x0002,
	STATE_FIRSTPERSONVIEW	= 0x0004,
	STATE_THIRDPERSONVIEW	= 0x0008,
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

enum system_states systemGetState ();
bool systemPushState (enum system_states state);
enum system_states systemPopState ();	// returns the new current state
bool systemClearStates ();				// entirely wipes the state stack

UIPANEL systemPopUI ();
void systemPushUI (UIPANEL p);
enum uiPanelTypes systemTopUIPanelType ();

void system_registerTimedFunction (void (*func)(TIMER), unsigned char weight);
void system_removeTimedFunction (void (*func)(TIMER));

void systemRender ();

int system_handler (Object * o, objMsg msg, void * a, void * b);

#endif /* SYSTEM_H */