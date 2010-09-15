#include "component_position.h"

void setPosition (Entity * e, VECTOR3 pos) {
  Component * p = entity_getAs (e, "position");
  WORLD * w = NULL;
  HEX * prev = NULL;
  struct position_data * pdata = NULL;
  if (p == NULL) {
    return;
  }
  pdata = p->comp_data;
  pdata->pos = pos;
  prev = pdata->tileOccupying;
  if (WorldObject != NULL) {
    w = obj_getClassData (WorldObject, "world");
    pdata->tileOccupying = map_hex_at_point (w->map, pos.x, pos.z);
  } else {
    pdata->tileOccupying = NULL;
  }
  if (pdata->tileOccupying == NULL && prev != NULL) {
    fprintf (stderr, "%s: Entity #%d fell out of the world somehow. Prior tile: %p, @ %d,%d\n", __FUNCTION__, e->guid, prev, prev->x, prev->y);
  }
}

int component_position (Object * obj, objMsg msg, void * a, void * b) {
  struct position_data ** cd = NULL;
  switch (msg) {
    case OM_CLSNAME:
      strncpy (a, "position", 32);
      return EXIT_SUCCESS;
    case OM_CLSINIT:
    case OM_CLSFREE:
    case OM_CLSVARS:
    case OM_CREATE:
      return EXIT_FAILURE;
    default:
      break;
  }

  switch (msg) {
    case OM_SHUTDOWN:
    case OM_DESTROY:
      obj_destroy (obj);
      return EXIT_SUCCESS;

    case OM_COMPONENT_INIT_DATA:
      cd = a;
      *cd = xph_alloc (sizeof (struct position_data), "struct position_data");
      (*cd)->pos = vectorCreate (0.0, 0.0, 0.0);
      (*cd)->orient.side = vectorCreate (1.0, 0.0, 0.0);
      (*cd)->orient.up = vectorCreate (0.0, 1.0, 0.0);
      (*cd)->orient.forward = vectorCreate (0.0, 0.0, 1.0);
      return EXIT_SUCCESS;

    case OM_COMPONENT_DESTROY_DATA:
      cd = a;
      xph_free (*cd);
      return EXIT_SUCCESS;

    case OM_UPDATE:
      return EXIT_FAILURE;

    default:
      return obj_pass ();
  }
  return EXIT_FAILURE;
}
