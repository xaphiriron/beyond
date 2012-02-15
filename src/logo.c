/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#include "beyond.h"
#include "turtle.h"
#include "turtledraw.h"

int main (int argc, char * argv []) {
  SYSTEM * sys = NULL;

  TURTLE * t = turtle_create ();
  SYMBOLSET * sym = symbol_createSet ();
  LSYSTEM * l = lsystem_create ();
  char * path = NULL;

  symbol_addOperation (sym, 'L', SYM_MOVE, 1.0);
  symbol_addOperation (sym, 'M', SYM_MOVE, 0.5);
  symbol_addOperation (sym, 'S', SYM_MOVE, 0.25);
  symbol_addOperation (sym, '-', SYM_ROTATE, -45.0);
  symbol_addOperation (sym, '+', SYM_ROTATE, 45.0);
  symbol_addOperation (sym, '[', SYM_STACKPUSH);
  symbol_addOperation (sym, ']', SYM_STACKPOP);

  lsystem_addProduction (l, 'X', "L[-S+L]M[+S-L]");
  lsystem_addProduction (l, 'L', "-M-L+M+");
  lsystem_addProduction (l, 'L', "L[-S+L]M[+S-L]");
  lsystem_addProduction (l, 'M', "-S-M+S+");
  lsystem_addProduction (l, 'M', "M");
  

  path = lsystem_iterate ("X", l, 5);
  turtle_runPath (path, t, sym);

  objClass_init (system_handler, NULL, NULL, NULL);
  sys = obj_getClassData (SystemObject, "SYSTEM");

  obj_message (SystemObject, OM_START, NULL, NULL);

  while (!sys->quit) {
    obj_message (SystemObject, OM_UPDATE, NULL, NULL);
    obj_message (ControlObject, OM_UPDATE, NULL, NULL);

    obj_messagePre (VideoObject, OM_PRERENDER, NULL, NULL);
    turtle_drawLines (t);
    obj_messagePre (VideoObject, OM_POSTRENDER, NULL, NULL);
  }

  xph_free (path);
  lsystem_destroy (l);
  symbol_destroySet (sym);
  turtle_destroy (t);

  obj_message (SystemObject, OM_DESTROY, NULL, NULL);
  objClass_destroy ("SYSTEM");
  objects_destroyEverything ();
  return 0;
}