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

	wp = wp_create ('b', 0, 0, 0);
	neighbors = wp_adjacent (wp, 0);
	while (i < 6)
	{
		pole = wp_getPole (neighbors[i]);
		//wp_print (neighbors[i]);
		fail_unless (
			pole == (i % 2 ? 'c' : 'a'),
			"Expected the %d-th directional offset to be pole '%c', but instead was '%c'.",
			i, (i % 2 ? 'c' : 'a'), pole
		);
		i++;
	}
	wp_destroyAdjacent (neighbors);
	wp_destroy (wp);

	wp = wp_create ('c', 0, 0, 0);
	neighbors = wp_adjacent (wp, 0);
	while (i < 6)
	{
		pole = wp_getPole (neighbors[i]);
		//wp_print (neighbors[i]);
		fail_unless (
			pole == (i % 2 ? 'a' : 'b'),
			"Expected the %d-th directional offset to be pole '%c', but instead was '%c'.",
			i, (i % 2 ? 'a' : 'b'), pole
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

START_TEST (test_worldpos_pole_cross)
{
	worldPosition
		o = wp_create ('a', 12, 0, 0),
		wp;
	int
		dist;
	unsigned int
		r, k, i;
	hexSystem_setRadii (1, 12);
	wp = wp_fromRelativeOffset (o, 12, 1, 0, 0);
	assert (wp_getPole (wp) == 'c');
	wp_getCoords (wp, &r, &k, &i);
	assert (r == 12 && k == 4 && i == 0);
	dist = wp_distance (o, wp, 12);
	fail_unless (
		dist == 1,
		"Distance must be calculated properly across pole edges. (Expecting 1, got %d)",
		dist
	);
	dist = wp_distance (wp, o, 12);
	fail_unless (
		dist == 1,
		"Distance calculations must be symmetric. (Expecting 1, got %d)",
		dist
	);

	wp_destroy (wp);
	wp_destroy (o);
}
END_TEST


Suite * make_worldpos_suite (void)
{
	Suite
		* s = suite_create ("World Position");
	TCase
		* tc_core = tcase_create ("Core"),
		* tc_distance = tcase_create ("Distance");
	tcase_add_test (tc_core, test_worldpos_create_valid);
	tcase_add_test (tc_core, test_worldpos_neighbors_small);
	tcase_add_test (tc_core, test_worldpos_distant_target);
	tcase_add_test (tc_core, test_worldpos_loop_target);
	suite_add_tcase (s, tc_core);

	tcase_add_test (tc_core, test_worldpos_pole_cross);
	suite_add_tcase (s, tc_distance);
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
