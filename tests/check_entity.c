#include "check_check.h"
#include <string.h>
#include "entitydebug.h"
#include "../src/entity.h"
#include "../src/list.h"

ENTITY
  * p = NULL,
  * c1 = NULL,
  * c2 = NULL,
  * c3 = NULL,
  * c4 = NULL;

void message_setup (void) {
  entclass_init (entitymessagedebug_handler, NULL, NULL, NULL);
  entclass_init (entitydebug_handler, "entityMessageDebug", NULL, NULL);
  p = entity_create ("entityDebug", NULL, NULL, NULL);
  c1 = entity_create ("entityDebug", p, NULL, NULL);
  c2 = entity_create ("entityDebug", p, NULL, NULL);
  c3 = entity_create ("entityDebug", p, NULL, NULL);
  c4 = entity_create ("entityDebug", c3, NULL, NULL);
}

void message_teardown (void) {
  entity_message (c4, EM_DESTROY, NULL, NULL);
  entity_message (c3, EM_DESTROY, NULL, NULL);
  entity_message (c2, EM_DESTROY, NULL, NULL);
  entity_message (c1, EM_DESTROY, NULL, NULL);
  entity_message (p, EM_DESTROY, NULL, NULL);
  entclass_destroyS ("entityMessageDebug");
  entclass_destroyS ("entityMessage");
}

START_TEST (test_entclass_create) {
  ENTCLASS
    * ec = NULL;
  fail_unless (
     entclass_get ("BLUH BLUH") == NULL,
     "An attempt to get a nonexistant entity class should return NULL");
  fail_unless (
    entity_create ("entityDebug", NULL, NULL, NULL) == NULL,
    "It is invalid to create an entity with a nonexistant entity class.");
  fail_unless (
    entclass_init (entitydebug_handler, "foo", NULL, NULL) == NULL,
    "It is invalid to create an entity class that inherits from a non-existant class.");
  ec = entclass_init (entitydebug_handler, NULL, NULL, NULL);
  fail_unless (
    ec != NULL,
    "A pointer to the entity class must be returned upon creation.");
  fail_unless (
    strcmp (ec->name, "entityDebug") == 0,
    "Entity classes must copy their names from their handler on creation");
  fail_unless (
     entclass_get ("entityDebug") == ec,
     "entclass_get must return a pointer to the named class if it exists");
  entclass_destroy (ec);
}
END_TEST

START_TEST (test_entclass_inherit) {
  ENTCLASS
    * ec = entclass_init (entitydebug_handler, NULL, NULL, NULL),
    * ecm = entclass_init (entitymessagedebug_handler, "entityDebug", NULL, NULL);
  fail_unless (
    entclass_inheritsFromS ("entityDebug", "BLUH") == FALSE);
  fail_unless (
    entclass_inheritsFromS ("foo", "bar") == FALSE);
  fail_unless (
    entclass_inheritsFromS ("entityDebug", "entityDebug") == TRUE,
    "A class should be considered to inherit from itself.");
  fail_unless (
    entclass_inheritsFrom (ec, ec) == TRUE);
  fail_unless (
    entclass_inheritsFrom (ecm, ec) == TRUE);
  fail_unless (
    entclass_inheritsFrom (ec, ecm) == FALSE);
  fail_unless (
    entclass_inheritsFromS ("entityMessageDebug", "entityDebug") == TRUE);
  fail_unless (
    entclass_inheritsFromS ("entityDebug", "entityMessageDebug") == FALSE);
  entclass_destroy (ec);
  entclass_destroy (ecm);
}
END_TEST


START_TEST (test_entclass_parentage) {
  entclass_init (entitydebug_handler, NULL, NULL, NULL);
  entclass_init (entitymessagedebug_handler, NULL, NULL, NULL);
  fail_unless (
    entclass_inheritsFromS ("entityMessageDebug", "entityDebug") == FALSE);
  fail_unless (
    entclass_chparentS ("entityDebug", "entityDebug") == FALSE,
    "Entity classes may not be chparented to themselves");
  entclass_chparentS ("entityMessageDebug", "entityDebug");
  fail_unless (
    entclass_chparentS ("entityDebug", "entityMessageDebug") == FALSE,
    "The entity heirarchy may not loop (a class may not be a parent of any of its parents)");
  fail_unless (
    entclass_inheritsFromS ("entityMessageDebug", "entityDebug") == TRUE,
    "entity_chparentClass must instantly alter the parentage of the given class");
  fail_unless (
    entclass_chparentS ("entityMessageDebug", NULL) == TRUE,
    "Chparetenting to NULL is valid; it should remove all class inheritance");
  fail_unless (
    entclass_inheritsFromS ("entityMessageDebug", "entityDebug") == FALSE);
}
END_TEST

START_TEST (test_entity_create) {
  ENTITY * e = NULL;
  ENTCLASS
    * ec = NULL,
    * ecm = NULL;
  ec = entclass_init (entitydebug_handler, NULL, NULL, NULL);
  ecm = entclass_init (entitymessagedebug_handler, NULL, NULL, NULL);
  e = entity_create ("entityDebug", NULL, NULL, NULL);
  fail_unless (
    e != NULL,
    "Entity not created");
  fail_unless (
    entity_isa (e, ec) == TRUE);
  fail_unless (
    entity_isaS (e, "entityDebug") == TRUE);
  fail_unless (
    entity_isa (e, ecm) == FALSE);
  fail_unless (
    entity_isaS (e, "entityMessageDebug") == FALSE);
  entclass_destroyS ("entityDebug");
  fail_unless (
    entclass_get ("entityDebug") == ec,
    "An entity class with instances cannot be destroyed.");
  entity_message (e, EM_DESTROY, NULL, NULL);
  fail_unless (
    ec->instances == 0);
  entclass_chparentS ("entityMessageDebug", "entityDebug");
  e = entity_create ("entityMessageDebug", NULL, NULL, NULL);
  fail_unless (
    entity_isa (e, ecm) == TRUE,
    "An entity instance must be the class it's instanced of.");
  fail_unless (
    entity_isa (e, ec) == TRUE,
    "An entity instance must be any classes its class is a child of.");
  fail_unless (
    entity_isaS (e, "entityDebug") == TRUE);
  fail_unless (
    entity_isaS (e, "entityMessageDebug") == TRUE);
  fail_unless (
    entclass_destroyS ("entityDebug") == FALSE,
    "An entity class that has child classes with instances cannot be destroyed.");
  entity_message (e, EM_DESTROY, NULL, NULL);
  fail_unless (
    ec->instances == 0 && ecm->instances == 0,
    "should have 0 instances >:( ec: %d, ecm: %d", ec->instances, ecm->instances);
  e = NULL;
  fail_unless (
    entclass_destroyS ("entityDebug") == TRUE);
/*
  fail_unless (
    entity_isaS (ecm, "entityDebug") == FALSE);
*/

/*
  fail_unless (
    entity_create ("entityDebug", NULL, NULL, NULL) == NULL,
    "Nonexistant entity classes should not be created");
  mark_point ();
  ec = entity_createClass (entity_debughandler_messagebus, NULL, NULL, NULL);
  fail_unless (
    ec != NULL,
    "Entity class must be returned upon creation.");
  mark_point ();
  fail_unless (
    entclass_get ("entityDebug") != NULL,
    "Entity class must be recorded in the class listing");
  mark_point ();
  e = entity_create ("entityDebug", NULL, NULL, NULL);
  mark_point ();
  fail_unless (
    e != NULL
      && strcmp (entclass_getName (e), "entityDebug") == 0,
    "Entity class not recorded");
  mark_point ();
  ed = e->data;
  fail_unless (
    ed->initialized == TRUE,
    "Entity initialization message must be sent automatically on entity creation");
  mark_point ();
  fail_unless (
    entclass_destroy ("entityDebug") == FALSE,
    "Classes with existing entities must not be destroyable.");

 * I don't really know what sort of parentage relations should be enforced here.
  entity_createClass (entity_debughandlerP_messagebus, "entityDebug", NULL, NULL);
  mark_point ();
  fail_unless (
    entclass_destroy ("entityDebugParent") == FALSE,
    "Classes that are parents of classes with existing entities must not be destroyable.");
  entity_message (e, EM_DESTROY, NULL, NULL);
  ec = entclass_get ("entityDebug");
  entclass_destroy ("entityDebugParent");
  fail_unless (
    entity_isChildOf (ec, NULL) == TRUE,
    "When destroyed, an entity class must remove itself from the class parentage tree by chparenting all its children to their grandparent.");
  */
}
END_TEST

  // this doesn't test things like chparenting objects with kids or deleting multiple children before creating new entities and soforth, and those had problems in previous iterations of the code.
START_TEST (test_entity_parentage) {
  ENTITY
    * gp = NULL,
    * p = NULL,
    * c1 = NULL,
    * c2 = NULL,
    * c3 = NULL;
  entclass_init (entitydebug_handler, NULL, NULL, NULL);
  entclass_init (entitymessagedebug_handler, "entityDebug", NULL, NULL);
  gp = entity_create ("entityDebug", NULL, NULL, NULL);
  p = entity_create ("entityDebug", gp, NULL, NULL);
  c1 = entity_create ("entityDebug", p, NULL, NULL);
  c2 = entity_create ("entityDebug", p, NULL, NULL);
  c3 = entity_create ("entityDebug", p, NULL, NULL);
  fail_unless (
    entity_isChildOf (p, gp)
      && entity_isChildOf (c1, p)
      && entity_isChildOf (c2, p)
      && entity_isChildOf (c3, p)
    );
  fail_unless (
      entity_areSiblings (c1, c2)
      && entity_areSiblings (c1, c3)
      && entity_areSiblings (c2, c3)
    );
  fail_unless (
      entity_childrenCount (gp) == 1
      && entity_childrenCount (p) == 3
      && entity_siblingCount (c1) == 2
      && entity_siblingCount (c2) == 2
      && entity_siblingCount (c3) == 2
    );
  entity_message (c2, EM_DESTROY, NULL, NULL);
  c2 = NULL;
  fail_unless (
    entity_isChildOf (c1, p) == TRUE
      && entity_isChildOf (c3, p) == TRUE,
    "Destroying an entity must remove it entirely from its parentage tree, without altering any other relationships (other children no longer have the same parent)");
  fail_unless (
    entity_areSiblings (c1, c3) == TRUE,
    "Destroying an entity must remove it entirely from its parentage tree, without altering any other relationships (other children are no longer siblings)");
  fail_unless (
    entity_siblingCount (c1) == 1
      && entity_siblingCount (c3) == 1
      && entity_childrenCount (p) == 2,
    "Destroying an entity must remove it entirely from its parentage tree, without altering any other relationships (children and sibling counts not updated)");
  c2 = entity_create ("entityDebug", p, NULL, NULL);
  fail_unless (
    entity_areSiblings (c2, c1) == TRUE
      && entity_areSiblings (c2, c3) == TRUE
      && entity_areSiblings (c1, c3) == TRUE
      && entity_siblingCount (c1) == 2
      && entity_siblingCount (c3) == 2
      && entity_siblingCount (c2) == 2
      && entity_childrenCount (p) == 3,
    "Adding an entity must instantly update the parentage tree, without altering any other relationships.");
  entity_chparent (c3, c2);
  fail_unless (
    entity_areSiblings (c1, c3) == FALSE
      && entity_areSiblings (c2, c3) == FALSE
      && entity_isChildOf (c3, c2) == TRUE
      && entity_areSiblings (c1, c2) == TRUE,
    "Changing an entity's parent must instantly update the parentage tree without altering any other relationships");
  entity_message (p, EM_DESTROY, NULL, NULL);
  fail_unless (
    entity_isChildOf (c1, gp) == TRUE
      && entity_isChildOf (c2, gp) == TRUE
      && entity_areSiblings (c1, c2) == TRUE,
    "When destroyed, an entity's children should be chparented to their grandparent, if one exists.");
  entity_message (gp, EM_DESTROY, NULL, NULL);
  entity_message (c2, EM_DESTROY, NULL, NULL);
  fail_unless (
    c3->parent == NULL,
    "When destroyed, an entity's children should be chparented to NULL if there is no existing grandparent.");
  // ...
  entity_message (c1, EM_DESTROY, NULL, NULL);
  entity_message (c3, EM_DESTROY, NULL, NULL);
}
END_TEST

START_TEST (test_em_siblings) {
  entity_messageSiblings (c1, EM_SIBLINGTEST, NULL, NULL);
  mark_point ();
  if (
    edebug_siblingTestResult (c1) != FALSE) {
    fail ("All siblings of an entity must be messaged (in arbitrary order) by a \"sibling\"-type message. (The original entity was messaged as well.)");
  } else if (
    edebug_siblingTestResult (c2) != TRUE
      || edebug_siblingTestResult (c3) != TRUE) {
    fail ("All siblings of an entity must be messaged (in arbitrary order) by a \"sibling\"-type message. (Not all siblings were messaged.)");
  }
}
END_TEST

START_TEST (test_em_children) {
  entity_messageChildren (p, EM_CHILDRENTEST, NULL, NULL);
  if (
    edebug_childrenTestResult (p) != FALSE) {
      fail ("All children of an entity, but not the entity itself, must be messaged by a \"child\"-type message (the parent was also messaged)");
  } else if (
    edebug_childrenTestResult (c1) != TRUE
      || edebug_childrenTestResult (c2) != TRUE
      || edebug_childrenTestResult (c3) != TRUE) {
      fail ("All children of an entity, but not the entity itself, must be messaged by a \"child\"-type message (not all children were messaged)");
    }
}
END_TEST

START_TEST (test_em_parents) {
  entity_messageParents (c1, EM_PARENTTEST, NULL, NULL);
  entity_messageParents (c2, EM_PARENTTEST, NULL, NULL);
  entity_messageParents (c3, EM_PARENTTEST, NULL, NULL);
  entity_messageParents (c4, EM_PARENTTEST, NULL, NULL);
  if (
    edebug_parentTestResult (p) == 3) {
    fail ("All parents, not just the immediate parent, must be messaged by a \"parent\"-type message");
  } else if (
    edebug_parentTestResult (p) != 4) {
    fail ("All parents of an entity, but not the entity itself, must be messaged by a \"parent\"-type message (the parents were not messaged properly)");
  } else if (
    edebug_parentTestResult (c1) != 0
      || edebug_parentTestResult (c2) != 0
      || edebug_parentTestResult (c3) != 1
      || edebug_parentTestResult (c4) != 0) {
    fail ("All parents of an entity, but not the entity itself, must be messaged by a \"parent\"-type message (the children were also messaged)");
  }
}
END_TEST

START_TEST (test_em_pass) {
  entity_message (c3, EM_PASSPRE, NULL, NULL);
  fail_unless (
    edebug_passResult (c3) == ENTPASS_CHILD,
    "entity_pass() must switch execution to the parent's command immediately, regardless of where in the child's command the function call occurs (pass took the value of parent's command when it shouldn't)");
  entity_message (c3, EM_PASSPOST, NULL, NULL);
  fail_unless (
    edebug_passResult (c3) == ENTPASS_PARENT,
    "entity_pass() must switch execution to the parent's command immediately, regardless of where in the child's command the function call occurs (pass took the value of child's command when it shouldn't)");
}
END_TEST

START_TEST (test_em_order) {
  entity_messagePre (p, EM_MESSAGEORDER, NULL, NULL);
  // we can't know and don't care about the order siblings are messaged in
  // ... OR DO WE???? (rly idk, we might. if we do, then they ought to be messaged in the order they were created, with additional entity_prioritizeOver() functions or something like that to change the order)
  fail_unless (
    edebug_messageOrderResult (p) == 1
      && edebug_messageOrderResult (c4) == 5,
    "Pre-order message traversal must visit roots before branches. (%d, %d)", edebug_messageOrderResult (p), edebug_messageOrderResult (c4));
  entity_messagePost (p, EM_MESSAGEORDER, NULL, NULL);
  fail_unless (
    edebug_messageOrderResult (p) == 5
      && edebug_messageOrderResult (c4) == 1,
    "Post-order message traversal must visit branches before roots. (%d, %d)", edebug_messageOrderResult (p), edebug_messageOrderResult (c4));
  // ...
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
START_TEST (test_em_skipchildren) {
  ENTITY * e5 = entity_create ("entityDebug", c2, NULL, NULL);
  entity_messagePre (p, EM_SKIPKIDSONORDER, (void *)0, NULL);
  fail_unless (
    edebug_messaged (p) == TRUE,
    "an entity_skipchildren() call cannot alter an in-progress message"
    );
  fail_unless (
    edebug_messaged (c1) == FALSE
      && edebug_messaged (c2) == FALSE
      && edebug_messaged (c3) == FALSE,
    "an entity_skipchildren() call must prevent any children which have not already been messaged from being messaged."
    );
  entity_messagePre (p, EM_MESSAGEORDER, NULL, NULL);
  entity_messagePre (p, EM_SKIPKIDSONORDER, (void *)edebug_messageOrderResult(c2), NULL);
  fail_unless (
    edebug_messaged (e5) == FALSE,
    "the child of an entity which has called entity_skipchildren MUST NOT BE MESSAGED");
  fail_unless (
    edebug_messaged (c4) == TRUE,
    "children of an entity which has called entity_skipchildren must be messaged as usual, unless one of their parents has also called entity_skipchildren");
  entity_message (e5, EM_DESTROY, NULL, NULL);
}
END_TEST

START_TEST (test_em_halt) {
  entity_messagePre (p, EM_MESSAGEORDER, NULL, NULL);
  entity_messagePre (p, EM_HALTONORDER, (void *)edebug_messageOrderResult (c3), NULL);
  fail_unless (
    edebug_messaged (c4) == FALSE,
    "An entity_halt call must prevent any further children from being messaged."
  );
  fail_unless (
    edebug_messaged (p) == TRUE
      && edebug_messaged (c3) == TRUE,
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

Suite * make_entity_suite (void) {
  Suite * s = suite_create ("Entity");
  TCase * tc_class = tcase_create ("Entity Classes");
  TCase * tc_entity = tcase_create ("Entities");
  TCase * tc_messages = tcase_create ("Entity Messages");
  tcase_add_test (tc_class, test_entclass_create);
  tcase_add_test (tc_class, test_entclass_inherit);
  tcase_add_test (tc_class, test_entclass_parentage);
  suite_add_tcase (s, tc_class);
/*
  tcase_add_test_raise_signal (tc_core, test_entity_inheritreq, 6);
  tcase_add_test (tc_core, test_entity_chparentreq);
*/
  tcase_add_test (tc_entity, test_entity_create);
  tcase_add_test (tc_entity, test_entity_parentage);
  suite_add_tcase (s, tc_entity);
  tcase_add_checked_fixture (tc_messages, message_setup, message_teardown);
  tcase_add_test (tc_messages, test_em_siblings);
  tcase_add_test (tc_messages, test_em_children);
  tcase_add_test (tc_messages, test_em_parents);
  tcase_add_test (tc_messages, test_em_pass);
  tcase_add_test (tc_messages, test_em_order);
  tcase_add_test (tc_messages, test_em_skipchildren);
  tcase_add_test (tc_messages, test_em_halt);
  suite_add_tcase (s, tc_messages);
  return s;
}
