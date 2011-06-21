#include <check.h>
#include <stdlib.h>
#include <ctype.h>

#include "../src/xph_log.h"
#include "../src/xph_memory.h"

#include "../src/map.h"
#include "../src/hex_utility.h"
#include "../src/vector.h"

void mapWorldDestroy (void)
{
	worldDestroy ();
	//xph_audit ();
}


START_TEST (mapCreateThreePoleTest)
{
	bool
		r;
	SUBHEX
		poles[3];
	LOG (E_FUNCLABEL, "%s:", __FUNCTION__);
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
	SUBHEX
		poles[3],
		child;
	unsigned int
		r = 0,
		k = 0,
		i = 0;
	signed int
		x, y;
	LOG (E_FUNCLABEL, "%s:", __FUNCTION__);
	mapSetSpanAndRadius (2, 4);
	mapGeneratePoles (POLE_TRI);
	poles[0] = mapPole ('r');
	poles[1] = mapPole ('g');
	poles[2] = mapPole ('b');
	mapForceSubdivide (poles[0]);
	while (r < 5)
	{
		hex_rki2xy (r, k, i, &x, &y);
		child = subhexData (poles[0], x, y);
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
		hex_nextValidCoord (&r, &k, &i);
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
	SUBHEX
		pole,
		spanOne;
	RELATIVEHEX
		rel;
	LOG (E_FUNCLABEL, "%s:", __FUNCTION__);
	mapSetSpanAndRadius (4, 8);
	mapGeneratePoles (POLE_TRI);
	pole = mapPole ('r');
	mapForceGrowAtLevelForDistance (pole, 1, 4);

	rel = mapRelativeSubhexWithCoordinateOffset (pole, -3, 0, 0);
	spanOne = mapRelativeTarget (rel);
	mapRelativeDestroy (rel);

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


START_TEST (mapCrossPoleTraversalTest)
{
	SUBHEX
		position,
		adjacent;
	RELATIVEHEX
		rel;
	signed int
		x, y;
	LOG (E_FUNCLABEL, "%s:", __FUNCTION__);

	mapSetSpanAndRadius (4, 8);
	mapGeneratePoles (POLE_TRI);
	mapForceSubdivide (mapPole ('r'));
	mapForceSubdivide (mapPole ('g'));
	mapForceSubdivide (mapPole ('b'));

	// get the subhex that's one level down and at the very edge of the pole's ground spread
	rel = mapRelativeSubhexWithCoordinateOffset (mapPole ('r'), -1, 8, 0);
	position = mapRelativeTarget (rel);
	mapRelativeDestroy (rel);
	// and then get the subhex that's one position more distant, which ought to cross the pole boundary
	rel = mapRelativeSubhexWithCoordinateOffset (position, 0, 1, 0);
	adjacent = mapRelativeTarget (rel);
	mapRelativeDestroy (rel);
	fail_unless
	(
		adjacent != NULL
	);
	fail_unless
	(
		subhexPoleName (adjacent) == 'b',
		"Expected adjacent subhex to be inside the blue pole, but instead got a pole value of '%c'",
		subhexPoleName (adjacent)
	);
	fail_unless
	(
		subhexParent (adjacent) == mapPole ('b'),
		"Expected adjacent subhex's direct parent to be the blue pole, but instead got subhex %p (which isn't)",
		subhexParent (adjacent)
	);
	subhexLocalCoordinates (adjacent, &x, &y);
	fail_unless
	(
		x == 0 && y == -8,
		"Expected adjacent subhex to be at coordinates 0, -8; instead got %d,%d.",
		x, y
	);
}
END_TEST


static char
	pTopPoles[]	= {'r', 'g', 'b',};
static signed int
	pTopGoal[]	= {0,  2, 1, 2, 1, 2, 1,  1, 0, 2, 0, 1, 0, 2, 0, 1, 0, 2, 0},
	pTopX[]		= {0,  1, 0,-1,-1, 0, 1,  2, 1, 0,-1,-2,-2,-2,-1, 0, 1, 2, 2},
	pTopY[]		= {0,  0, 1, 1, 0,-1,-1,  0, 1, 2, 2, 2, 1, 0,-1,-2,-2,-2,-1};

START_TEST (mapPoleTraversalTopology)
{
	SUBHEX
		targetPole,
		calc;
	RELATIVEHEX
		rel;
	unsigned short
		i = _i % 19,
		poleID = _i / 19;
	char
		poleStart = pTopPoles[poleID],
		poleGoal = pTopPoles[(poleID + pTopGoal[i]) % 3];

	LOG (E_FUNCLABEL, "%s [%d (%d:%d)]:", __FUNCTION__, _i, poleID, i);
	
	mapSetSpanAndRadius (4, 8);
	mapGeneratePoles (POLE_TRI);

	rel = mapRelativeSubhexWithCoordinateOffset
	(
		mapPole (poleGoal), 0,
		0, 0
	);
	targetPole = mapRelativeTarget (rel);
	mapRelativeDestroy (rel);

	rel = mapRelativeSubhexWithCoordinateOffset
	(
		mapPole (poleStart), 0,
		pTopX[i], pTopY[i]
	);
	calc = mapRelativeTarget (rel);
	mapRelativeDestroy (rel);

	fail_unless
	(
		calc == targetPole && calc == mapPole (poleGoal),
		"Movement across poles from '%c' @ %d, %d should end at pole '%c', but got '%c' instead", poleStart, pTopX[i], pTopY[i], poleGoal, subhexPoleName (calc)
	);
}
END_TEST


static char
	poleStart[]	=  {'r', 'r', 'r', 'r', 'r', 'r', 'r',
					'g', 'g', 'g', 'g', 'g', 'g', 'g',
					'b', 'b', 'b', 'b', 'b', 'b', 'b'},
	poleGoal[]	=  {'r', 'b', 'g', 'b', 'g', 'b', 'g',
					'g', 'r', 'b', 'r', 'b', 'r', 'b',
					'b', 'g', 'r', 'g', 'r', 'g', 'r'};
static signed int
	poleCI[]	=  {0, 1, 2, 3, 4, 5, 6,
					0, 1, 2, 3, 4, 5, 6,
					0, 1, 2, 3, 4, 5, 6},
	poleX[]		=  {0, 9, -8,-17, -9,  8, 17},
	poleY[]		=  {0, 8, 17,  9, -8,-17, -9};

START_TEST (mapRelativeHexExactCrossPole)
{
	SUBHEX
		targetPole,
		calc;
	RELATIVEHEX
		rel;
	signed int
		x, y;

	LOG (E_FUNCLABEL, "%s [%d]:", __FUNCTION__, _i);
	
	mapSetSpanAndRadius (4, 8);
	mapGeneratePoles (POLE_TRI);
	mapForceSubdivide (mapPole ('r'));
	mapForceSubdivide (mapPole ('g'));
	mapForceSubdivide (mapPole ('b'));

	mark_point ();

	rel = mapRelativeSubhexWithCoordinateOffset (mapPole (poleGoal[_i]), -1, 0, 0);
	targetPole = mapRelativeTarget (rel);
	mapRelativeDestroy (rel);

	mark_point ();

	rel = mapRelativeSubhexWithCoordinateOffset
	(
		mapPole (poleStart[_i]), -1,
		poleX[poleCI[_i]], poleY[poleCI[_i]]
	);
	calc = mapRelativeTarget (rel);
	mapRelativeDestroy (rel);

	mark_point ();

	subhexLocalCoordinates (calc, &x, &y);

	fail_unless
	(
		calc == targetPole && subhexPoleName (calc) == poleGoal[_i] && x == 0 && y == 0 && subhexSpanLevel (calc) == 3,
		"Movement from pole %c @ -1, %d, %d should end at the centre of pole %c. Expected '%c', 0, 0, 3; got '%c', %d, %d, %d instead.",
		toupper (poleStart[_i]), poleX[poleCI[_i]], poleY[poleCI[_i]],
		toupper (poleGoal[_i]), poleGoal[_i], subhexPoleName (calc), x, y, subhexSpanLevel (calc)
	);
}
END_TEST

/* TESTS STILL TO WRITE:
 *  - check to make sure vector distance is calculated properly, in various
 *    combinations of direction and span level
 *  - any test whatsoever with mapAdjacentSubhexes (which should return a
 *    dynarr of RELATIVEHEXes, each of which should have perfect coordinate
 *    fidelity down to the centre subhex, even if the adjacent subhexes don't
 *    exist in memory) (this is used while rendering)
 *  - a test of mapRelativeSubhexWithVectorOffset, of what i'm not sure. right
 *    now it'll fail if given a vector that condenses into a coordinate that's
 *    outside the range of the signed int type, so maybe something that tests
 *    that. this isn't super important, though, since it fails loudly and
 *    catastrophically if that happens, so i'll definitely know if it actually
 *    comes up in practice
 *  - a test of mapRelativeSubhexWithSubhex, which is actually important!
 *    since 1. the function hasn't been written yet and 2. it's used in the
 *    actual game rendering code, so it's pretty important to get it working
 *    right!!!
 *  - check to make sure mapForceGrowAtLevelForDistance works right even if
 *    the distance given is greater than the map radius (or more generally,
 *    that it works even if it has to cross subhex edges to do so -- it
 *    ultimately does its traversal calculations with
 *    mapRelativeSubhexWithCoordinateOffset, so if the test fails then it's
 *    due to that code not being right.
 */

/* TODO: a loop test for crossing at the corners of platters, that checks the remainder to see if it's the correct value. it's not finished yet - xph 2011 06 04


START_TEST (mapRelativeHexExactEdgeCross)
{

	rel = mapRelativeSubhexWithCoordinateOffset
	(
		mapPole [poleStart[_i]], -1,
		poleTopX[(_i % 6) + 1], poleTopY[(_i % 6) + 1]
	);
	calc = mapRelativeTarget (rel);
	mapRelativeDestroy (rel)

}
END_TEST
*/

START_TEST (mapPoleEdgeDirectionsPlusOne)
{
	signed int
		* edgesRG,
		* edgesGB,
		* edgesBR;
	int
		i = 0;
	LOG (E_FUNCLABEL, "%s:", __FUNCTION__);
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
	LOG (E_FUNCLABEL, "%s:", __FUNCTION__);
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
	StartX[] = {8, 0,-8,-8, 0, 8, 0},
	StartY[] = {0, 8, 8, 0,-8,-8, 0},
	MoveX[] = {1, 0,-1,-1, 0, 1},
	MoveY[] = {0, 1, 1, 0,-1,-1};
static const unsigned int
	slookup[] = {6, 6, 6, 6, 6, 6, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5},
	mlookup[] = {0, 1, 2, 3, 4, 5, 0, 5, 1, 0, 2, 1, 3, 2, 4, 3, 5, 4};

START_TEST (mapRelativeDistanceSpan0Edge)
{
	RELATIVEHEX
		start,
		adj;
	SUBHEX
		startHex;
	VECTOR3
		adjDistance,
		distance;

	logSetLevel (E_ALL);
	LOG (E_ERR, "%s [%d]:", __FUNCTION__, _i);

	mapSetSpanAndRadius (2, 8);
	mapGeneratePoles (POLE_TRI);
	mapForceGrowAtLevelForDistance (mapPole ('r'), 1, 1);

	ERROR ("GETTING INITIAL HEX (at %d, %d)", StartX[slookup[_i]], StartY[slookup[_i]]);
	start = mapRelativeSubhexWithCoordinateOffset (mapPole ('r'), -2, StartX[slookup[_i]], StartY[slookup[_i]]);
	startHex = mapRelativeTarget (start);
	fail_unless
	(
		subhexSpanLevel (startHex) == 0 && startHex != NULL,
		"Expected valid subhex with span level 0; got %p/%d instead",
		startHex, subhexSpanLevel (startHex)
	);

	ERROR ("GETTING ADJACENT HEX (at %d,%d distant from start)", MoveX[mlookup[_i]], MoveY[mlookup[_i]]);
	adj = mapRelativeSubhexWithCoordinateOffset (startHex, 0, MoveX[mlookup[_i]], MoveY[mlookup[_i]]);

	distance = mapRelativeDistance (adj);

	adjDistance = hex_xyCoord2Space (MoveX[mlookup[_i]], MoveY[mlookup[_i]]);
	fail_unless
	(
		vector_cmp (&distance, &adjDistance),
		"Got distance vector of %.2f,%.2f,%.2f; expected %.2f,%.2f,%.2f",
		distance.x, distance.y, distance.z,
		adjDistance.x, adjDistance.y, adjDistance.z
	);

	logSetLevel (E_NONE);
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
	signed int
		ux, uy;

	LOG (E_FUNCLABEL, "%s [%d]:", __FUNCTION__, _i);

	mapSetSpanAndRadius (0, 2);
	mapScaleCoordinates (1, xDown[_i], yDown[_i], &ux, &uy, NULL, NULL);
	fail_unless
	(
		ux == xUp[uGoal[_i]] && uy == yUp[uGoal[_i]],
		"Scaling up should round to the closest coordinate. Expected %d, %d, but got %d, %d.", xUp[uGoal[_i]], yUp[uGoal[_i]], ux, uy
	);
}
END_TEST

START_TEST (mapCoordinateScaleDown)
{
	signed int
		dx, dy;

	LOG (E_FUNCLABEL, "%s [%d]:", __FUNCTION__, _i);

	mapSetSpanAndRadius (0, 2);
	mapScaleCoordinates (-1, xUp[_i], yUp[_i], &dx, &dy, NULL, NULL);
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

	tcase_add_checked_fixture (tcSubdivision, NULL, mapWorldDestroy);
	tcase_add_test (tcSubdivision, mapForceSubdivideTest);
	suite_add_tcase (s, tcSubdivision);

	tcase_add_checked_fixture (tcRadialGrowth, NULL, mapWorldDestroy);
	tcase_add_test (tcRadialGrowth, mapGrowAroundSubhexTest);
	suite_add_tcase (s, tcRadialGrowth);
	return s;
}

Suite * makeMapTraversalSuite (void)
{
	Suite
		* s = suite_create ("Map Traversal");
	TCase
		* tcPole = tcase_create ("Cross-pole movement"),
		* tcCoordinates = tcase_create ("Coordinate math"),
		* tcVector = tcase_create ("Relative distance between hexes");

	tcase_add_checked_fixture (tcPole, NULL, mapWorldDestroy);
	tcase_add_loop_test (tcPole, mapPoleTraversalTopology, 0, 57);
	tcase_add_loop_test (tcPole, mapRelativeHexExactCrossPole, 0, 21);
	tcase_add_test (tcPole, mapCrossPoleTraversalTest);
	suite_add_tcase (s, tcPole);

	tcase_add_loop_test (tcCoordinates, mapCoordinateScaleUp, 0, 24);
	tcase_add_loop_test (tcCoordinates, mapCoordinateScaleDown, 0, 6);
	tcase_add_test (tcCoordinates, mapPoleEdgeDirectionsPlusOne);
	tcase_add_test (tcCoordinates, mapPoleEdgeDirectionsMinusOne);
	suite_add_tcase (s, tcCoordinates);

	// this should be 18 really
	tcase_add_loop_test (tcVector, mapRelativeDistanceSpan0Edge, 0, 18);
	suite_add_tcase (s, tcVector);

	return s;
}

int main () {
	int
		number_failed = 0;
	SRunner
		* sr = srunner_create (makeMapInitSuite ());

	srunner_add_suite (sr, makeMapTraversalSuite ());
	srunner_add_suite (sr, makeMapGenerationSuite ());
	//srunner_add_suite (sr,  ());

	logSetLevel (E_NONE);

	srunner_run_all (sr, CK_NORMAL);
	number_failed = srunner_ntests_failed (sr);
	srunner_free (sr);
	return (number_failed == 0)
		? EXIT_SUCCESS
		: EXIT_FAILURE;
}
