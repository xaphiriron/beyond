/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#ifndef SYSTEM_H
#define SYSTEM_H

#include "xph_timer.h"
#include "entity.h"

#include "map.h"
#include "ogdl/ogdl.h"

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

typedef struct xph_system
{
	Dynarr
		updateFuncs,
		state;			// stores enum system_states
	float
		timer_mult;
	unsigned long
		timestep;
	ACCUMULATOR
		* acc;

	bool
		quit;
	Graph
		config;
} SYSTEM;

extern SYSTEM
	* System;
extern Entity
	SystemEntity;

void systemInit ();
void systemDestroy ();

int systemLoop ();

void systemRender (Dynarr entities);


const TIMER * system_getTimer ();

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
enum system_states systemState ();
bool systemPushState (enum system_states state);
enum system_states systemPopState ();	// returns the new current state
bool systemClearStates ();				// entirely wipes the state stack

/* this function should:
 *  1. set (not push; reset and add) the state to STATE_LOADING
 *  2. initialize/clear a system global loading struct
 *  3. add the loader function to the timed functions run each tick
 *  4. store finishCallback to be called when loading completes (see below)
 */
void systemLoad (void (*initialize)(void), void (*loader)(TIMER *), void (*finishCallback)(void));
void loadSetGoal (unsigned int goal);
void loadSetLoaded (unsigned int loaded);
void loadSetText (char * displayText);

/* the system should render the loading data however applicable (e.g., progress bar; informational text) and continue calling the loader function (via the system timed functions feature) until it signals that it's done (presumably by setting the loading data to 100%) at which point the system should remove the loader from the timed functions and call the finish callback function, which is responsible for getting the system into a reasonable state before the next tick (i.e., immediately)
 *  - xph 2011 06 16
 */

void system_registerTimedFunction (void (*func)(TIMER *), unsigned char weight);
void system_removeTimedFunction (void (*func)(TIMER *));

#endif /* SYSTEM_H */