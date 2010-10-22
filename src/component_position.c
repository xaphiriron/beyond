#include "component_position.h"

void setPosition (Entity e, VECTOR3 pos) {
  Component
    p = entity_getAs (e, "position"),
    c = entity_getAs (e, "collide");
  WORLD * w = NULL;
  HEX * prev = NULL;
  struct position_data * pdata = NULL;
  collide_data * cdata = NULL;
  if (p == NULL) {
    return;
  }
  cdata = component_getData (c);
  pdata = component_getData (p);
  pdata->pos = pos;
  prev = pdata->tileOccupying;
  if (WorldObject != NULL) {
    w = obj_getClassData (WorldObject, "world");
    pdata->tileOccupying = map_hex_at_point (w->map, pos.x, pos.z);
    /*
    if (pdata->tileOccupying != NULL) {
      printf ("entity #%d: over tile coordinate %d,%d {%d %d %d}\n", e->guid, pdata->tileOccupying->x, pdata->tileOccupying->y, pdata->tileOccupying->r, pdata->tileOccupying->k, pdata->tileOccupying->i);
    }
    */
  } else {
    pdata->tileOccupying = NULL;
  }
  if (pdata->tileOccupying == NULL && prev != NULL) {
    fprintf (stderr, "%s: Entity #%d fell out of the world somehow. Prior tile: %p, @ %d,%d\n", __FUNCTION__, entity_GUID (e), prev, prev->x, prev->y);
    vector_destroy (pdata->tileFootprint);
    pdata->tileFootprint = NULL;
    if (cdata) {
      cdata->onStableGround = FALSE;
    }
  }
  if (pdata->tileOccupying != NULL && pdata->tileOccupying != prev) {
    // we're assuming here that objects will be spherical enough so that a rotation around any axis won't change its footprint. this is not a reasonable assumption, and it means either 1) specifying a very large footprint for long, thin objects or 2) rewriting this to update footprint on rotation, not just translation.
    // we're also assuming no object will be larger than two tiles around. again, not reasonable. fix later.
    // (2010-09-27 - xph)
    if (pdata->tileFootprint != NULL) {
      vector_destroy (pdata->tileFootprint);
    }
    pdata->tileFootprint = map_adjacent_tiles (w->map, pdata->tileOccupying->x, pdata->tileOccupying->y);
    vector_push_back (pdata->tileFootprint, pdata->tileOccupying);
    if (cdata) {
      cdata->onStableGround = FALSE;
    }
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
      (*cd)->tileOccupying = NULL;
      (*cd)->tileFootprint = NULL;
      return EXIT_SUCCESS;

    case OM_COMPONENT_DESTROY_DATA:
      cd = a;
      if ((*cd)->tileFootprint != NULL) {
        vector_destroy ((*cd)->tileFootprint);
      }
      xph_free (*cd);
      return EXIT_SUCCESS;

    case OM_UPDATE:
      return EXIT_FAILURE;

    default:
      return obj_pass ();
  }
  return EXIT_FAILURE;
}
