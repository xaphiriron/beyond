#include "beyond.h"

int main (int argc, char * argv[]) {
  SYSTEM * sys = NULL;

  entclass_init (system_handler, NULL, NULL, NULL);
  sys = entity_getClassData (SystemEntity, "SYSTEM");

  entity_message (SystemEntity, EM_START, NULL, NULL);

  while (!sys->quit) {
    entity_message (SystemEntity, EM_UPDATE, NULL, NULL);

    entity_message (ControlEntity, EM_UPDATE, NULL, NULL);
    entity_message (PhysicsEntity, EM_UPDATE, NULL, NULL);
    while (physics_hasTime (PhysicsEntity)) {
      entity_messagePre (WorldEntity, EM_UPDATE, NULL, NULL);
      entity_messagePre (WorldEntity, EM_POSTUPDATE, NULL, NULL);
    }

    entity_messagePre (VideoEntity, EM_PRERENDER, NULL, NULL);
    entity_messagePre (WorldEntity, EM_PRERENDER, NULL, NULL);
    entity_messagePre (WorldEntity, EM_RENDER, NULL, NULL);
    entity_messagePre (WorldEntity, EM_POSTRENDER, NULL, NULL);
    entity_messagePre (VideoEntity, EM_POSTRENDER, NULL, NULL);
  }

  entity_message (SystemEntity, EM_DESTROY, NULL, NULL);
  entclass_destroyS ("SYSTEM");
  entclass_destroyAll ();
  return 0;
}