#include "component_camera.h"

void updateCamera (Entity * e) {
  Component
    * c = entity_getAs (e, "camera"),
    * p = entity_getAs (e, "position");
  struct camera_data * cdata = NULL;
  struct position_data * pdata = NULL;
  if (c == NULL) {
    return;
  }
  cdata = c->comp_data;
  //printf ("%s (%p)\n", __FUNCTION__, e);
  if (p == NULL) {
    //printf ("no position\n");
    cdata->m[0] = cdata->m[5] = cdata->m[10] = cdata->m[15] = 1.0;
    cdata->m[1] = cdata->m[2] = cdata->m[3] = cdata->m[4] = \
    cdata->m[6] = cdata->m[7] = cdata->m[8] = cdata->m[9] = \
    cdata->m[11] = cdata->m[12] = cdata->m[13] = cdata->m[14] = \
      0.0;
    return;
  }
  pdata = p->comp_data;

  cdata->m[0] = pdata->orient.side.x;
  cdata->m[1] = pdata->orient.up.x;
  cdata->m[2] = pdata->orient.forward.x;
  cdata->m[3] = 0.0;

  cdata->m[4] = pdata->orient.side.y;
  cdata->m[5] = pdata->orient.up.y;
  cdata->m[6] = pdata->orient.forward.y;
  cdata->m[7] = 0.0;

  cdata->m[8] = pdata->orient.side.z;
  cdata->m[9] = pdata->orient.up.z;
  cdata->m[10] = pdata->orient.forward.z;
  cdata->m[11] = 0.0;

  cdata->m[12] = pdata->pos.x * pdata->orient.side.x +
                 pdata->pos.y * pdata->orient.side.y +
                 pdata->pos.z * pdata->orient.side.z;
  cdata->m[13] = pdata->pos.x * pdata->orient.up.x +
                 pdata->pos.y * pdata->orient.up.y +
                 pdata->pos.z * pdata->orient.up.z;
  cdata->m[14] = pdata->pos.x * pdata->orient.forward.x +
                 pdata->pos.y * pdata->orient.forward.y +
                 pdata->pos.z * pdata->orient.forward.z;
  cdata->m[15] = 1.0;

  //printf ("\n%+6.2f %+6.2f %+6.2f %+6.2f\n%+6.2f %+6.2f %+6.2f %+6.2f\n%+6.2f %+6.2f %+6.2f %+6.2f\n%+6.2f %+6.2f %+6.2f %+6.2f\n", cdata->m[0], cdata->m[1], cdata->m[2], cdata->m[3], cdata->m[4], cdata->m[5], cdata->m[6], cdata->m[7], cdata->m[8], cdata->m[9], cdata->m[10], cdata->m[11], cdata->m[12], cdata->m[13], cdata->m[14], cdata->m[15]);

}

int component_camera (Object * obj, objMsg msg, void * a, void * b) {
  struct camera_data ** cd = NULL;
  Vector * v = NULL;
  Entity * e = NULL;
  int i = 0;
  switch (msg) {
    case OM_CLSNAME:
      strncpy (a, "camera", 32);
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
    case OM_COMPONENT_INIT_DATA:
      cd = a;
      *cd = xph_alloc (sizeof (struct camera_data), "struct camera_data");
      updateCamera (b);
      return EXIT_SUCCESS;

    case OM_COMPONENT_DESTROY_DATA:
      cd = a;
      xph_free (*cd);
      *cd = NULL;
      return EXIT_SUCCESS;

    case OM_UPDATE:
      v = entity_getEntitiesWithComponent (1, "camera");
      while (i < vector_size (v)) {
        vector_at (e, v, i++);
        updateCamera (e);
      }
      return EXIT_SUCCESS;

    case OM_POSTUPDATE:
      return EXIT_SUCCESS;

    default:
      return obj_pass ();
  }
  return EXIT_FAILURE;
}
