#include <check.h>
#include <stdlib.h>
#include "../src/dynarr.h"
#include "../src/lsystem.h"

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

Suite * make_suite (void) {
	Suite
		* s = suite_create ("L-Systems");
	TCase
		* tc_lsys = tcase_create ("L-Systems");
	tcase_add_test (tc_lsys, test_lsystem_create);
	tcase_add_test (tc_lsys, test_lsystem_productions);
	tcase_add_test (tc_lsys, test_lsystem_iterate);
	suite_add_tcase (s, tc_lsys);
	return s;
}

int main () {
  int number_failed = 0;
  SRunner * sr = srunner_create (make_suite ());

  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0)
    ? EXIT_SUCCESS
    : EXIT_FAILURE;
}
