#ifndef TURTLE_H
#define TURTLE_H
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include "bool.h"
#include "xph_memory.h"
#include "vector.h"
#include "dynarr.h"
#include "line.h"
#include "point.h"

// THINGS LEFT TO DO:
//  X turtle_drawLines(), which requires that i know how to find the scale and
//    position values for an arbitrary collection of points. a little context:
//  D we want drawing turtle lines to be an automated process, so that we can
//  O just call turtle_drawLines() with whatever args it takes and have it draw
//  N everything automagically with GL commands. More specifically, we want a
//  E single function call to draw the lines flat on screen, autosized to fit
//  ! all of them at once. To do this, we need to know two things: the amount
//  ! to scale the point values by so that the entire pattern is smaller than
//  ! the screen's boundary, and the amount we need to offset all the points so
//    that they are centered on the screen.
//  X l-system values that take multiple expansions, picked randomly. it would
//    be nice if it was smart enough to see when two commands would lead to a
//  N divergent solution (i.e., the turtle ends up in a different position
//  V depending on which expansion is picked, so that a cycle may or may not
//  M close, randomly) and warn about that.
//    (WHOOPS APPARENTLY I ALREADY DID THIS, NEVERMIND. MINUS THE SMART PART)
//  - spirals! or general cycles that self-modify between steps. maybe have
//    turtle_runPath() take an additional argument; the symbols to alter each
//    step. for example, ('F', SYM_ADDVAL, 1) would add 1 to the value of F
//    (rotation, movement, whatever) between each execution of the cycle path.
//    self-modifying cycles that modify distance moved will never close, while
//    self-modifying cycles that modify degree rotated will still close, but
//    the cycles-to-close algorithm differs.
//  - circles. curves. arcs and ellipses. they'd require different
//    line-intersection algorithms and rendering functions, and I don't know
//    how we could deal with spline curves or beziers or whatever, which
//    require more than one argument to create. (right now the each turtle
//    movement command can have one argument: distance, or rotation, or
//    whatever, but all curves take more than that-- even an arc requires
//    radius of a circle and the arc distance)
//  ~ extend all of the above to 3D (replace _ROTATE with _PITCH, _YAW, and
//    _HEADING?)
//    (oh hell no, i can do this later when i have rendering done, so that i
//     can actually see what i'm drawing)
//  - unit tests to test the above

// CALCULATE THE CONVEX HULL OF THE LINE'S POINTS
// FIND THE CENTER OF GRAVITY OF THE CONVEX HULL. THAT'S THE POSITION OFFSET
// FIND THE MIN AND MAX X AND Y VALUES. SUBTRACT ONE FROM THE OTHER TO GET THE X AND Y DISTANCE. DIVIDE THE LARGER OF THE TWO BY THE WINDOW RESOLUTION (which is slightly more complicated than that on account of having a window that's not a square) AND THAT IS THE SCALE MULTIPLIER, I THINK
// THIS WOULD BE CALCULATED/UPDATED AUTOMATICALLY ON A RUNCYCLE OR RUNPATH, AND WOULD BE CLEARED ON A turtle_clearLines()
// IT WOULD BE USED BY THE HYPOTHETICAL turtle_drawLines() FUNCTION, WHICH WOULD PROBABLY TAKE (TURTLE *, float matrix[16]) OR JUST THE TURTLE AND DRAW ON THE X,Y PLANE AND TRUST THE REQUIRED TRANSFORMS HAVE BEEN DONE ALREADY.
// EASY VERSION:
// turtle_drawLines (const TURTLE * t, const float matrix[16]) {
//   glPushMatrixf (&matrix);
//   turtle_drawLines2d (t);
//   glPopMatrix();
// }
// turtle_drawLines2d (const TURTLE * t) {
//   ... SOMETHING COMPLEX ...
// }
//

enum turtle_pen {
  TURTLE_PENUP = FALSE,
  TURTLE_PENDOWN = TRUE
};

enum turtle_settings {
  TURTLE_PEN,
  TURTLE_HEADING,
  TURTLE_POSITION
};

typedef Dynarr * TLINES;

struct tloc {
  float heading;
  VECTOR3 position;
};

typedef struct turtle {
  float heading;
  VECTOR3
   position,
   headingVector;
  enum turtle_pen penDown;
  Dynarr
    locationStack,
    lines;
  POINTS points;

  bool scaleCenterClean;
  VECTOR3 center;
  float
    scale,
    lastXResolution,
    lastYResolution;
} TURTLE;

enum symbol_commands {
  SYM_MOVE,
  SYM_ROTATE,
  SYM_PENUP,
  SYM_PENDOWN,
  SYM_STACKPUSH,
  SYM_STACKPOP
};

typedef struct symbolset {
  Dynarr symbols;
} SYMBOLSET;

typedef struct symbol {
  char l;
  enum symbol_commands type;
  float val;
} SYMBOL;

typedef struct lsystem {
  Dynarr p;
} LSYSTEM;

typedef struct production {
  char l;
  Dynarr exp;
} PRODUCTION;

TURTLE * turtle_create ();
TURTLE * turtle_createA (float x, float y, float z, float rot);
void turtle_destroy (TURTLE *);

float turtle_getHeading (const TURTLE *);
const struct vector * turtle_getPosition (const TURTLE *);
enum turtle_pen turtle_getPenStatus (const TURTLE *);

void turtle_rotate (TURTLE *, float deg);
void turtle_move (TURTLE *, float mag);
void turtle_penUp (TURTLE *);
void turtle_penDown (TURTLE *);
void turtle_push (TURTLE *);
void turtle_pop (TURTLE *);

void turtle_SETFAKERESOLUTION (float x, float y);
void turtle_FAKERESOLUTION (bool enable);

const TLINES turtle_getLines (const TURTLE *);
const POINTS turtle_getPoints (TURTLE *);
float turtle_getScale (TURTLE *);
const VECTOR3 * turtle_getCenter (TURTLE *);
void turtle_clearLines (TURTLE *);
void turtle_resetPosition (TURTLE *);

void turtle_setDefault (enum turtle_settings, ...);

int tline_count (const TLINES);

void turtle_runPath (char * p, TURTLE *, const SYMBOLSET *);
void turtle_runCycle (char * c, TURTLE *, const SYMBOLSET *);

SYMBOLSET * symbol_createSet ();
SYMBOL * symbol_create (char l, enum symbol_commands type, float val);
void symbol_destroy (SYMBOL * s);
void symbol_destroySet (SYMBOLSET *);

SYMBOL * symbol_addOperation (SYMBOLSET *, char s, enum symbol_commands type, ...);
bool symbol_isDefined (const SYMBOLSET *, char s);
SYMBOL * symbol_getSymbol (const SYMBOLSET *, char s);
int symbol_definedSymCount (const SYMBOLSET *);

int gcd (int m, int n);
int symbol_cyclesToClose (char * s, const SYMBOLSET *);

int symbol_sort (const void * a, const void * b);
int symbol_search (const void * key, const void * datum);

LSYSTEM * lsystem_create ();
void lsystem_destroy (LSYSTEM * l);
PRODUCTION * production_create (const char l, const char * exp);
void production_destroy (PRODUCTION * p);
void production_addRule (PRODUCTION * p, const char * exp);

bool lsystem_isDefined (const LSYSTEM * l, const char s);

PRODUCTION * lsystem_getProduction (const LSYSTEM * l, char s);
int lsystem_addProduction (LSYSTEM * l, const char s, const char * p);
void lsystem_clearProductions (LSYSTEM * l, const char s);
char * lsystem_getRandProduction (const LSYSTEM * l, const char s);

char * lsystem_iterate (const char * seed, const LSYSTEM * l, int i);

int production_sort (const void * a, const void * b);
int production_search (const void * key, const void * datum);

#endif /* TURTLE_H */