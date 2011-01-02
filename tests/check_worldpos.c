#include <stdlib.h>
#include <check.h>
#include "../src/world_position.h"

START_TEST (test_worldpos_create_valid)
{
	worldPosition
		wp = wp_create ('a', 0, 0, 0);
	unsigned int
		r, k, i;
	//wp_print (wp);
	fail_unless (wp != NULL);
	fail_unless (wp_getPole (wp) == 'a');
	fail_unless (wp_getCoords (wp, &r, &k, &i) == TRUE);
	fail_unless (r == 0 && k == 0 && i == 0);
	wp_destroy (wp);
}
END_TEST

START_TEST (test_worldpos_neighbors_small)
{
	worldPosition
		wp = wp_create ('a', 0, 0, 0),
		* neighbors = NULL;
	unsigned char
		i = 0,
		pole;
	neighbors = wp_adjacent (wp, 0);
	//wp_print (wp);
	while (i < 6)
	{
		pole = wp_getPole (neighbors[i]);
		//wp_print (neighbors[i]);
		fail_unless (
			pole == (i % 2 ? 'b' : 'c'),
			"Expected the %d-th directional offset to be pole '%c', but instead was '%c'.",
			i, (i % 2 ? 'b' : 'c'), pole
		);
		i++;
	}
	wp_destroyAdjacent (neighbors);
	wp_destroy (wp);
}
END_TEST

START_TEST (test_worldpos_distant_target)
{
	worldPosition
		wp = wp_create ('a', 0, 0, 0),
		target;
	unsigned char
		pole;
	unsigned int
		r, k, i;
	target = wp_fromRelativeOffset (wp, 3, 6, 0, 0);
	//wp_print (target);
	pole = wp_getPole (target);
	wp_getCoords (target, &r, &k, &i);
	fail_unless (
		pole == 'c' && r == 3 && k == 4 && i == 2,
		"Got '%c'{%d %d %d}, was expecting 'c'{3 4 2}",
		pole, r, k, i
	);
	wp_destroy (target);

	target = wp_fromRelativeOffset (wp, 3, 7, 0, 3);
	//wp_print (target);
	pole = wp_getPole (target);
	wp_getCoords (target, &r, &k, &i);
	fail_unless (
		pole == 'c' && r == 0 && k == 0 && i == 0,
		"Got '%c'{%d %d %d}, was expecting 'c'{0 0 0}",
		pole, r, k, i
	);
	wp_destroy (target);

	target = wp_fromRelativeOffset (wp, 3, 7, 5, 3);
	//wp_print (target);
	pole = wp_getPole (target);
	wp_getCoords (target, &r, &k, &i);
	fail_unless (
		pole == 'b' && r == 0 && k == 0 && i == 0,
		"Got '%c'{%d %d %d}, was expecting 'b'{0 0 0}",
		pole, r, k, i
	);
	wp_destroy (target);

	wp_destroy (wp);
}
END_TEST

START_TEST (test_worldpos_loop_target)
{
	worldPosition
		wp = wp_create ('a', 0, 0, 0),
		target;
	unsigned char
		pole;
	unsigned int
		r, k, i;
	target = wp_fromRelativeOffset (wp, 0, 2, 0, 0);
	pole = wp_getPole (target);
	wp_getCoords (target, &r, &k, &i);
	fail_unless (
		pole == 'b' && r == 0 && k == 0 && i == 0,
		"Got '%c'{%d %d %d}, was expecting 'b'{0 0 0}",
		pole, r, k, i
	);
	wp_destroy (target);

	target = wp_fromRelativeOffset (wp, 0, 5, 0, 0);
	pole = wp_getPole (target);
	wp_getCoords (target, &r, &k, &i);
	fail_unless (
		pole == 'b' && r == 0 && k == 0 && i == 0,
		"Got '%c'{%d %d %d}, was expecting 'b'{0 0 0}",
		pole, r, k, i
	);
	wp_destroy (target);

	wp_destroy (wp);
}
END_TEST

Suite * make_worldpos_suite (void)
{
	Suite
		* s = suite_create ("World Position");
	TCase
		* tc_core = tcase_create ("Core");
	tcase_add_test (tc_core, test_worldpos_create_valid);
	tcase_add_test (tc_core, test_worldpos_neighbors_small);
	tcase_add_test (tc_core, test_worldpos_distant_target);
	tcase_add_test (tc_core, test_worldpos_loop_target);
	suite_add_tcase (s, tc_core);
	return s;
}

int main (void)
{
	int
		number_failed = 0;
	SRunner
		* sr = srunner_create (make_worldpos_suite ());
	srunner_run_all (sr, CK_NORMAL);
	number_failed = srunner_ntests_failed (sr);
	srunner_free (sr);
	return (number_failed == 0)
		? EXIT_SUCCESS
		: EXIT_FAILURE;
}
