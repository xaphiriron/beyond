#include <check.h>
#include <stdlib.h>
#include "../src/cpv.h"

struct large_structure {
  double f[10];
};

Vector * v = NULL;
void create_setup (void) {
  v = vector_create (4, sizeof (int));
}

void create_teardown (void) {
  vector_destroy (v);
  v = NULL;
}

START_TEST (test_vector_create) {
  fail_unless (v != NULL);
}
END_TEST

START_TEST (test_vector_size) {
  fail_unless (vector_size (v) == 0);
}
END_TEST

START_TEST (test_vector_capacity) {
  fail_unless (vector_capacity (v) == 4);
}
END_TEST

START_TEST (test_vector_empty) {
  fail_unless (vector_empty (v) == TRUE);
}
END_TEST

START_TEST (test_vector_set) {
  int
    i = 4,
    j = 0;
  Vector * v = vector_create (2, sizeof (int));
  vector_push_back (v, i);
  vector_at (j, v, 0);
  fail_unless (j == i);
  vector_destroy (v);
}
END_TEST

START_TEST (test_vector_set_large) {
  struct large_structure
    f,
    g;
  Vector * v = vector_create (2, sizeof (struct large_structure));
  int i = 0;
  while (i < 10) {
    f.f[i] = (i + 1) * 100.0;
    i++;
  }
  vector_push_back (v, f);
  vector_at (g, v, 0);

  i = 0;
  while (i < 10) {
    fail_unless (f.f[i] == g.f[i]);
    i++;
  }
  vector_destroy (v);
}
END_TEST

START_TEST (test_vector_set_size) {
  Vector * v = vector_create (50, sizeof (int));
  int
    i = 0,
    j = 0;
  while (i < 50) {
    vector_push_back (v, ++i);
    vector_back (j, v);
    fail_unless (
      vector_size (v) == j,
      "vector_size must return the number of items stored in the vector. (Expecting %d, got %d)", j, vector_size (v)
    );
  }
  vector_destroy (v);
}
END_TEST

START_TEST (test_vector_set_resize) {
  Vector * v = vector_create (1, 1);
  char
    l = 'a',
    m = 0;
  while (l <= 'z') {
    vector_push_back (v, l++);
  }
  fail_unless (
    vector_size (v) == 26,
    "Vectors must resize themselves as more values are placed into them. (Expecting 26, got %d)", vector_size (v)
  );
  fail_unless (
    vector_capacity (v) >= 26,
    "Vectors must allocate further memory space as values are placed inside them (Expecting at least 26, got %d)", vector_capacity (v)
  );
  while (l > 'a') {
    vector_pop_back (m, v);
    l--;
    fail_unless (
      m == l,
      "Vectors must pop values off in the inverse order they were pushed. (Expecting '%d', got '%d')", l, m);
  }
  vector_destroy (v);
}
END_TEST

START_TEST (test_vector_set_capacity) {
  Vector * v = vector_create (4, sizeof (long));
  vector_push_back (v, 1);
  vector_push_back (v, 1 * 3);
  vector_push_back (v, 1 * 3 * 3);
  vector_push_back (v, 1 * 3 * 3 * 3);
  vector_push_back (v, 1 * 3 * 3 * 3 * 3);
  fail_unless (
    vector_capacity (v) > 5 &&
    vector_size (v) < vector_capacity (v),
    "Vectors must enlarge their capacity when filled past overflow."
  );
}
END_TEST

START_TEST (test_vector_clear) {
  Vector * v = vector_create (12, sizeof (float));
  float m = 1.00;
  while (m <= 2048.0) {
    vector_push_back (v, m);
    m *= 2.0;
  }
  fail_unless (
    vector_size (v) == 12,
    "(expecting size of 12, got %d)", vector_size (v)
  );

  vector_clear (v);
  fail_unless (
    vector_size (v) == 0,
    "vector_clear must remove all data from the vector"
  );
}
END_TEST

// there was a memory corruption bug that occurred when an object was filled to capacity and then the final entry was removed (in this case, by vector_pop_back). the cause was an off-by-one error in the removal function that tried to wipe the memory in v->l + (v->o + 1) * v->s instead of v->l + v->o * v->s.
START_TEST (test_vector_destroy) {
  Vector * v = vector_create (2, sizeof (void *));
  void * ptr = (void *)0xffffffff;
  vector_push_back (v, ptr);
  vector_push_back (v, ptr);
  fail_unless (vector_size (v) == 2 && vector_capacity (v) == 2);
  vector_pop_back (ptr, v);
  fail_unless (vector_size (v) == 1 && vector_capacity (v) == 2);
  fail_unless (vector_at (ptr, v, vector_size (v)) == NULL);
  fail_unless (vector_at (ptr, v, vector_capacity (v)) == NULL);
  mark_point ();
  vector_destroy (v);
}
END_TEST

/*
START_TEST (test_vector_set_mistype) {
  Vector * v = vector_create (4, 1);
  int i = 0;
  vector_push_back (v, i++);
}
END_TEST
*/


// there was a bug in which the supposedly "blank" data block we keep so we always have v->s bytes of zeros to return a "NULL" for got overwritten somehow, so that a vector_pop_back (...) != NULL test would never return false and keep looping forever. In cases where the pointer was actually used, however, it never got past a single iteration: since it was garbage data (e.g., not 0xdeadbeef) read as a pointer, it would inevitably segfault.
START_TEST (test_vector_loop_final) {
  Vector * v = vector_create (4, sizeof (void *));
  void
    * ptr = (void *)0xDEADBEEF,
    * m = NULL;
  int i = 0;
  while (i < 6) {
    vector_push_back (v, ptr);
    i++;
  }
  while (vector_pop_back (m, v) != NULL) {
    //printf ("popped %p value\n", m);
    --i;
  }
  fail_unless (i == 0);
}
END_TEST

Suite * make_vector_suite (void) {
  Suite * s = suite_create ("Vector");
  TCase
    * tc_create = tcase_create ("Creation"),
    * tc_set = tcase_create ("Setting & Fetching"),
    * tc_clear = tcase_create ("Clear"),
    * tc_loop = tcase_create ("Loops & NULL values");
  tcase_add_checked_fixture (tc_create, create_setup, create_teardown);
  tcase_add_test (tc_create, test_vector_create);
  tcase_add_test (tc_create, test_vector_size);
  tcase_add_test (tc_create, test_vector_capacity);
  tcase_add_test (tc_create, test_vector_empty);
  suite_add_tcase (s, tc_create);
  tcase_add_test (tc_set, test_vector_set);
  tcase_add_test (tc_set, test_vector_set_large);
  tcase_add_test (tc_set, test_vector_set_resize);
  tcase_add_test (tc_set, test_vector_set_size);
  tcase_add_test (tc_set, test_vector_set_capacity);
  //tcase_add_exit_test (tc_set, test_vector_set_mistype, 5);
  suite_add_tcase (s, tc_set);
  tcase_add_test (tc_clear, test_vector_clear);
  tcase_add_test (tc_clear, test_vector_destroy);
  suite_add_tcase (s, tc_clear);
  tcase_add_test (tc_loop, test_vector_loop_final);
  suite_add_tcase (s, tc_loop);
  return s;
}

int main (void) {
  int number_failed = 0;
  SRunner * sr = srunner_create (make_vector_suite ());

  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0)
    ? EXIT_SUCCESS
    : EXIT_FAILURE;
}