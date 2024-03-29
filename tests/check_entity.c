#include "check_entity.h"

START_TEST (test_entity_create)
{
	Entity
		e = NULL;
	e = entity_create ();
	fail_unless
	(
		entity_GUID (e) > 0,
		"An entity must be assigned a non-zero GUID upon creation."
	);
	fail_unless
	(
		entity_exists (entity_GUID (e)),
		"An entity which has been created and not destroyed must be identified as existing."
	);
	entity_destroy (e);
	fail_unless
	(
		!entity_exists (entity_GUID (e)),
		"An entity which has been destroyed must be identified as not existing."
	);
}
END_TEST

START_TEST (test_component_attach)
{
	Entity
		e = entity_create ();
	EntComponent
		cd = NULL;
	cd = entity_getAs (e, "COMPONENT_NAME");
	fail_unless
	(
		cd == NULL,
		"An attempt to get a non-existant component from an entity should return NULL."
	);
	component_register ("COMPONENT_NAME", NULL);
	component_instantiate ("COMPONENT_NAME", e);
	cd = entity_getAs (e, "COMPONENT_NAME");
	fail_unless
	(
		cd != NULL,
		"If a component is instantiated on an entity, it must be returned by a entity_getAs call with the component name."
	);
	entity_destroy (e);
}
END_TEST

START_TEST (test_component_detach)
{
	Entity
		e = entity_create ();
	EntComponent
		cd = NULL;

	component_register ("COMPONENT_NAME", NULL);
	component_instantiate ("COMPONENT_NAME", e);
	cd = entity_getAs (e, "COMPONENT_NAME");
	fail_unless (cd != NULL);
	component_remove ("COMPONENT_NAME", e);
	cd = entity_getAs (e, "COMPONENT_NAME");
	fail_unless (cd == NULL);
	entity_destroy (e);
}
END_TEST

/*

START_TEST (test_component_message_entity) {
  Entity e = entity_create ();
  Component cd = NULL;
  entity_registerComponentAndSystem (component_name_obj_func);
  entity_registerComponentAndSystem (component_name_2_obj_func);
  component_instantiate ("COMPONENT_NAME", e);
  component_instantiate ("COMPONENT_NAME_2", e);
  cd = entity_getAs (e, "COMPONENT_NAME");
  mark_point ();
  component_messageEntity (cd, "COMPONENT_MESSAGE_ENTITY_COMPONENTS", NULL);
  mark_point ();
  entitySubsystem_store ("COMPONENT_NAME");
  entitySubsystem_store ("COMPONENT_NAME_2");
  entitySubsystem_runOnStored (OM_UPDATE);
  cd = entity_getAs (e, "COMPONENT_NAME_2");
  fail_unless (debugComponent_messageReceived (cd, "COMPONENT_MESSAGE_ENTITY_COMPONENTS"));
  entity_destroy (e);
}
END_TEST

*/

/* I don't really know what that "debugComponent_messageReceived" function would do. messages are sent to COMPONENTS, from components, not entities. I think I need to figure out just how components will be stored-- if they are objects, they can all child from a "default component", which implements certain basic component behaviors like having a magic number for each component type, or debug message tracking, or whatever.
 */

/*

START_TEST (test_system_component_message) {
  Entity
    e = entity_create (),
    f = entity_create (),
    g = entity_create ();
  Component cd = NULL;
  entity_registerComponentAndSystem (component_name_obj_func);
  entity_registerComponentAndSystem (component_name_2_obj_func);
  component_instantiate ("COMPONENT_NAME", e);
  component_instantiate ("COMPONENT_NAME", f);
  component_instantiate ("COMPONENT_NAME_2", g);
  cd = entity_getAs (e, "COMPONENT_NAME");
  component_messageSystem (cd, "COMPONENT_MESSAGE_SYSTEM_COMPONENTS", NULL);
  entitySubsystem_store ("COMPONENT_NAME");
  entitySubsystem_runOnStored (OM_UPDATE);
  cd = entity_getAs (f, "COMPONENT_NAME");
  fail_unless (debugComponent_messageReceived (cd, "COMPONENT_MESSAGE_SYSTEM_COMPONENTS"));
  cd = entity_getAs (g, "COMPONENT_NAME");
  fail_unless (!debugComponent_messageReceived (cd, "COMPONENT_MESSAGE_SYSTEM_COMPONENTS"));
  entity_destroy (e);
  entity_destroy (f);
  entity_destroy (g);
}
END_TEST

START_TEST (test_system_system_message) {
  Entity
    e = entity_create (),
    f = entity_create (),
    g = entity_create ();
  Component cd = NULL;
  entity_registerComponentAndSystem (component_name_obj_func);
  entity_registerComponentAndSystem (component_name_2_obj_func);
  component_instantiate ("COMPONENT_NAME", e);
  component_instantiate ("COMPONENT_NAME", f);
  component_instantiate ("COMPONENT_NAME_2", g);
  cd = entity_getAs (e, "COMPONENT_NAME");
  component_messageSystem (cd, "COMPONENT_MESSAGE_OTHER_SYSTEMS", NULL);
  entitySubsystem_store ("COMPONENT_NAME");
  entitySubsystem_runOnStored (OM_UPDATE);
  cd = entity_getAs (g, "COMPONENT_NAME_2");
  fail_unless (debugComponent_messageReceived (cd, "COMPONENT_MESSAGE_OTHER_SYSTEMS"));
  cd = entity_getAs (f, "COMPONENT_NAME");
  fail_unless (!debugComponent_messageReceived (cd, "COMPONENT_MESSAGE_OTHER_SYSTEMS"));
  entity_destroy (e);
  entity_destroy (f);
  entity_destroy (g);
}
END_TEST

*/

/*
 * basically iterating does two things: it selects a subset of all entities to iterate over (which it can do internally, since it's perfectly capable of looking at entities and seeing which have which components) and it does something to each of those entities, in an arbitrary order (which it probably has to do externally-- should it get passed a void (*)(Entity) function pointer? so that it calls the function on each entity it iterates over? would that work?) I suppose the other alternative is that entitySystems / entityManagers are objects and that they're messaged with OM_ITERATE or something.
 * how do entity messages work, though? I don't really grasp the whole subscribe/message thing that people sometimes mention going on. really I only grasp half the point of having entity managers in the first place. :/
 *

START_TEST (test_manager_iterate_one) {
  Entity
    e = entity_create (),
    f = entity_create ();
  component_instantiate ("COMPONENT_NAME", e);
  entityManager_iterate (debugComponent_update, 1, COMPONENT_NAME);
  fail_unless (debugComponent_updated (e) == TRUE);
  fail_unless (debugComponent_updated (f) == FALSE);
}
END_TEST

START_TEST (test_manager_iterate_two) {
  Entity
    e = entity_create (),
    f = entity_create ();
  component_instantiate ("COMPONENT_NAME", e);
  component_instantiate ("COMPONENT_NAME", f);
  entityManager_iterate (debugComponent_update, 1, COMPONENT_NAME);
  fail_unless (debugComponent_updated (e) == TRUE);
  fail_unless (debugComponent_updated (f) == TRUE);
}
END_TEST

START_TEST (test_manager_iterate_intersection) {
  Entity
    e = entity_create (),
    f = entity_create (),
    g = entity_create (),
    h = entity_create ();
  component_instantiate ("COMPONENT_NAME", e);
  component_instantiate ("COMPONENT_NAME", f);
  component_instantiate ("COMPONENT_NAME_2", f);
  component_instantiate ("COMPONENT_NAME_2", g);
  entityManager_iterate (debugComponent_update, 2, COMPONENT_NAME, COMPONENT_NAME_2);
  fail_unless (
    debugComponent_updated (f) == TRUE
  );
  fail_unless (
    debugComponent_updated (e) == FALSE &&
    debugComponent_updated (g) == FALSE &&
    debugComponent_updated (h) == FALSE
  );
}
END_TEST
*/

START_TEST (test_manager_fetch_one) {
  Entity
    e = entity_create (),
    f = entity_create (),
    g = entity_create (),
    h = entity_create ();
  Entity
    p = NULL,
    q = NULL;
  Dynarr v = NULL;

  component_register ("COMPONENT_NAME", NULL);

  component_register ("COMPONENT_NAME_2", NULL);
  component_instantiate ("COMPONENT_NAME", e);
  component_instantiate ("COMPONENT_NAME", f);
  component_instantiate ("COMPONENT_NAME_2", f);
  component_instantiate ("COMPONENT_NAME_2", g);
  v = entity_getWith (1, "COMPONENT_NAME");
  fail_unless (
    dynarr_size (v) == 2,
    "When getting a list of entities with a given component, every entity with the specified component should be returned, no matter its state or other components. (Received a vector with %d entr%s, when there should have been 2).",
    dynarr_size (v), (dynarr_size (v) == 1 ? "y" : "ies")
  );
  p = *(Entity *)dynarr_at (v, 0);
  q = *(Entity *)dynarr_at (v, 1);
  if (!((p == e && q == f) ||
    (p == f && q == e))) {
    fail (
      "was expecting values %p and %p, in any order, but instead got %p and %p.",
      e, f, p, q
    );
  }
  mark_point ();
  dynarr_destroy (v);
  entity_destroy (e);
  entity_destroy (f);
  entity_destroy (g);
  entity_destroy (h);
}
END_TEST


START_TEST (test_manager_fetch_two) {
  Entity
    e = entity_create (),
    f = entity_create (),
    g = entity_create (),
    h = entity_create ();
  Dynarr v = NULL;

  component_register ("COMPONENT_NAME", NULL);
  component_register ("COMPONENT_NAME_2", NULL);
  component_instantiate ("COMPONENT_NAME", e);
  component_instantiate ("COMPONENT_NAME", f);
  component_instantiate ("COMPONENT_NAME_2", f);
  component_instantiate ("COMPONENT_NAME_2", g);
  v = entity_getWith (2, "COMPONENT_NAME", "COMPONENT_NAME_2");
  fail_unless (
    *(Entity *)dynarr_at (v, 0) == f && dynarr_size (v) == 1
  );
  dynarr_destroy (v);
  entity_destroy (e);
  entity_destroy (f);
  entity_destroy (g);
  entity_destroy (h);
}
END_TEST

/*
 * iterate_union is probably not going to be something we'll want to use often. entities with discrete components don't have much to say to each other.
 *
START_TEST (test_system_iterate_union) {
  Entity
    e = entity_create (),
    f = entity_create (),
    g = entity_create (),
    h = entity_create ();
  component_instantiate ("COMPONENT_NAME", e);
  component_instantiate ("COMPONENT_NAME", f);
  component_instantiate ("COMPONENT_NAME_2", f);
  component_instantiate ("COMPONENT_NAME_2", g);
  entityManager_iterate_union (???, 2, COMPONENT_NAME, COMPONENT_NAME_2);
  fail_unless (
    debugComponent_updated (f) == TRUE &&
    debugComponent_updated (e) == TRUE &&
    debugComponent_updated (g) == TRUE
  );
  fail_unless (
    debugComponent_updated (h) == FALSE
  );
}
END_TEST
 */

Suite * make_entity_suite () {
  Suite * s = suite_create ("Entities");
  TCase
    * tc_create = tcase_create ("Creation"),
    * tc_components = tcase_create ("Components"),
    * tc_iterate = tcase_create ("Iterating")/*,
    * tc_system = tcase_create ("Systems")*/;

  tcase_add_test (tc_create, test_entity_create);
  suite_add_tcase (s, tc_create);

  tcase_add_test (tc_components, test_component_attach);
  tcase_add_test (tc_components, test_component_detach);
/*
  tcase_add_test (tc_components, test_component_message_entity);
*/
  suite_add_tcase (s, tc_components);

/*
  tcase_add_test (tc_iterate, test_manager_iterate_one);
  tcase_add_test (tc_iterate, test_manager_iterate_two);
  tcase_add_test (tc_iterate, test_manager_iterate_intersection);
*/
  tcase_add_test (tc_iterate, test_manager_fetch_one);
  tcase_add_test (tc_iterate, test_manager_fetch_two);
  suite_add_tcase (s, tc_iterate);

/*
  tcase_add_test (tc_system, test_system_component_message);
  tcase_add_test (tc_system, test_system_system_message);
  suite_add_tcase (s, tc_system);
*/
  return s;
}

int main (void) {
  int number_failed = 0;
  SRunner * sr = srunner_create (make_entity_suite ());

  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0)
    ? EXIT_SUCCESS
    : EXIT_FAILURE;
}
