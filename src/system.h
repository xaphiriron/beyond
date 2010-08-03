#ifndef SYSTEM_H
#define SYSTEM_H

#include "entity.h"
#include "video.h"
#include "input.h"
#include "physics.h"
#include "world.h"
#include "timer.h"

extern ENTITY * SystemEntity;

typedef struct system {
  enum system_states {
    STATE_INIT 			= 0x0001,
    STATE_TYPING		= 0x0002,
    STATE_FIRSTPERSONVIEW	= 0x0004,
    STATE_THIRDPERSONVIEW	= 0x0008,
    // ...
    STATE_QUIT			= 0x8000
  } state;

  bool quit;
  CLOCK * clock;
} SYSTEM;

SYSTEM * system_create ();
void system_destroy (SYSTEM *);

int system_handler (ENTITY * e, eMessage msg, void * a, void * b);

#endif /* SYSTEM_H */