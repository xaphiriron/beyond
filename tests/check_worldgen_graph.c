//#define MEM_DEBUG

#include <check.h>
#include "../src/worldgen_graph.h"
#include "../src/xph_memory.h"

GRAPH
	g = NULL;
void
	* e = NULL,
	* v = NULL;

/***
 * GRAPH INITIALIZATION
 */

void graphBlankCreate ()
{
	g = worldgenCreateBlankRegionGraph ();
}

START_TEST (graphCreateTest)
{
	fail_unless (g != NULL);
}
END_TEST

void graphBlankCreateDestroy ()
{
	g = worldgenCreateBlankRegionGraph ();
	e = graphGetRawEdges (g);
	v = graphGetRawVertices (g);
	graphDestroy (g);
}

START_TEST (graphDestroyMainGraphTest)
{
	DEBUG ("%s", __FUNCTION__);
	fail_unless (!xph_isallocated (g));
}
END_TEST

START_TEST (graphDestroyEdgesTest)
{
	DEBUG ("%s", __FUNCTION__);
	fail_unless (e != NULL && !xph_isallocated (e));
}
END_TEST

START_TEST (graphDestroyVerticesTest)
{
	DEBUG ("%s", __FUNCTION__);
	fail_unless (v != NULL && !xph_isallocated (v));
}
END_TEST

/***
 * graphWorldBase
 */

START_TEST (graphWorldBaseInvalidRBaseTest)
{
	DEBUG ("%s", __FUNCTION__);
	graphWorldBase (g, GRAPH_POLE_R);
	fail_unless (graphVertexCount (g) == 0);
}
END_TEST

START_TEST (graphWorldBaseInvalidGBaseTest)
{
	DEBUG ("%s", __FUNCTION__);
	graphWorldBase (g, GRAPH_POLE_G);
	fail_unless (graphVertexCount (g) == 0);
}
END_TEST

START_TEST (graphWorldBaseInvalidBBaseTest)
{
	DEBUG ("%s", __FUNCTION__);
	graphWorldBase (g, GRAPH_POLE_B);
	fail_unless (graphVertexCount (g) == 0);
}
END_TEST

START_TEST (graphWorldBaseInitializeRGTest)
{
	DEBUG ("%s", __FUNCTION__);
	graphWorldBase (g, GRAPH_POLE_R | GRAPH_POLE_G);
/*
	fail_unless
	(
		graphRegionCount (g) == 1,
		"Expecting region count of 1, got %d instead",
		graphRegionCount (g)
	);
*/
	fail_unless (graphVertexCount (g) == 2);
	fail_unless (graphEdgeCount (g) == 3);
}
END_TEST

START_TEST (graphWorldBaseInitializeRBTest)
{
	DEBUG ("%s", __FUNCTION__);
	graphWorldBase (g, GRAPH_POLE_R | GRAPH_POLE_B);
/*
	fail_unless
	(
		graphRegionCount (g) == 1,
		"Expecting region count of 1, got %d instead",
		graphRegionCount (g)
	);
*/
	fail_unless (graphVertexCount (g) == 2);
	fail_unless (graphEdgeCount (g) == 3);
}
END_TEST

START_TEST (graphWorldBaseInitializeGBTest)
{
	DEBUG ("%s", __FUNCTION__);
	graphWorldBase (g, GRAPH_POLE_G | GRAPH_POLE_B);
/*
	fail_unless
	(
		graphRegionCount (g) == 1,
		"Expecting region count of 1, got %d instead",
		graphRegionCount (g)
	);
*/
	fail_unless (graphVertexCount (g) == 2);
	fail_unless (graphEdgeCount (g) == 3);
}
END_TEST

START_TEST (graphWorldBaseInitializeRGBTest)
{
	DEBUG ("%s", __FUNCTION__);
	graphWorldBase (g, GRAPH_POLE_R | GRAPH_POLE_G | GRAPH_POLE_B);
/*
	fail_unless
	(
		graphRegionCount (g) == 3,
		"Expecting region count of 3, got %d instead",
		graphRegionCount (g)
	);
*/
	fail_unless (graphVertexCount (g) == 3);
	fail_unless (graphEdgeCount (g) == 9);
}
END_TEST

Suite * makeGraphInitSuite ()
{
	Suite
		* s = suite_create ("Graph initialization");
	TCase
		* tcCreate = tcase_create ("Creation"),
		* tcDestroy = tcase_create ("Destruction");

	tcase_add_checked_fixture (tcCreate, graphBlankCreate, NULL);
	tcase_add_test (tcCreate, graphCreateTest);
	suite_add_tcase (s, tcCreate);

	tcase_add_checked_fixture (tcDestroy, graphBlankCreateDestroy, NULL);
	tcase_add_test (tcDestroy, graphDestroyMainGraphTest);
	tcase_add_test (tcDestroy, graphDestroyEdgesTest);
	tcase_add_test (tcDestroy, graphDestroyVerticesTest);
	suite_add_tcase (s, tcDestroy);

	return s;
}

Suite * makeGraphWorldBaseSuite ()
{
	Suite
		* s = suite_create ("graphWorldBase");
	TCase
		* tcInvalid = tcase_create ("Invalid initializations"),
		* tcRGBase = tcase_create ("R-G initialization"),
		* tcRBBase = tcase_create ("R-B initialization"),
		* tcGBBase = tcase_create ("G-B initialization"),
		* tcFullBase = tcase_create ("Full (R-G-B) initialization");

	tcase_add_checked_fixture (tcInvalid, graphBlankCreate, NULL);
	tcase_add_test (tcInvalid, graphWorldBaseInvalidRBaseTest);
	tcase_add_test (tcInvalid, graphWorldBaseInvalidGBaseTest);
	tcase_add_test (tcInvalid, graphWorldBaseInvalidBBaseTest);
	suite_add_tcase (s, tcInvalid);

	tcase_add_checked_fixture (tcRGBase, graphBlankCreate, NULL);
	tcase_add_test (tcRGBase, graphWorldBaseInitializeRGTest);
	suite_add_tcase (s, tcRGBase);

	tcase_add_checked_fixture (tcRBBase, graphBlankCreate, NULL);
	tcase_add_test (tcRBBase, graphWorldBaseInitializeRBTest);
	suite_add_tcase (s, tcRBBase);

	tcase_add_checked_fixture (tcGBBase, graphBlankCreate, NULL);
	tcase_add_test (tcGBBase, graphWorldBaseInitializeGBTest);
	suite_add_tcase (s, tcGBBase);

	tcase_add_checked_fixture (tcFullBase, graphBlankCreate, NULL);
	tcase_add_test (tcFullBase, graphWorldBaseInitializeRGBTest);
	suite_add_tcase (s, tcFullBase);

	return s;
}

int main () {
	int
		number_failed = 0;
	SRunner
		* sr = srunner_create (makeGraphInitSuite ());

	srunner_add_suite (sr, makeGraphWorldBaseSuite ());

	logSetLevel (E_ALL);

	srunner_run_all (sr, CK_NORMAL);
	number_failed = srunner_ntests_failed (sr);
	srunner_free (sr);
	return (number_failed == 0)
		? EXIT_SUCCESS
		: EXIT_FAILURE;
}
