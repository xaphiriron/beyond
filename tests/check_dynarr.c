#include <check.h>

#include <assert.h>
#include <stdlib.h>

#include "../src/dynarr.h"

Dynarr
	da = NULL;
DynIterator
	iterator = NULL;

void init_setup ()
{
	da = dynarr_create (4, 1);
}

void init_teardown ()
{
	dynarr_destroy (da);
	da = NULL;
}

START_TEST (test_dynarr_create)
{
	fail_unless (da != NULL);
	fail_unless (dynarr_size (da) == 0);
	fail_unless (dynarr_capacity (da) >= 4);
	fail_unless (dynarr_isEmpty (da) == TRUE);
}
END_TEST

/***
 * Invalid Initialization tests
 */

START_TEST (test_dynarr_negative_capacity)
{
	da = dynarr_create (-1, 1);
	fail_unless (da == NULL);
}
END_TEST

START_TEST (test_dynarr_zero_capacity)
{
	da = dynarr_create (0, 1);
	fail_unless (da == NULL);
}
END_TEST

START_TEST (test_dynarr_zero_size)
{
	da = dynarr_create (4, 0);
	fail_unless (da == NULL);
}
END_TEST

/***
 * Assigning tests
 */

void assign_setup ()
{
	da = dynarr_create (4, sizeof (int));
}

void assign_teardown ()
{
	dynarr_destroy (da);
	da = NULL;
}

START_TEST (test_dynarr_assign)
{
	int
		val = 61,
		check;
	dynarr_assign (da, 0, val);
	check = *(char *)dynarr_at (da, 0);
	fail_unless (val == check);
	fail_unless (!dynarr_isEmpty (da));
}
END_TEST

START_TEST (test_dynarr_assign_size)
{
	dynarr_assign (da, 0, 89);
	fail_unless (dynarr_size (da) == 1);
	dynarr_assign (da, 0, 90);
	fail_unless (dynarr_size (da) == 1);
	dynarr_assign (da, 1, 91);
	fail_unless (dynarr_size (da) == 2);
	dynarr_assign (da, 2, 92);
	fail_unless (dynarr_size (da) == 3);
	dynarr_assign (da, 8, 93);
	fail_unless (dynarr_size (da) == 4);
}
END_TEST

START_TEST (test_dynarr_negative_assign)
{
	char
		val = 62,
		check = 0;
	dynarr_assign (da, -20, val);
	check = *(char *)dynarr_at (da, -20);
	fail_unless (check == 0);
}
END_TEST

START_TEST (test_dynarr_assign_pastcapacity)
{
	int
		check;
	dynarr_assign (da, 64, 256);
	check = *(int *)dynarr_at (da, 64);
	fail_unless (check == 256);
	fail_unless (dynarr_capacity (da) >= 64);
}
END_TEST

START_TEST (test_dynarr_assign_full)
{
	int
		i = 0,
		check,
		capacity;
	capacity = dynarr_capacity (da);
	while (i <= capacity)
	{
		dynarr_assign (da, i, i);
		i++;
	}
	while (i > 0)
	{
		--i;
		check = *(int *)dynarr_at (da, i);
		fail_unless
		(
			check == i,
			"Expecting %d, got %d",
			i, check
		);
	}
	fail_unless
	(
		dynarr_size (da) <= dynarr_capacity (da)
	);
	fail_unless
	(
		dynarr_capacity (da) > capacity,
		"An array entirely filled must enlarge its capacity. (Got %d, expecting > %d)",
		dynarr_capacity (da), capacity
	);
}
END_TEST

/***
 * Removing tests
 */

void remove_setup ()
{
	da = dynarr_create (2, sizeof (int));
	dynarr_assign (da, 0, 5);
	dynarr_assign (da, 1, 4);
	dynarr_assign (da, 2, 3);
	dynarr_assign (da, 3, 2);
	dynarr_assign (da, 4, 1);
	dynarr_assign (da, 5, 0);
}

void remove_teardown ()
{
	dynarr_destroy (da);
	da = NULL;
}

START_TEST (test_dynarr_remove_size)
{
	assert (dynarr_size (da) == 6);
	dynarr_unset (da, 3);
	fail_unless (dynarr_size (da) == 5);
	dynarr_unset (da, 4);
	fail_unless (dynarr_size (da) == 4);
	dynarr_unset (da, 0);
	dynarr_unset (da, 1);
	dynarr_unset (da, 2);
	dynarr_unset (da, 5);
	fail_unless (dynarr_size (da) == 0);
}
END_TEST

START_TEST (test_dynarr_remove_at)
{
	int
		val;
	dynarr_unset (da, 0);
	val = *(int *)dynarr_at (da, 0);
	fail_unless
	(
		val == 0,
		"An attempt to get an unset index should always return 0 (got %d instead)",
		val
	);
}
END_TEST


START_TEST (test_dynarr_clear)
{
	assert (dynarr_size (da));
	dynarr_clear (da);
	fail_unless
	(
		dynarr_size (da) == 0
	);
	fail_unless
	(
		dynarr_isEmpty (da)
	);
}
END_TEST

/***
 * Pushing & Popping tests
 */

void stack_setup ()
{
	da = dynarr_create (2, sizeof (int));
}

void stack_teardown ()
{
	dynarr_destroy (da);
	da = NULL;
}

START_TEST (test_dynarr_push_empty)
{
	int
		val;
	dynarr_push (da, 42);
	val = *(int *)dynarr_at (da, 0);
	fail_unless (val == 42);
}
END_TEST

START_TEST (test_dynarr_pop_empty)
{
	int
		val;
	val = *(int *)dynarr_pop (da);
	fail_unless (val == 0);
}
END_TEST

START_TEST (test_dynarr_pop_nonseq)
{
	int
		i,
		val;
	dynarr_assign (da, 3, 3);
	dynarr_assign (da, 6, 6);
	dynarr_assign (da, 0, 0);
	dynarr_assign (da, 1, 1);
	dynarr_assign (da, 2, 2);
	dynarr_assign (da, 4, 4);
	dynarr_assign (da, 5, 5);
	i = dynarr_size (da);
	while (i-- > 0)
	{
		val = *(int *)dynarr_pop (da);
		fail_unless
		(
			val == i,
			"Expecting %d; got %d",
			i, val
		);
	}
}
END_TEST

START_TEST (test_dynarr_push_nonseq)
{
	int
		i,
		val;
	dynarr_assign (da, 3, 3);
	dynarr_push (da, 4);
	dynarr_push (da, 5);
	dynarr_push (da, 6);
	dynarr_push (da, 7);
	dynarr_push (da, 8);
	dynarr_assign (da, 2, 2);
	dynarr_assign (da, 1, 1);
	dynarr_assign (da, 0, 0);
	i = dynarr_size (da);
	while (i-- > 0)
	{
		val = *(int *)dynarr_pop (da);
		fail_unless
		(
			val == i,
			"Expecting %d; got %d",
			i, val
		);
	}
}
END_TEST

START_TEST (test_dynarr_front)
{
	int
		check;
	dynarr_assign (da, 5, 5);
	dynarr_assign (da, 2, 2);
	dynarr_assign (da, 7, 7);
	dynarr_assign (da, 3, 3);
	check = *(int *)dynarr_front (da);
	fail_unless
	(
		check == 2
	);
}
END_TEST

START_TEST (test_dynarr_back)
{
	int
		check;
	dynarr_assign (da, 5, 5);
	dynarr_assign (da, 2, 2);
	dynarr_assign (da, 7, 7);
	dynarr_assign (da, 3, 3);
	check = *(int *)dynarr_back (da);
	fail_unless
	(
		check == 7
	);
}
END_TEST

/***
 * Searching tests
 */

static int alpha_search (const void * keyp, const void * datum)
{
	char
		key;
	memcpy (&key, keyp, 1);
	return key - *(char *)datum;
}

static int reverse_alpha_sort (const void * a, const void * b)
{
	return *(char *)b - *(char *)a;
}


void search_setup ()
{
	da = dynarr_create (2, 1);
	dynarr_assign (da, 2, 'C');
	dynarr_assign (da, 5, 'F');
	dynarr_assign (da, 6, 'G');
	dynarr_assign (da, 3, 'D');
	dynarr_assign (da, 4, 'E');
	dynarr_assign (da, 0, 'A');
	dynarr_assign (da, 1, 'B');
	dynarr_push (da, 'H');
	dynarr_push (da, 'I');
	dynarr_push (da, 'J');
	dynarr_push (da, 'K');
	dynarr_push (da, 'L');
	dynarr_push (da, 'M');
	dynarr_push (da, 'N');
}

void search_teardown ()
{
	dynarr_destroy (da);
	da = NULL;
}

START_TEST (test_dynarr_in)
{
	int
		index;
	char
		check;
	index = in_dynarr (da, 'A');
	check = *(char *)dynarr_at (da, index);
	fail_unless (check == 'A');
	index = in_dynarr (da, 'a');
	fail_unless (index < 0);
	index = in_dynarr (da, 'N');
	check = *(char *)dynarr_at (da, index);
	fail_unless (check == 'N');
	index = in_dynarr (da, 'H');
	check = *(char *)dynarr_at (da, index);
	fail_unless (check == 'H');
}
END_TEST

START_TEST (test_dynarr_search)
{
	char
		check;
	check = *(char *)dynarr_search (da, alpha_search, 'F');
	fail_unless (check == 'F');
	check = *(char *)dynarr_search (da, alpha_search, 'Z');
	fail_unless (check == 0);
}
END_TEST

START_TEST (test_dynarr_sort)
{
	char
		check,
		i = 0;
	dynarr_sort (da, reverse_alpha_sort);
	while (i < 13)
	{
		check = *(char *)dynarr_at (da, i);
		fail_unless
		(
			check == 'N' - i,
			"At index %d: Expecting \'%c\'; got \'%c\'",
			i, 'N' - i, check
		);
		i++;
	}
}
END_TEST

START_TEST (test_dynarr_condense)
{
	char
		check;
	dynarr_unset (da, 1);
	dynarr_unset (da, 3);
	dynarr_unset (da, 5);
	dynarr_unset (da, 7);
	dynarr_unset (da, 9);
	dynarr_unset (da, 11);
	dynarr_unset (da, 13);
	assert (dynarr_size (da) == 7);
	dynarr_condense (da);
	fail_unless (dynarr_size (da) == 7);
	check = *(char *)dynarr_at (da, 12);
	fail_unless (check == 0);
	check = *(char *)dynarr_at (da, 6);
	fail_unless (check == 'M');
}
END_TEST

START_TEST (test_dynarr_sort_condense)
{
	char
		check;
	dynarr_assign (da, 22, 'Z');
	dynarr_assign (da, 25, 'Y');
	dynarr_sort (da, reverse_alpha_sort);
	check = *(char *)dynarr_at (da, 0);
	fail_unless
	(
		check == 'Z',
		"Expecting \'%c\'; got \'%c\'",
		'Z', check
	);
}
END_TEST

/***
 * Iteration tests
 */

void iterate_setup ()
{
	da = dynarr_create (2, sizeof (int));
	iterator = dynIterator_create (da);
}

void iterate_teardown ()
{
	dynIterator_destroy (iterator);
	dynarr_destroy (da);
	iterator = NULL;
	da = NULL;
}

START_TEST (test_dynarr_iterate_empty)
{
	fail_unless
	(
		dynIterator_done (iterator),
		"Iteration over an empty array must report done-ness before any indices are returned."
	);
}
END_TEST

START_TEST (test_dynarr_iterate_seq)
{
	int
		check,
		i = 0;
	dynarr_push (da, 0);
	dynarr_push (da, 1);
	dynarr_push (da, 2);
	dynarr_push (da, 3);
	dynarr_push (da, 4);
	dynarr_push (da, 5);
	dynarr_push (da, 6);
	dynarr_push (da, 7);
	dynarr_push (da, 8);
	dynarr_push (da, 9);
	dynarr_push (da, 10);
	while (i <= 10)
	{
		check = *(char *)dynIterator_next (iterator);
		fail_unless
		(
			check == i,
			"An iterator must return the values of the array in index order. (Expecting %d; got %d)",
			i, check
		);
		i++;
	}
	check = *(char *)dynIterator_next (iterator);
	fail_unless (check == 0);
}
END_TEST

START_TEST (test_dynarr_iterate_nonseq)
{
	int
		check,
		i = 0;
	dynarr_assign (da, 4, 2);
	dynarr_assign (da, 9, 6);
	dynarr_assign (da, 11, 7);
	dynarr_assign (da, 5, 3);
	dynarr_assign (da, 6, 4);
	dynarr_assign (da, 58, 10);
	dynarr_assign (da, 8, 5);
	dynarr_assign (da, 13, 8);
	dynarr_assign (da, 2, 1);
	dynarr_assign (da, 0, 0);
	dynarr_assign (da, 15, 9);
	while (i <= 10)
	{
		check = *(char *)dynIterator_next (iterator);
		fail_unless
		(
			check == i,
			"An iterator must return the values of the array in index order. (Expecting %d; got %d)",
			i, check
		);
		i++;
	}
	check = *(char *)dynIterator_next (iterator);
	fail_unless (check == 0);
}
END_TEST

START_TEST (test_dynarr_iterate_alter)
{
	int
		check,
		i = 0;
	dynarr_push (da, 7);
	dynarr_push (da, 7);
	dynarr_push (da, 7);
	while ((check = *(char *)dynIterator_next (iterator)) != 0)
	{
		dynarr_push (da, 7);
		i++;
	}
	fail_unless
	(
		i == 3,
		"An iterator must iterate over every entry set at the first iterator call, and must NOT iterate over any entries added after the first iterator call. (Should have iterated over 3 entries, instead iterated over %d)",
		i
	);
}
END_TEST

START_TEST (test_dynarr_iterate_reset)
{
	int
		check;
	dynarr_push (da, 0);
	dynarr_push (da, 1);
	dynarr_push (da, 2);
	assert ((check = *(char *)dynIterator_next (iterator)) == 0);
	assert ((check = *(char *)dynIterator_next (iterator)) == 1);
	dynIterator_reset (iterator);
	check = *(char *)dynIterator_next (iterator);
	fail_unless (check == 0);
	check = *(char *)dynIterator_next (iterator);
	fail_unless (check == 1);
}
END_TEST

/***
 * Type Polymorphism tests
 */

START_TEST (test_dynarr_assign_char)
{
	char
		check = 0;
	da = dynarr_create (4, 1);
	dynarr_assign (da, 0, 'q');
	check = *(char *)dynarr_at (da, 0);
	fail_unless (check == 'q');
	dynarr_destroy (da);
}
END_TEST

START_TEST (test_dynarr_assign_short)
{
	short
		check = 0;
	da = dynarr_create (4, sizeof (short));
	dynarr_assign (da, 0, 64);
	check = *(short *)dynarr_at (da, 0);
	fail_unless (check == 64);
	dynarr_destroy (da);
}
END_TEST

START_TEST (test_dynarr_assign_int)
{
	int
		check = 0;
	da = dynarr_create (4, sizeof (int));
	dynarr_assign (da, 0, 65536);
	check = *(int *)dynarr_at (da, 0);
	fail_unless (check == 65536);
	dynarr_destroy (da);
}
END_TEST

START_TEST (test_dynarr_assign_floatptr)
{
	float
		* val = malloc (sizeof (float)),
		* check = NULL;
	*val = 256.00390625;
	da = dynarr_create (4, sizeof (float *));
	dynarr_assign (da, 0, val);
	check = *(float **)dynarr_at (da, 0);
	fail_unless (check == val);
	fail_unless (*check == 256.00390625);
	dynarr_destroy (da);
	free (val);
}
END_TEST


Suite * make_dynarr_suite (void)
{
	Suite
		* s = suite_create ("Dynamic Arrays");
	TCase
		* tc_init = tcase_create ("Initialization"),
		* tc_invini = tcase_create ("Invalid Initialization"),
		* tc_assign = tcase_create ("Assigning"),
		* tc_remove = tcase_create ("Removing"),
		* tc_stack = tcase_create ("Pushing & Popping"),
		* tc_search = tcase_create ("Searching"),
		* tc_iterate = tcase_create ("Iteration"),
		* tc_poly = tcase_create ("Type Polymorphism");

	tcase_add_checked_fixture (tc_init, init_setup, init_teardown);
	tcase_add_test (tc_init, test_dynarr_create);
	suite_add_tcase (s, tc_init);

	tcase_add_test (tc_invini, test_dynarr_negative_capacity);
	tcase_add_test (tc_invini, test_dynarr_zero_capacity);
	tcase_add_test (tc_invini, test_dynarr_zero_size);
	suite_add_tcase (s, tc_invini);

	tcase_add_checked_fixture (tc_assign, assign_setup, assign_teardown);
	tcase_add_test (tc_assign, test_dynarr_assign);
	tcase_add_test (tc_assign, test_dynarr_negative_assign);
	tcase_add_test (tc_assign, test_dynarr_assign_size);
	tcase_add_test (tc_assign, test_dynarr_assign_pastcapacity);
	tcase_add_test (tc_assign, test_dynarr_assign_full);
	suite_add_tcase (s, tc_assign);

	tcase_add_checked_fixture (tc_remove, remove_setup, remove_teardown);
	tcase_add_test (tc_remove, test_dynarr_remove_size);
	tcase_add_test (tc_remove, test_dynarr_remove_at);
	tcase_add_test (tc_remove, test_dynarr_clear);
	suite_add_tcase (s, tc_remove);

	tcase_add_checked_fixture (tc_stack, stack_setup, stack_teardown);
	tcase_add_test (tc_stack, test_dynarr_push_empty);
	tcase_add_test (tc_stack, test_dynarr_pop_empty);
	tcase_add_test (tc_stack, test_dynarr_pop_nonseq);
	tcase_add_test (tc_stack, test_dynarr_push_nonseq);
	tcase_add_test (tc_stack, test_dynarr_front);
	tcase_add_test (tc_stack, test_dynarr_back);
	suite_add_tcase (s, tc_stack);

	tcase_add_checked_fixture (tc_search, search_setup, search_teardown);
	tcase_add_test (tc_search, test_dynarr_in);
	tcase_add_test (tc_search, test_dynarr_search);
	tcase_add_test (tc_search, test_dynarr_sort);
	tcase_add_test (tc_search, test_dynarr_condense);
	tcase_add_test (tc_search, test_dynarr_sort_condense);
	suite_add_tcase (s, tc_search);

	tcase_add_checked_fixture (tc_iterate, iterate_setup, iterate_teardown);
	tcase_add_test (tc_iterate, test_dynarr_iterate_empty);
	tcase_add_test (tc_iterate, test_dynarr_iterate_seq);
	tcase_add_test (tc_iterate, test_dynarr_iterate_nonseq);
	tcase_add_test (tc_iterate, test_dynarr_iterate_alter);
	tcase_add_test (tc_iterate, test_dynarr_iterate_reset);
	suite_add_tcase (s, tc_iterate);

	/* floats and doubles: officially not supported.
	 */
	tcase_add_test (tc_poly, test_dynarr_assign_char);
	tcase_add_test (tc_poly, test_dynarr_assign_short);
	tcase_add_test (tc_poly, test_dynarr_assign_int);
	tcase_add_test (tc_poly, test_dynarr_assign_floatptr);
	suite_add_tcase (s, tc_poly);

	return s;
}

int main () {
  int number_failed = 0;
  SRunner * sr = srunner_create (make_dynarr_suite ());

  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0)
    ? EXIT_SUCCESS
    : EXIT_FAILURE;
}
