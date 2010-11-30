#include "check_check.h"
#include "../src/turtle.h"
#include "../src/vector.h"
#include "../src/fcmp.h"

START_TEST (test_turtle_create) {
  struct turtle * t = turtle_create ();
  struct vector v = vectorCreate (0.0, 0.0, 0.0);
  fail_unless (
    t != NULL,
    "Turtle not created");
  fail_unless (
    turtle_getHeading (t) == 0.0 &&
    vector_cmp (turtle_getPosition (t), &v),
    "Turtle position and heading not initialized correctly.");
  turtle_destroy (t);
  turtle_setDefault (TURTLE_HEADING, -45.5);
  v = vectorCreate (1.0, -20.0, 300.0);
  turtle_setDefault (TURTLE_POSITION, v);
  t = turtle_create ();
  fail_unless (
    fcmp (turtle_getHeading (t), -45.5),
    "Heading on creation is allowed to be changed from the default.");
  fail_unless (
    vector_cmp (turtle_getPosition (t), &v),
    "Position on creation is allowed to be changed from the default.");
  turtle_destroy (t);
}
END_TEST

START_TEST (test_turtle_rotate) {
  struct turtle * t = turtle_create ();
  turtle_rotate (t, 90.0);
  fail_unless (
    fcmp (turtle_getHeading (t), 90.0),
    "Turtle must be able to rotate");
  turtle_rotate (t, -180.0);
  fail_unless (
    fcmp (turtle_getHeading (t), -90.0),
    "Turtle rotation must stack.");
  turtle_rotate (t, -180.0);
  fail_unless (
    fcmp (turtle_getHeading (t), 90.0),
    "Turtle rotation must be clamped to -180 through 180.");
  turtle_rotate (t, 360.0);
  fail_unless (
    fcmp (turtle_getHeading (t), 90.0),
    "Turtle rotation must be clamped to -180 through 180.");
  turtle_destroy (t);
}
END_TEST

START_TEST (test_turtle_move) {
  struct turtle * t = turtle_create ();
  struct vector v = vectorCreate (0.0, 0.0, 0.0);
  turtle_move (t, 10.0);
  v = vectorCreate (0.0, 10.0, 0.0);
  fail_unless (
    vector_cmp (turtle_getPosition (t), &v),
    "Turtle must move when told to move");
  turtle_rotate (t, 90.0);
  turtle_move (t, 10.0);
  v = vectorCreate (10.0, 10.0, 0.0);
  fail_unless (
    vector_cmp (turtle_getPosition (t), &v),
    "Turtle movement must stack");
  turtle_rotate (t, 180.0);
  turtle_move (t, 20.0);
  turtle_rotate (t, -90.0);
  turtle_move (t, 20.0);
  v = vectorCreate (-10.0, -10.0, 0.0);
  fail_unless (
    vector_cmp (turtle_getPosition (t), &v));
  turtle_destroy (t);
}
END_TEST

START_TEST (test_turtle_pen) {
  struct turtle * t = turtle_create ();
  const TLINES l = turtle_getLines (t);
  fail_unless (
    turtle_getPenStatus (t) == TURTLE_PENDOWN,
    "Turtle pen should be down automatically on creation.");
  turtle_destroy (t);
  turtle_setDefault (TURTLE_PEN, TURTLE_PENUP);
  t = turtle_create ();
  fail_unless (
    turtle_getPenStatus (t) == TURTLE_PENUP,
    "Default turtle pen status should be altered by _setDefault.");
  turtle_penDown (t);
  turtle_move (t, 200.0);
  fail_unless (
    tline_count (l) == 1,
    "Lines must be recorded when pen is down");
  turtle_rotate (t, 90.0);
  turtle_move (t, 200.0);
  fail_unless (
    tline_count (l) == 2);
  turtle_penUp (t);
  turtle_move (t, 200.0);
  fail_unless (
    tline_count (l) == 2,
    "Lines must not be recorded when pen is up");
  turtle_clearLines (t);
  fail_unless (
    tline_count (l) == 0,
    "Lines must be clearable.");
  turtle_destroy (t);
}
END_TEST

START_TEST (test_turtle_stack) {
  struct turtle * t = turtle_create ();
  VECTOR3 v = vectorCreate (0.0, 50.0, 0.0);
  turtle_move (t, 25.0);
  turtle_push (t);
  turtle_move (t, 25.0);
  turtle_rotate (t, 90.0);
  turtle_push (t);
  turtle_move (t, 50.0);
  turtle_rotate (t, 45.0);
  turtle_pop (t);
  fail_unless (
    vector_cmp (turtle_getPosition (t), &v),
    "Turtle position stack must restore last pushed position.");
  fail_unless (
    fcmp (turtle_getHeading (t), 90.0),
    "Turtle position stack must restore last pushed heading.");
  turtle_pop (t);
  v =  vectorCreate (0.0, 25.0, 0.0);
  fail_unless (
    vector_cmp (turtle_getPosition (t), &v),
    "Turtle stack must have multiple entries");
  turtle_pop (t);
  fail_unless (
    vector_cmp (turtle_getPosition (t), &v),
    "Stack underflow must be ignored");
  turtle_destroy (t);
}
END_TEST

START_TEST (test_symbols_create) {
  SYMBOLSET * s = symbol_createSet ();
  fail_unless (
    s != NULL,
    "Symbolset not created");
  fail_unless (
    symbol_definedSymCount (s) == 0,
    "A symbolset should be created with no symbols by default");
  symbol_destroySet (s);
}
END_TEST

START_TEST (test_symbols_define) {
  TURTLE * t = turtle_create ();
  VECTOR3 v = vectorCreate (0.0, 0.0, 0.0);
  const TLINES l = turtle_getLines (t);
  SYMBOLSET * s = symbol_createSet ();
  SYMBOL * sym = NULL;

  fail_unless (
    symbol_isDefined (s, 'Q') == FALSE);
  fail_unless (
    (sym = symbol_getSymbol (s, 'Q')) == NULL,
    "A request for an undefined symbol must return NULL");
  symbol_addOperation (s, 'F', SYM_MOVE, 1.0);
  fail_unless (
    symbol_isDefined (s, 'F'),
    "Symbols must be defined when added to a symbol set.");
  symbol_addOperation (s, '+', SYM_ROTATE, 90.0);
  symbol_addOperation (s, '-', SYM_ROTATE, -90.0);
  fail_unless (
    symbol_isDefined (s, 'F') &&
    symbol_isDefined (s, '+') &&
    symbol_isDefined (s, '-') &&
    symbol_definedSymCount (s) == 3);
  turtle_penDown (t);
  turtle_runPath ("QQSTUV_Q=Q", t, s);
  fail_unless (
    tline_count (l) == 0,
    "A turtle must completely ignore undefined symbols.");
  turtle_runPath ("F+", t, s);
  v = vectorCreate (0.0, 1.0, 0.0);
  fail_unless (
    vector_cmp (turtle_getPosition (t), &v),
    "Turtles must move in response to paths run.");
  fail_unless (
    turtle_getHeading (t) == 90.0,
    "Turtles must rotate in response to paths run.");
  turtle_runPath ("F+F+", t, s);
  fail_unless (
    tline_count (l) == 3,
    "Turtles must draw lines behind them when they run paths.");
  symbol_addOperation (s, '+', SYM_ROTATE, 80.0);
  sym = symbol_getSymbol (s, '+');
  fail_unless (
    sym->type == SYM_ROTATE &&
    sym->val == 80.0,
    "Symbols must be redefinable.");
  sym = NULL;
  symbol_destroySet (s);
  turtle_destroy (t);
}
END_TEST

START_TEST (test_symbols_path) {
  TURTLE * t = turtle_create ();
  const TLINES l = turtle_getLines (t);
  SYMBOLSET * s = symbol_createSet ();
  VECTOR3 v = vectorCreate (0.0, 0.0, 0.0);

  symbol_addOperation (s, 'F', SYM_MOVE, 1.0);
  symbol_addOperation (s, '+', SYM_ROTATE, 80.0);
  symbol_addOperation (s, '^', SYM_PENUP);
  symbol_addOperation (s, 'v', SYM_PENDOWN);

  fail_unless (
    symbol_cyclesToClose ("F+", s) == 9,
    "symbol_cyclesToClose must calculate the number of cycle iterations to close a polygon.");
  turtle_runCycle ("F+", t, s);
  fail_unless (
    tline_count (l) == 9,
    "Turtles must draw lines in a cycle if their pen is down");
  symbol_addOperation (s, '+', SYM_ROTATE, -180.0);
  fail_unless (
    symbol_cyclesToClose ("F+", s) == 2,
    "symbol_cyclesToClose must calculate the number of cycle iterations to close a polygon. (%d)", symbol_cyclesToClose ("F+", s));
  fail_unless (
    symbol_cyclesToClose ("F", s) < 0,
    "A cycle that does not close must return negative.");
  turtle_clearLines (t);
  symbol_addOperation (s, '+', SYM_ROTATE, 120.0);
  symbol_addOperation (s, '-', SYM_ROTATE, -40.0);
  turtle_runCycle ("F+^F-v", t, s);
  fail_unless (
    tline_count (l) == 9,
    "SYM_PENUP and SYM_PENDOWN must raise and lower the pen when triggered by symbols.");
  symbol_addOperation (s, '[', SYM_STACKPUSH);
  symbol_addOperation (s, ']', SYM_STACKPOP);
  turtle_clearLines (t);
  turtle_resetPosition (t);
  turtle_runPath ("F[[--FF+FF", t, s);
  turtle_runPath ("]FF+FF--]", t, s);
  v = vectorCreate (0.0, 1.0, 0.0);
  fail_unless (
    vector_cmp (turtle_getPosition (t), &v) == TRUE,
    "SYM_STACKPUSH and SYM_STACKPOP must return turtle to its stored position.");
  fail_unless (
    fcmp (turtle_getHeading (t), 0.0) == TRUE,
    "SYM_STACKPUSH and SYM_STACKPOP must revert heading changes as well as position.");

  symbol_destroySet (s);
  turtle_destroy (t);
}
END_TEST

START_TEST (test_lsystem_create) {
  LSYSTEM * l = lsystem_create ();
  fail_unless (
    l != NULL,
    "L-system not created.");
  lsystem_destroy (l);
}
END_TEST

START_TEST (test_lsystem_productions) {
  LSYSTEM * l = lsystem_create ();
  fail_unless (
    lsystem_addProduction (l, 'F', "FF") == 1,
    "The return value of _addProduction must be the number of production rules defined for that variable, including the most recently added.");
  fail_unless (
    lsystem_addProduction (l, 'F', "F[-X]F") == 2,
    "The return value of _addProduction must be the number of production rules defined for that variable, including the most recently added.");
  fail_unless (
    lsystem_isDefined (l, 'F') == TRUE,
    "Defined symbols must cause lsystem_isDefined to return TRUE");
  fail_unless (
    lsystem_isDefined (l, 'X') == FALSE,
    "Undefined symbols must cause lsystem_isDefined to return FALSE");
  lsystem_clearProductions (l, 'F');
  fail_unless (
    lsystem_isDefined (l, 'F') == FALSE,
    "Production rules must be clearable.");
  lsystem_destroy (l);
}
END_TEST

START_TEST (test_lsystem_iterate) {
  LSYSTEM * l = lsystem_create ();
  char * e = NULL;
  e = lsystem_iterate ("X", l, 12);
  fail_unless (
    strcmp (e, "X") == 0,
    "Undefined variables must implicitly use the A -> A production rule");
  lsystem_addProduction (l, 'F', "FF");
  xph_free (e);
  e = lsystem_iterate ("F", l, 0);
  fail_unless (
    strcmp (e, "F") == 0,
    "Iterating <= 0 times should return a copy of the original string");
  xph_free (e);
  e = lsystem_iterate ("F", l, 3);
  fail_unless (
    strcmp (e, "FFFFFFFF") == 0);
  lsystem_addProduction (l, 'X', "F[[-X]+X]");
  xph_free (e);
  e = lsystem_iterate ("X", l, 4);
  fail_unless (
    strcmp (e, "FFFFFFFF[[-FFFF[[-FF[[-F[[-X]+X]]+F[[-X]+X]]]+FF[[-F[[-X]+X]]+F[[-X]+X]]]]+FFFF[[-FF[[-F[[-X]+X]]+F[[-X]+X]]]+FF[[-F[[-X]+X]]+F[[-X]+X]]]]") == 0,
    "Binary tree expansion failure");
  lsystem_clearProductions (l, 'X');
  lsystem_clearProductions (l, 'F');
  lsystem_addProduction (l, 'K', "K-K+K-K");
  xph_free (e);
  e = lsystem_iterate ("K", l, 5);
  fail_unless (
    strcmp (e, "K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K-K-K+K-K-K-K+K-K+K-K+K-K-K-K+K-K") == 0,
    "Koch curve expansion failure");
  lsystem_destroy (l);
  xph_free (e);
}
END_TEST

START_TEST (test_draw_center) {
  TURTLE * t = turtle_create ();
  SYMBOLSET * s = symbol_createSet ();
  const VECTOR3 * c = NULL;
  VECTOR3 d = vectorCreate (0, 0, 0);
  fail_unless (
    vector_cmp (turtle_getCenter (t), &d) == TRUE,
    "If no lines are drawn, the default center must be the origin"
  );
  symbol_addOperation (s, '+', SYM_ROTATE, 90.0);
  symbol_addOperation (s, 'F', SYM_MOVE, 100.0);
  turtle_runCycle ("F+", t, s);
  mark_point ();
  c = turtle_getCenter (t);
  mark_point ();
  d = vectorCreate (50.0, 50.0, 0);
  fail_unless (
    vector_cmp (c, &d) == TRUE,
    "Turtle center must be the center of its drawn lines (%f, %f, %f)", c->x, c->y, c->z
  );
}
END_TEST

START_TEST (test_draw_scale) {
  TURTLE * t = turtle_create ();
  SYMBOLSET * s = symbol_createSet ();
  float scale = 0;
  scale = turtle_getScale (t);
  fail_unless (
    fcmp (scale, 1.0) == TRUE,
    "If no lines are drawn, the default scale must be 1"
  );
  symbol_addOperation (s, '+', SYM_ROTATE, 90.0);
  symbol_addOperation (s, 'F', SYM_MOVE, 100.0);
  turtle_runCycle ("F+", t, s);
  turtle_SETFAKERESOLUTION (1000.0, 1000.0);
  mark_point ();
  scale = turtle_getScale (t);
  mark_point ();
  fail_unless (
    fcmp (scale, 10.0) == TRUE,
    "Scale must return a correct value for resize if the design can be enlarged"
  );
  turtle_SETFAKERESOLUTION (10.0, 10.0);
  scale = turtle_getScale (t);
  fail_unless (
    fcmp (scale, 0.1) == TRUE,
    "Scale must return a correct value < 1.0 if the design is larger than the screen"
  );
  turtle_SETFAKERESOLUTION (720.0, 540.0);
  scale = turtle_getScale (t);
  fail_unless (
    fcmp (scale, 5.4) == TRUE,
    "The scale value should respect the most limiting dimension (y)"
  );
  turtle_SETFAKERESOLUTION (540.0, 720.0);
  scale = turtle_getScale (t);
  fail_unless (
    fcmp (scale, 5.4) == TRUE,
    "The scale value should respect the most limiting dimension (x)"
  );
}
END_TEST

void draw_setup (void) {
  turtle_FAKERESOLUTION (TRUE);
}

void draw_teardown (void) {
  turtle_FAKERESOLUTION (FALSE);
}

Suite * make_turtle_suite (void) {
  Suite * s = suite_create ("Turtle");
  TCase
    * tc_core = tcase_create ("Core"),
    * tc_symbol = tcase_create ("Symbols"),
    * tc_lsys = tcase_create ("L-Systems"),
    * tc_draw = tcase_create ("Drawing");
  tcase_add_test (tc_core, test_turtle_create);
  tcase_add_test (tc_core, test_turtle_rotate);
  tcase_add_test (tc_core, test_turtle_move);
  tcase_add_test (tc_core, test_turtle_pen);
  tcase_add_test (tc_core, test_turtle_stack);
  suite_add_tcase (s, tc_core);
  tcase_add_test (tc_symbol, test_symbols_create);
  tcase_add_test (tc_symbol, test_symbols_define);
  tcase_add_test (tc_symbol, test_symbols_path);
  suite_add_tcase (s, tc_symbol);
  tcase_add_test (tc_lsys, test_lsystem_create);
  tcase_add_test (tc_lsys, test_lsystem_productions);
  tcase_add_test (tc_lsys, test_lsystem_iterate);
  suite_add_tcase (s, tc_lsys);
  tcase_add_checked_fixture (tc_draw, draw_setup, draw_teardown);
  tcase_add_test (tc_draw, test_draw_center);
  tcase_add_test (tc_draw, test_draw_scale);
  suite_add_tcase (s, tc_draw);
  return s;
}
