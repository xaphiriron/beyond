#include "entitydebug.h"

static const struct eDebug * edebug_getClassData (const ENTITY * e);

int entitydebug_handler (ENTITY * e, eMessage msg, void * a, void * b) {
  struct eDebug * ed = NULL;
  switch (msg) {
    case EM_CLSNAME:
      strncpy (a, "entityDebug", 32);
      return EXIT_SUCCESS;
    case EM_CLSINIT:
    case EM_CLSFREE:
      return EXIT_FAILURE;
    case EM_CREATE:
      ed = xph_alloc (sizeof (struct eDebug), "struct eDebug");
      memset (ed, '\0', sizeof (struct eDebug));
      ed->initialized = TRUE;
      entity_addClassData (e, "entityDebug", ed);
      return EXIT_SUCCESS;
    default:
      break;
  }
  ed = entity_getClassData (e, "entityDebug");
  switch (msg) {
    case EM_SHUTDOWN:
    case EM_DESTROY:
      //printf ("killing entity %p (w/ entityDebug)\n", e);
      xph_free (ed);
      // this shouldn't actually be here (since we are destroying only this class's data-- this message is sent to ALL HANDLERS of the entity, and once we call entity_destroy all the other calls will explode with segfaults). we need to call entity_destroy only after all of the handlers has been called. this is something of a tangle, since EM_SHUTDOWN as well as EM_DESTROY may cause a class to destroy itself. maybe we should keep track of entity classes killed and destroy the entity itself when it has no more classes???? idk
      entity_rmClassData (e, "entityDebug");
      entity_destroy (e);
      break;

    case EM_SIBLINGTEST:
      ed->siblingTest = TRUE;
      return EXIT_SUCCESS;
    case EM_CHILDRENTEST:
      ed->childTest = TRUE;
      return EXIT_SUCCESS;
    case EM_PARENTTEST:
      ed->parentTest++;
      return EXIT_SUCCESS;
    case EM_SKIPKIDSONORDER:
      ed->messaged = TRUE;
      if ((int)a == 0 || ed->order == (int)a) {
        entity_skipchildren ();
      }
      return EXIT_SUCCESS;
    case EM_HALTONORDER:
      ed->messaged = TRUE;
      if ((int)a == 0 || ed->order == (int)a) {
        entity_halt ();
      }
      return EXIT_SUCCESS;
    case EM_RESETMESSAGE:
      ed->messaged = FALSE;
      return EXIT_SUCCESS;

    case EM_PASSPRE:
      entity_pass ();
      ed->pass = ENTPASS_CHILD;
      return EXIT_SUCCESS;
    case EM_PASSPOST:
      ed->pass = ENTPASS_CHILD;
      entity_pass ();
      return EXIT_SUCCESS;

    case EM_MESSAGEORDER:
      ed->order = entity_getRegister (0) + 1;
      entity_setRegister (0, ed->order);
      return EXIT_SUCCESS;

    default:
      return entity_pass ();
      break;
  }
  return EXIT_FAILURE;
}

int entitymessagedebug_handler (ENTITY * e, eMessage msg, void * a, void * b) {
  struct eDebug * ed = NULL;
  switch (msg) {
    case EM_CLSNAME:
      strncpy (a, "entityMessageDebug", 32);
      return EXIT_SUCCESS;
    case EM_CLSINIT:
    case EM_CLSFREE:
    case EM_CREATE:
      return EXIT_FAILURE;
    default:
      break;
  }
  ed = entity_getClassData (e, "entityDebug");
  switch (msg) {
    case EM_SHUTDOWN:
    case EM_DESTROY:
      //printf ("KILLING ENTITY %p (w/ entityMessageDebug)\n", e);
      xph_free (ed);
      // IDK HOW TO HANDLE THIS. baser classes might need to free their own data, but they can't call entity_destroy() since its baser classes might need to free... etc. and we can't free it here because this class might be extended in the future! D:
      entity_rmClassData (e, "entityDebug");
      entity_destroy (e);
      break;

    case EM_PASSPRE:
    case EM_PASSPOST:
      ed->pass = ENTPASS_PARENT;
      return EXIT_SUCCESS;

    default:
      break;
  }
  return EXIT_FAILURE;
}

static const struct eDebug * edebug_getClassData (const ENTITY * e) {
  const struct eDebug * d = entity_getClassData (e, "entityDebug");
  if (NULL == d) {
    abort();
  }
  return d;
}

bool edebug_siblingTestResult (const ENTITY * e) {
  const struct eDebug * d = edebug_getClassData (e);
  return d->siblingTest;
}

bool edebug_childrenTestResult (const ENTITY * e) {
  const struct eDebug * d = edebug_getClassData (e);
  return d->childTest;
}

bool edebug_parentTestResult (const ENTITY * e) {
  const struct eDebug * d = edebug_getClassData (e);
  return d->parentTest;
}

enum edebug_pass edebug_passResult (const ENTITY * e) {
  const struct eDebug * d = edebug_getClassData (e);
  return d->pass;
}

int edebug_messageOrderResult (const ENTITY * e) {
  const struct eDebug * d = edebug_getClassData (e);
  return d->order;
}

bool edebug_messaged (const ENTITY * e) {
  const struct eDebug * d = edebug_getClassData (e);
  return d->messaged;
}
