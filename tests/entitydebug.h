#ifndef ENTITYDEBUG_H
#define ENTITYDEBUG_H
#include "../src/entity.h"

enum edebug_messages {
  EM_SIBLINGTEST = EM_LAST + 1,
  EM_CHILDRENTEST,
  EM_PARENTTEST,
  EM_PASSPRE,
  EM_PASSPOST,
  EM_MESSAGEORDER,
  EM_SKIPKIDSONORDER,
  EM_HALTONORDER,
  EM_RESETMESSAGE
};

enum edebug_pass {
  ENTPASS_CHILD,
  ENTPASS_PARENT
};

struct eDebug {
  bool
    initialized,
    siblingTest,
    parentTest,
    childTest,
    messaged;
  int order;
  enum edebug_pass pass;
};

int entitydebug_handler (ENTITY * e, eMessage m, void * a, void * b);
int entitymessagedebug_handler (ENTITY * e, eMessage m, void * a, void * b);

bool edebug_siblingTestResult (const ENTITY * e);
bool edebug_childrenTestResult (const ENTITY * e);
bool edebug_parentTestResult (const ENTITY * e);
enum edebug_pass edebug_passResult (const ENTITY * e);
int edebug_messageOrderResult (const ENTITY * e);
bool edebug_messaged (const ENTITY * e);

#endif /* ENTITYDEBUG_H */