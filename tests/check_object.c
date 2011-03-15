#include "check_object.h"

Object
  * p = NULL,
  * c1 = NULL,
  * c2 = NULL,
  * c3 = NULL,
  * c4 = NULL;

START_TEST (test_objclass_create) {
  ObjClass
    * c = NULL;
  fail_unless (
     objClass_get ("BLUH BLUH") == NULL,
     "An attempt to get a nonexistant object class should return NULL");
  fail_unless (
    obj_create ("BLUH BLUH", NULL, NULL, NULL) == NULL,
    "It is invalid to create an object with a nonexistant class.");
  fail_unless (
    objClass_init (object_debug_handler, "foo", NULL, NULL) == NULL,
    "It is invalid to create a class that inherits from a non-existant class.");
  fail_unless (
    objClass_init (object_debug_broken_handler, NULL, NULL, NULL) == NULL,
    "A object handler which does not respond to the most basic messages cannot be used to create a class"
  );

  c = objClass_init (object_debug_handler, NULL, NULL, NULL);
  fail_unless (
    c != NULL,
    "A pointer to the class must be returned upon creation.");
  fail_unless (
    strcmp (c->name, "debug") == 0,
    "Classes must copy their names from their handler on creation");
  fail_unless (
     objClass_get ("debug") == c,
     "objClass_get must return a pointer to the named class if it exists");
  objClass_destroyP (c);
}
END_TEST


START_TEST (test_objclass_inherit) {
  ObjClass
    * p = objClass_init (object_debug_handler, NULL, NULL, NULL),
    * c = objClass_init (object_debug_child_handler, "debug", NULL, NULL);
  fail_unless (
    objClass_inherits ("debug", "BLUH") == FALSE);
  fail_unless (
    objClass_inherits ("foo", "bar") == FALSE);
  fail_unless (
    objClass_inherits ("debug", "debug") == TRUE,
    "A class should be considered to inherit from itself.");
  fail_unless (
    objClass_inheritsP (p, p) == TRUE);
  fail_unless (
    objClass_inheritsP (p, c) == TRUE);
  fail_unless (
    objClass_inheritsP (c, p) == FALSE);
  fail_unless (
    objClass_inherits ("debug", "debug child") == TRUE);
  fail_unless (
    objClass_inherits ("debug child", "debug") == FALSE);
  objClass_destroyP (p);
  objClass_destroyP (c);
}
END_TEST


START_TEST (test_objclass_parentage) {
  objClass_init (object_debug_handler, NULL, NULL, NULL);
  objClass_init (object_debug_child_handler, NULL, NULL, NULL);
  fail_unless (
    objClass_inherits ("debug child", "debug") == FALSE);
  fail_unless (
    objClass_chparent ("debug", "debug") == FALSE,
    "Classes may not be chparented to themselves"
  );
  objClass_chparent ("debug", "debug child");
  fail_unless (
    objClass_chparent ("debug child", "debug") == FALSE,
    "The class heirarchy may not loop (a class may not be a parent of any of its parents)");
  fail_unless (
    objClass_inherits ("debug", "debug child") == TRUE,
    "objClass_chparent must instantly alter the parentage of the given class");
  fail_unless (
    objClass_chparent (NULL, "debug child") == TRUE,
    "Chparenting to NULL is valid; it should remove all class inheritance");
  fail_unless (
    objClass_inherits ("debug", "debug child") == FALSE
  );
}
END_TEST



















START_TEST (test_object_create_fake) {
  Object * o = obj_create ("fake", NULL, NULL, NULL);
  fail_unless (o == NULL);
}
END_TEST

START_TEST (test_object_create) {
  Object * o = NULL;
  objClass_init (object_debug_handler, NULL, NULL, NULL);
  o = obj_create ("debug", NULL, NULL, NULL);
  fail_unless (
    o != NULL,
    "obj_create must return the object created"
  );
  obj_message (o, OM_DESTROY, NULL, NULL);
}
END_TEST

START_TEST (test_object_create_classset) {
  Object * o = NULL;
  objClass_init (object_debug_handler, NULL, NULL, NULL);
  o = obj_create ("debug", NULL, NULL, NULL);
  fail_unless (
    strcmp (obj_getClassName (o), "debug") == 0,
    "A created object must have the name of the class it was created as. (Expected \"debug\", got \"%s\")", obj_getClassName (o)
  );
}
END_TEST

START_TEST (test_object_create_defaults) {
  Object * o = NULL;
  objClass_init (object_debug_handler, NULL, NULL, NULL);
  o = obj_create ("debug", NULL, NULL, NULL);
  fail_unless (
    object_debug_initialized (o) == 1,
    "An object must receive an OM_CREATE message on creation, which must be able to set the object's values (Expected %d, got %d)", 1, object_debug_initialized (o)
  );
  fail_unless (
    strcmp (object_debug_name (o), "default") == 0,
    "An object must receive an OM_CREATE message on creation, which must be able to set the object's values (Expected \"%s\", got \"%s\")", "default", object_debug_name (o)
  );
  obj_message (o, OM_DESTROY, NULL, NULL);
}
END_TEST

START_TEST (test_object_create_class_initializer) {
  Object * o = NULL;
  int c = 0;
  objClass_init (object_debug_handler, NULL, (void *)52, (char *)"some name");
  o = obj_create ("debug", NULL, NULL, NULL);
  fail_unless (
    object_debug_initialized (o) == 52,
    "When a class is initialized, it may take arguments that must be passed to the object handler when it receives its OM_CLSINIT message. (Expected 52, got %d)", object_debug_initialized (o)
  );
  c = strcmp (object_debug_name (o), "some name");
  fail_unless (
    c == 0,
    "When a class is initialized, it may take arguments that must be passed to the object handler when it receives its OM_CLSINIT message. (Expected \"some name\", got \"%s\" (%d))", object_debug_name (o), c
  );
  obj_message (o, OM_DESTROY, NULL, NULL);
}
END_TEST

START_TEST (test_object_create_object_initializer) {
  Object * o = NULL;
  objClass_init (object_debug_handler, NULL, NULL, NULL);
  o = obj_create ("debug", NULL, (void *)24, "some name");
  fail_unless (
    object_debug_initialized (o) == 24,
    "An object may pass values in obj_create's final two arguments which will be passed further to the object's handler on an OM_CREATE message (Expected 52, got %d)", object_debug_initialized (o)
  );
  fail_unless (
    strcmp (object_debug_name (o), "some name") == 0,
    "(Expected \"some name\", got \"%s\")", object_debug_name (o)
  );
  obj_message (o, OM_DESTROY, NULL, NULL);
}
END_TEST

START_TEST (test_object_create_init_override) {
  Object
    * o = NULL,
    * p = NULL;
  objClass_init (object_debug_handler, NULL, (void *)92, "catoblepas");
  o = obj_create ("debug", NULL, (void *)52, "furthermore");
  fail_unless (object_debug_initialized (o) == 52);
  fail_unless (
    strcmp (object_debug_name (o), "furthermore") == 0,
    "(Expected \"some name\", got \"%s\")", object_debug_name (o)
  );
  p = obj_create ("debug", NULL, NULL, NULL);
  fail_unless (object_debug_initialized (p) == 92);
  fail_unless (
    strcmp (object_debug_name (p), "catoblepas") == 0,
    "(Expected \"catoblepas\", got \"%s\")", object_debug_name (o)
  );
  obj_message (o, OM_DESTROY, NULL, NULL);
  obj_message (p, OM_DESTROY, NULL, NULL);
}
END_TEST






START_TEST (test_object_parentage) {
  Object
    * gp = NULL,
    * p = NULL,
    * c1 = NULL,
    * c2 = NULL,
    * c3 = NULL;
  objClass_init (object_debug_handler, NULL, NULL, NULL);
  objClass_init (object_debug_child_handler, "debug", NULL, NULL);
  gp = obj_create ("debug", NULL, NULL, NULL);
  p = obj_create ("debug", gp, NULL, NULL);
  c1 = obj_create ("debug", p, NULL, NULL);
  c2 = obj_create ("debug", p, NULL, NULL);
  c3 = obj_create ("debug", p, NULL, NULL);
  fail_unless (
    obj_isChild (gp, p)
      && obj_isChild (p, c1)
      && obj_isChild (p, c2)
      && obj_isChild (p, c3),
    "When an object is created, the second arguement, an object pointer, mut be set as the object's parent. (%d %d %d %d)",
    obj_isChild (gp, p), obj_isChild (p, c1),
    obj_isChild (p, c2), obj_isChild (p, c3)
    );
  fail_unless (
      obj_areSiblings (c1, c2)
      && obj_areSiblings (c1, c3)
      && obj_areSiblings (c2, c3)
    );
  fail_unless (
      obj_childCount (gp) == 1
      && obj_childCount (p) == 3
      && obj_siblingCount (c1) == 2
      && obj_siblingCount (c2) == 2
      && obj_siblingCount (c3) == 2
    );
  obj_message (c2, OM_DESTROY, NULL, NULL);
  c2 = NULL;
  fail_unless (
    obj_isChild (p, c1) == TRUE
      && obj_isChild (p, c3) == TRUE,
    "Destroying an entity must remove it entirely from its parentage tree, without altering any other relationships (other children no longer have the same parent)");
  fail_unless (
    obj_areSiblings (c1, c3) == TRUE,
    "Destroying an entity must remove it entirely from its parentage tree, without altering any other relationships (other children are no longer siblings)");
  fail_unless (
    obj_siblingCount (c1) == 1
      && obj_siblingCount (c3) == 1
      && obj_childCount (p) == 2,
    "Destroying an entity must remove it entirely from its parentage tree, without altering any other relationships (children and sibling counts not updated)");
  c2 = obj_create ("debug", p, NULL, NULL);
  fail_unless (
    obj_areSiblings (c2, c1) == TRUE
      && obj_areSiblings (c2, c3) == TRUE
      && obj_areSiblings (c1, c3) == TRUE
      && obj_siblingCount (c1) == 2
      && obj_siblingCount (c3) == 2
      && obj_siblingCount (c2) == 2
      && obj_childCount (p) == 3,
    "Adding an entity must instantly update the parentage tree, without altering any other relationships.");
  obj_chparent (c2, c3);
  fail_unless (
    obj_areSiblings (c1, c3) == FALSE
      && obj_areSiblings (c2, c3) == FALSE
      && obj_isChild (c2, c3) == TRUE
      && obj_areSiblings (c1, c2) == TRUE,
    "Changing an entity's parent must instantly update the parentage tree without altering any other relationships");
  obj_message (p, OM_DESTROY, NULL, NULL);
  fail_unless (
    obj_isChild (gp, c1) == TRUE
      && obj_isChild (gp, c2) == TRUE
      && obj_areSiblings (c1, c2) == TRUE,
    "When destroyed, an entity's children should be chparented to their grandparent, if one exists.");
  mark_point ();
  //printf ("gp: %p, c2: %p\n", gp, c2);
  mark_point ();
  obj_message (gp, OM_DESTROY, NULL, NULL);
  mark_point ();
  obj_message (c2, OM_DESTROY, NULL, NULL);
  mark_point ();
  fail_unless (
    c3->parent == NULL,
    "When destroyed, an entity's children should be chparented to NULL if there is no existing grandparent.");
  // ...
  obj_message (c1, OM_DESTROY, NULL, NULL);
  obj_message (c3, OM_DESTROY, NULL, NULL);
}
END_TEST








void message_setup (void) {
  objClass_init (object_debug_handler, NULL, NULL, NULL);
  objClass_init (object_debug_child_handler, "debug", NULL, NULL);
  p = obj_create ("debug", NULL, NULL, NULL);
  c1 = obj_create ("debug", p, NULL, NULL);
  c2 = obj_create ("debug", p, NULL, NULL);
  c3 = obj_create ("debug", p, NULL, NULL);
  c4 = obj_create ("debug", c3, NULL, NULL);
}

void message_teardown (void) {
  obj_message (c4, OM_DESTROY, NULL, NULL);
  obj_message (c3, OM_DESTROY, NULL, NULL);
  obj_message (c2, OM_DESTROY, NULL, NULL);
  obj_message (c1, OM_DESTROY, NULL, NULL);
  obj_message (p, OM_DESTROY, NULL, NULL);
  objClass_destroy ("debug child");
  objClass_destroy ("debug");
}





START_TEST (test_object_message_siblings) {
  obj_messageSiblings (c1, OM_MESSAGE, NULL, NULL);
  mark_point ();
  if (
    object_debug_messageCount (c1) != FALSE) {
    fail ("All siblings of an entity must be messaged (in arbitrary order) by a \"sibling\"-type message. (The original entity was messaged as well.)");
  } else if (
    object_debug_messageCount (c2) != TRUE
      || object_debug_messageCount (c3) != TRUE) {
    fail ("All siblings of an entity must be messaged (in arbitrary order) by a \"sibling\"-type message. (Not all siblings were messaged: %d %d)", object_debug_messageCount (c2), object_debug_messageCount (c3));
  }
}
END_TEST

START_TEST (test_object_message_children) {
  obj_messageChildren (p, OM_MESSAGE, NULL, NULL);
  if (
    object_debug_messageCount (p) != FALSE) {
      fail ("All children of an entity, but not the entity itself, must be messaged by a \"child\"-type message (the parent was also messaged)");
  } else if (
    object_debug_messageCount (c1) != TRUE
      || object_debug_messageCount (c2) != TRUE
      || object_debug_messageCount (c3) != TRUE) {
      fail ("All children of an entity, but not the entity itself, must be messaged by a \"child\"-type message (not all children were messaged)");
    }
}
END_TEST

START_TEST (test_object_message_parents) {
  obj_messageParents (c1, OM_MESSAGE, NULL, NULL);
  obj_messageParents (c2, OM_MESSAGE, NULL, NULL);
  obj_messageParents (c3, OM_MESSAGE, NULL, NULL);
  obj_messageParents (c4, OM_MESSAGE, NULL, NULL);
  if (
    object_debug_messageCount (p) == 3) {
    fail ("All parents, not just the immediate parent, must be messaged by a \"parent\"-type message");
  } else if (
    object_debug_messageCount (p) != 4) {
    fail ("All parents of an entity, but not the entity itself, must be messaged by a \"parent\"-type message (the parents were not messaged properly)");
  } else if (
    object_debug_messageCount (c1) != 0
      || object_debug_messageCount (c2) != 0
      || object_debug_messageCount (c3) != 1
      || object_debug_messageCount (c4) != 0) {
    fail ("All parents of an entity, but not the entity itself, must be messaged by a \"parent\"-type message (the children were also messaged)");
  }
}
END_TEST

START_TEST (test_object_message_order) {
  // we can't know and don't care about the order siblings are messaged in
  // ... OR DO WE???? (rly idk, we might. if we do, then they ought to be messaged in the order they were created, with additional entity_prioritizeOver() functions or something like that to change the order)
  obj_messagePre (p, OM_MESSAGEORDER, NULL, NULL);
  fail_unless (
    object_debug_order (p) == 1
      && object_debug_order (c4) == 5,
    "Pre-order message traversal must visit roots before branches. (%d, %d)", object_debug_order (p), object_debug_order (c4));
  obj_message (p, OM_MESSAGEORDERRESET, NULL, NULL);
  obj_messagePost (p, OM_MESSAGEORDER, NULL, NULL);
  fail_unless (
    object_debug_order (p) == 5
      && object_debug_order (c4) == 1,
    "Post-order message traversal must visit branches before roots. (%d, %d)", object_debug_order (p), object_debug_order (c4));
  // ...
}
END_TEST


void pass_setup (void) {
  objClass_init (object_debug_handler, NULL, NULL, NULL);
  objClass_init (object_debug_child_handler, "debug", NULL, NULL);
  p = obj_create ("debug child", NULL, NULL, NULL);
}

void pass_teardown (void) {
  obj_message (p, OM_DESTROY, NULL, NULL);
}


START_TEST (test_object_message_pass) {
  mark_point ();
  obj_message (p, OM_PASSPRE, NULL, NULL);
  mark_point ();
  fail_unless (
    object_debug_pass (p) == OBJ_DEBUG_PASS_CHILD,
    "entity_pass() must switch execution to the parent's command immediately, regardless of where in the child's command the function call occurs (pass took the value of parent's command when it shouldn't)");
  mark_point ();
  obj_message (p, OM_PASSPOST, NULL, NULL);
  mark_point ();
  fail_unless (
    object_debug_pass (p) == OBJ_DEBUG_PASS_PARENT,
    "entity_pass() must switch execution to the parent's command immediately, regardless of where in the child's command the function call occurs (pass took the value of child's command when it shouldn't)");
  mark_point ();
}
END_TEST


/* skipping children and halting will likely need to be a lot more complex,
 * but right now they don't, so they aren't. most notably in the fixme
 * category, halt and skipchildren messages are only reset when the callstack
 * becomes empty. this isn't a problem with halting, but skipchildren needs
 * something more complex-- reset on each callstack /push/, perhaps, so that
 * it only persists for one "skip", and thus only skips one entity's children
 * as it ought. but idk, hence why I haven't done it.
 */
START_TEST (test_object_message_skipchildren) {
  Object * e5 = obj_create ("debug", c2, NULL, NULL);
  obj_messagePre (p, OM_SKIPONORDERVALUE, (void *)0, NULL);
  fail_unless (
    object_debug_messageCount (p) == 1,
    "an object_skipchildren() call cannot alter an in-progress message (got %d, expecting 1)", object_debug_messageCount (p)
    );
  fail_unless (
    object_debug_messageCount (c1) == 0
      && object_debug_messageCount (c2) == 0
      && object_debug_messageCount (c3) == 0,
    "an object_skipchildren() call must prevent any children which have not already been messaged from being messaged."
    );
  obj_messagePre (p, OM_MESSAGEORDER, NULL, NULL);
  obj_messagePre (p, OM_SKIPONORDERVALUE, (void *)object_debug_order (c2), NULL);
  fail_unless (
    object_debug_messageCount (e5) == 0,
    "the child of an object which has called object_skipchildren MUST NOT BE MESSAGED");
  fail_unless (
    object_debug_messageCount (c4) == 1,
    "siblings and cousins of an object which has called object_skipchildren must be messaged as usual, unless one of their parents has also called object_skipchildren");
  obj_message (e5, OM_DESTROY, NULL, NULL);
}
END_TEST


START_TEST (test_object_message_halt) {
  obj_messagePre (p, OM_MESSAGEORDER, NULL, NULL);
  obj_messagePre (p, OM_HALTONORDERVALUE, (void *)object_debug_order (c3), NULL);
  fail_unless (
    object_debug_messageCount (c4) == FALSE,
    "An entity_halt call must prevent any further children from being messaged."
  );
  fail_unless (
    object_debug_messageCount (p) == TRUE
      && object_debug_messageCount (c3) == TRUE,
    "An entity_halt call must not interrupt or alter messages which have already happened."
  );
/*
  // the state of c1 and c2 is undefined :/ i mean in this instance we know they have been messaged since they come before c3 in the sibling pointers, but this isn't something to rely on
  entity_messagePre (p, EM_RESETMESSAGE, NULL, NULL);
  entity_messagePre (p, EM_HALTONORDER, (void *)edebug_messageOrderResult (c1), NULL);
  fail_unless (
    edebug_messaged (c4) == FALSE,
    "An entity_halt call must prevent any further children from being messaged.");
  // ... idk (we want to test siblings of the halting entity getting messaged [they should] and children of the halting entity getting messaged [they shouldn't], and of course warning somehow if it would be pointless to halt [when traversing post-order])
*/
}
END_TEST


Suite * make_object_suite () {
  Suite * s = suite_create ("Objects");
  TCase
    * tc_class = tcase_create ("Classes"),
    * tc_create = tcase_create ("Creation"),
    * tc_parentage = tcase_create ("Parentage"),
    * tc_messaging = tcase_create ("Messaging"),
    * tc_pass = tcase_create ("Passing"),
    * tc_skiphalt = tcase_create ("Skipping and Halting"),
    * tc_submessages = tcase_create ("Recursive Messaging");

  tcase_add_test (tc_class, test_objclass_create);
  tcase_add_test (tc_class, test_objclass_inherit);
  tcase_add_test (tc_class, test_objclass_parentage);
  suite_add_tcase (s, tc_class);

  tcase_add_test (tc_create, test_object_create_fake);
  tcase_add_test (tc_create, test_object_create);
  tcase_add_test (tc_create, test_object_create_classset);
  tcase_add_test (tc_create, test_object_create_defaults);
  tcase_add_test (tc_create, test_object_create_class_initializer);
  tcase_add_test (tc_create, test_object_create_object_initializer);
  tcase_add_test (tc_create, test_object_create_init_override);
  suite_add_tcase (s, tc_create);

  tcase_add_test (tc_parentage, test_object_parentage);
  suite_add_tcase (s, tc_parentage);

  tcase_add_checked_fixture (tc_messaging, message_setup, message_teardown);
  tcase_add_test (tc_messaging, test_object_message_siblings);
  tcase_add_test (tc_messaging, test_object_message_children);
  tcase_add_test (tc_messaging, test_object_message_parents);
  tcase_add_test (tc_messaging, test_object_message_order);
  suite_add_tcase (s, tc_messaging);

  tcase_add_checked_fixture (tc_pass, pass_setup, pass_teardown);
  tcase_add_test (tc_pass, test_object_message_pass);
  suite_add_tcase (s, tc_pass);

  tcase_add_checked_fixture (tc_skiphalt, message_setup, message_teardown);
  tcase_add_test (tc_skiphalt, test_object_message_halt);
  tcase_add_test (tc_skiphalt, test_object_message_skipchildren);
  suite_add_tcase (s, tc_skiphalt);
  suite_add_tcase (s, tc_submessages);
  return s;
}

int main (void)
{
	int
		number_failed = 0;
	SRunner
		* sr = srunner_create (make_object_suite ());

	logSetLevel (E_OFF);
  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0)
    ? EXIT_SUCCESS
    : EXIT_FAILURE;
}
