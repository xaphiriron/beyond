#include <check.h>
#include <stdlib.h>

#include "../src/xph_log.h"
#include "../src/xph_memory.h"

#include "../src/map.h"
#include "../src/hex_utility.h"

START_TEST (mapCreateThreePoleTest)
{
	DEBUG ("%s:", __FUNCTION__);
	bool
		r;
	SUBHEX
		poles[3];
	mapSetSpanAndRadius (4, 8);
	r = mapGeneratePoles (POLE_TRI);
	poles[0] = mapPole ('r');
	poles[1] = mapPole ('g');
	poles[2] = mapPole ('b');
	fail_unless (poles[0] != NULL);
	fail_unless (poles[1] != NULL);
	fail_unless (poles[2] != NULL);

	fail_unless
	(
		subhexSpanLevel (poles[0]) == 4,
		"Expected pole %d to have a span level of %d; got %d instead",
		0, 4, subhexSpanLevel (poles[0])
	);

	fail_unless
	(
		subhexSpanLevel (poles[1]) == 4,
		"Expected pole %d to have a span level of %d; got %d instead",
		1, 4, subhexSpanLevel (poles[1])
	);

	fail_unless
	(
		subhexSpanLevel (poles[2]) == 4,
		"Expected pole %d to have a span level of %d; got %d instead",
		2, 4, subhexSpanLevel (poles[2])
	);
}
END_TEST

START_TEST (mapForceSubdivideTest)
{
	DEBUG ("%s:", __FUNCTION__);
	SUBHEX
		poles[3],
		child;
	int
		i = hx (4);
	mapSetSpanAndRadius (2, 4);
	mapGeneratePoles (POLE_TRI);
	poles[0] = mapPole ('r');
	poles[1] = mapPole ('g');
	poles[2] = mapPole ('b');
	mapForceSubdivide (poles[0]);
	while (i > 0)
	{
		i--;
		child = subhexData (poles[0], i);
		fail_unless
		(
			child != NULL,
			"Expected child at offset %d; was NULL instead",
			i
		);
		fail_unless
		(
			subhexSpanLevel (child) == 1,
			"Expected child (at offset %d) to have span of 1; got %d instead",
			i, subhexSpanLevel (child)
		);
	}
}
END_TEST

/***
 * given a subhex and a span/magnitude offset, generate all subhexes down to
 * the specified span level around the subhex (e.g., span 1 magnitude 3 would
 * generate every span 1 subhex within 3 span 1 coordinate steps from the
 * specified subhex) -- if the subhex itself isn't generated down to the span
 * 1 level, the first step is to do that
 */
START_TEST (mapGrowAroundSubhexTest)
{
	DEBUG ("%s:", __FUNCTION__);
	SUBHEX
		pole,
		spanOne;
	mapSetSpanAndRadius (4, 8);
	mapGeneratePoles (POLE_TRI);
	pole = mapPole ('r');
	mapForceGrowAtLevelForDistance (pole, 1, 4);
	spanOne = mapGetRelativePosition (pole, 4, 0, 0);
	fail_unless
	(
		spanOne != NULL
	);
	fail_unless
	(
		subhexSpanLevel (spanOne) == 1,
		"Expected subhex to have a span level of 1, got %d instead",
		subhexSpanLevel (spanOne)
	);
	// then test to see if all coordinates <= 4 span 1 steps distant to it are filled
}
END_TEST


/***
 * things that code/tests have to be written for:
 *  - given two subhexes, calculate a direction vector (which is normalized)
 *    and a rough magnitude (which would cap out at some point as just "far")
 *    between them
 *  - given a subhex and a span/x/y offset, return the generated subhex closest
 *    (where 'closest' means 'smallest subhex that the specified position is
 *    inside') to the location, with an optional 'vagueness' parameter that
 *    would return the number of span levels above or below the target the
 *    returned subhex is
 *  - given a subhex and a span/x/y offset, return the target subhex,
 *    generating it if necessary.
 */

START_TEST (mapCrossPoleTraversalTest)
{
	DEBUG ("%s:", __FUNCTION__);
	SUBHEX
		position,
		adjacent;
	signed int
		x, y;
	mapSetSpanAndRadius (4, 8);
	mapGeneratePoles (POLE_TRI);
	mapForceSubdivide (mapPole ('r'));
	mapForceSubdivide (mapPole ('g'));
	mapForceSubdivide (mapPole ('b'));

	// get the subhex that's one level down and at the very edge of the pole's ground spread
	position = mapGetRelativePosition (mapPole ('r'), -1, 8, 0);
	// and then get the subhex that's one position more distant, which ought to cross the pole boundary
	adjacent = mapGetRelativePosition (position, 0, 1, 0);
	fail_unless
	(
		adjacent != NULL
	);
	fail_unless
	(
		subhexPoleName (adjacent) == 'g',
		"Expected adjacent subhex to be inside the green pole, but instead got a pole value of '%c'",
		subhexPoleName (adjacent)
	);
	fail_unless
	(
		subhexParent (adjacent) == mapPole ('g'),
		"Expected adjacent subhex's direct parent to be the green pole, but instead got subhex %p (which isn't)",
		subhexParent (adjacent)
	);
	subhexLocalCoordinates (adjacent, &x, &y);
	fail_unless
	(
		x == -8 && y == 8,
		"Expected adjacent subhex to be at coordinates -8,8, but instead got %d,%d.",
		x, y
	);
}
END_TEST

START_TEST (mapPoleEdgeDirectionsPlusOne)
{
	signed int
		* edgesRG,
		* edgesGB,
		* edgesBR;
	int
		i = 0;
	mapGeneratePoles (POLE_TRI);
	edgesRG = mapPoleConnections ('r', 'g'),
	edgesGB = mapPoleConnections ('g', 'b'),
	edgesBR = mapPoleConnections ('b', 'r');
	while (edgesRG[i] != -1)
	{
		switch (i)
		{
			case 0:
				fail_unless
				(
					edgesRG[i] == 1,
					"Expected first pole connection to be along edge 1; instead got %d",
					edgesRG[i]
				);
				break;
			case 1:
				fail_unless
				(
					edgesRG[i] == 3,
					"Expected second pole connection to be along edge 3; instead got %d",
					edgesRG[i]
				);
				break;
			case 2:
				fail_unless
				(
					edgesRG[i] == 5,
					"Expected second pole connection to be along edge 5; instead got %d",
					edgesRG[i]
				);
				break;
			default:
				fail ("There should be only three pole connections between any two poles, but instead there were more (in this case R->G)");
				break;
		}
		i++;
	}
	fail_unless
	(
		memcmp (edgesRG, edgesGB, 4) == 0,
		"The R->G pole connection directions ought to be the same as the G->B pole connection directions"
	);
	fail_unless
	(
		memcmp (edgesGB, edgesBR, 4) == 0,
		"The G->B pole connection directions ought to be the same as the B->R pole connection directions"
	);

	xph_free (edgesRG);
	xph_free (edgesGB);
	xph_free (edgesBR);
}
END_TEST


START_TEST (mapPoleEdgeDirectionsMinusOne)
{
	signed int
		* edgesRB,
		* edgesGR,
		* edgesBG;
	int
		i = 0;
	mapGeneratePoles (POLE_TRI);
	edgesRB = mapPoleConnections ('r', 'b'),
	edgesGR = mapPoleConnections ('g', 'r'),
	edgesBG = mapPoleConnections ('b', 'g');
	while (edgesRB[i] != -1)
	{
		switch (i)
		{
			case 0:
				fail_unless
				(
					edgesRB[i] == 0,
					"Expected first pole connection to be along edge 0; instead got %d",
					edgesRB[i]
				);
				break;
			case 1:
				fail_unless
				(
					edgesRB[i] == 2,
					"Expected second pole connection to be along edge 2; instead got %d",
					edgesRB[i]
				);
				break;
			case 2:
				fail_unless
				(
					edgesRB[i] == 4,
					"Expected third pole connection to be along edge 4; instead got %d",
					edgesRB[i]
				);
				break;
			default:
				fail ("There should be only three pole connections between any two poles, but instead there were more (in this case R->B)");
				break;
		}
		i++;
	}
	fail_unless
	(
		memcmp (edgesRB, edgesGR, 4) == 0,
		"The R->B pole connection directions ought to be the same as the G->R pole connection directions"
	);
	fail_unless
	(
		memcmp (edgesGR, edgesBG, 4) == 0,
		"The G->R pole connection directions ought to be the same as the B->G pole connection directions"
	);

	xph_free (edgesRB);
	xph_free (edgesGR);
	xph_free (edgesBG);
}
END_TEST


static const signed int
	xDown[]	= {0, 3,-2,-5,-3, 2, 5,
				2, 3, 3,  0, 0, 1, -2,-3,-2, -2,-3,-3,  0, 0,-1,  2, 3, 2},
	yDown[]	= {0, 2, 5, 3,-2,-5,-3,
				0, 0,-1,  2, 3, 2,  2, 3, 3,  0, 0, 1, -2,-3,-2, -2,-3,-3},
	uGoal[]	= {0, 1, 2, 3, 4, 5, 6, 
				0, 1, 6,  0, 2, 1,  0, 3, 2,  0, 4, 3,  0, 5, 4,  0, 6, 5},

	xUp[]	= {0, 1, 0,-1,-1, 0, 1},
	yUp[]	= {0, 0, 1, 1, 0,-1,-1},
	dGoal[]	= {0, 1, 2, 3, 4, 5, 6};
	

START_TEST (mapCoordinateScaleUp)
{
	DEBUG ("%s:", __FUNCTION__);
	signed int
		ux, uy;
	mapSetSpanAndRadius (0, 2);
	mapScaleCoordinates (1, xDown[_i], yDown[_i], &ux, &uy);
	fail_unless
	(
		ux == xUp[uGoal[_i]] && uy == yUp[uGoal[_i]],
		"Scaling up should round to the closest coordinate. Expected %d, %d, but got %d, %d.", xUp[uGoal[_i]], yUp[uGoal[_i]], ux, uy
	);
}
END_TEST

START_TEST (mapCoordinateScaleDown)
{
	DEBUG ("%s:", __FUNCTION__);
	signed int
		dx, dy;
	mapSetSpanAndRadius (0, 2);
	mapScaleCoordinates (-1, xUp[_i], yUp[_i], &dx, &dy);
	fail_unless
	(
		dx == xDown[dGoal[_i]] && dy == yDown[dGoal[_i]],
		"Invalid downwards scale. Values %d, %d expanded one down should be %d, %d, but got %d, %d instead.", xUp[_i], yUp[_i], xDown[dGoal[_i]], yDown[dGoal[_i]], dx, dy
	);
}
END_TEST


Suite * makeMapInitSuite (void)
{
	Suite
		* s = suite_create ("Map Creation");
	TCase
		* tcThreePole = tcase_create ("Tri-pole creation");

	//tcase_add_checked_fixture (, , );
	tcase_add_test (tcThreePole, mapCreateThreePoleTest);
	suite_add_tcase (s, tcThreePole);

	return s;
}

Suite * makeMapGenerationSuite (void)
{
	Suite
		* s = suite_create ("Map Generation and subdivision");
	TCase
		* tcSubdivision = tcase_create ("Forced subhex subdivision"),
		* tcRadialGrowth = tcase_create ("Subdividing around subhexes");

	tcase_add_test (tcSubdivision, mapForceSubdivideTest);
	suite_add_tcase (s, tcSubdivision);

	tcase_add_test (tcRadialGrowth, mapGrowAroundSubhexTest);
	suite_add_tcase (s, tcRadialGrowth);
	return s;
}

Suite * makeMapTraversalSuite (void)
{
	Suite
		* s = suite_create ("Map Traversal");
	TCase
		* tcMovement = tcase_create ("Cross-pole movement"),
		* tcCoordinates = tcase_create ("Coordinate math");

	tcase_add_test (tcMovement, mapCrossPoleTraversalTest);
	tcase_add_test (tcMovement, mapPoleEdgeDirectionsPlusOne);
	tcase_add_test (tcMovement, mapPoleEdgeDirectionsMinusOne);
	suite_add_tcase (s, tcMovement);

	tcase_add_loop_test (tcCoordinates, mapCoordinateScaleUp, 0, 24);
	tcase_add_loop_test (tcCoordinates, mapCoordinateScaleDown, 0, 6);
	suite_add_tcase (s, tcCoordinates);
	return s;
}

int main () {
	int
		number_failed = 0;
	SRunner
		* sr = srunner_create (makeMapInitSuite ());

	srunner_add_suite (sr, makeMapGenerationSuite ());
	srunner_add_suite (sr, makeMapTraversalSuite ());
	//srunner_add_suite (sr,  ());

	logSetLevel (E_ALL);

	srunner_run_all (sr, CK_NORMAL);
	number_failed = srunner_ntests_failed (sr);
	srunner_free (sr);
	return (number_failed == 0)
		? EXIT_SUCCESS
		: EXIT_FAILURE;
}
