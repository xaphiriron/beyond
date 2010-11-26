#include "component_camera.h"


/***
 * the camera component exists simply to visualize the data already stored in
 * the position component. orientation is a property of the position component,
 * not the camera.
 */
struct camera_data
{
	float m[16];
	CameraGroundLabel l;
};

const float * camera_getMatrix (Entity e)
{
	cameraComponent
		cdata = component_getData (entity_getAs (e, "camera"));
	if (cdata == NULL)
		return NULL;
	return cdata->m;
}

void camera_update (Entity e)
{
	cameraComponent
		cdata = component_getData (entity_getAs (e, "camera"));
	positionComponent
		pdata = component_getData (entity_getAs (e, "position"));
	VECTOR3
		groundOffset,
		fullPos,
		side,
		forward,
		up;
	float
		radians,
		rMatrix[16] = {
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
		};
	if (cdata == NULL || cdata->l == NULL)
	{
		fprintf (stderr, "%s (#%d): entity has no camera data (%p) or camera data has no label (%p)\n", __FUNCTION__, entity_GUID (e), cdata, cdata == NULL ? NULL : cdata->l);
		return;
	}
	if (pdata == NULL)
	{
		//printf ("no position\n");
		cdata->m[0] = cdata->m[5] = cdata->m[10] = cdata->m[15] = 1.0;
		cdata->m[1] = cdata->m[2] = cdata->m[3] = cdata->m[4] = \
		cdata->m[6] = cdata->m[7] = cdata->m[8] = cdata->m[9] = \
		cdata->m[11] = cdata->m[12] = cdata->m[13] = cdata->m[14] = \
			0.0;
		return;
	}
	groundOffset = label_getOriginOffset (cdata->l);
	radians = -(ground_getLabelRotation (cdata->l) * 60.0) / 180.0 * M_PI;
	side = pdata->orient.side;
	up = pdata->orient.up;
	forward = pdata->orient.forward;
	if (!fcmp (radians, 0.0))
	{
		//printf ("%s: camera object on rotated ground. rotating axes BACK by %5.2f\n", __FUNCTION__, -ground_getLabelRotation (cdata->l) * 60.0);
		rMatrix[0] = cos (radians);
		rMatrix[2] = -sin (radians);
		rMatrix[8] = sin (radians);
		rMatrix[10] = cos (radians);
		side = vectorMultiplyByMatrix (&side, rMatrix);
		up = vectorMultiplyByMatrix (&up, rMatrix);
		forward = vectorMultiplyByMatrix (&forward, rMatrix);
	}
	//printf ("radians: %f; rot: %d\n", radians, ground_getLabelRotation (cdata->l));
	fullPos.x =
		pdata->pos.x * side.x +
		pdata->pos.y * side.y +
		pdata->pos.z * side.z +
		groundOffset.x;
	fullPos.y =
		pdata->pos.x * up.x +
		pdata->pos.y * up.y +
		pdata->pos.z * up.z +
		groundOffset.y;
	fullPos.z =
		pdata->pos.x * forward.x +
		pdata->pos.y * forward.y +
		pdata->pos.z * forward.z +
		groundOffset.z;
	cdata->m[0] = side.x;
	cdata->m[1] = up.x;
	cdata->m[2] = forward.x;
	cdata->m[3] = 0.0;
	cdata->m[4] = side.y;
	cdata->m[5] = up.y;
	cdata->m[6] = forward.y;
	cdata->m[7] = 0.0;
	cdata->m[8] = side.z;
	cdata->m[9] = up.z;
	cdata->m[10] = forward.z;
	cdata->m[11] = 0.0;
	cdata->m[12] = -fullPos.x;
	cdata->m[13] = -fullPos.y;
	cdata->m[14] = -fullPos.z;
	cdata->m[15] = 1.0;
/*
	printf (
		"\n%+7.2f %+7.2f %+7.2f %+7.2f\n%+7.2f %+7.2f %+7.2f %+7.2f\n%+7.2f %+7.2f %+7.2f %+7.2f\n%+7.2f %+7.2f %+7.2f %+7.2f\n",
		cdata->m[0], cdata->m[4], cdata->m[8], cdata->m[12],
		cdata->m[1], cdata->m[5], cdata->m[9], cdata->m[13],
		cdata->m[2], cdata->m[6], cdata->m[10], cdata->m[14],
		cdata->m[3], cdata->m[7], cdata->m[11], cdata->m[15]
	);
*/

}

void camera_updateLabelsFromEdgeTraversal (Entity e, struct ground_edge_traversal * t)
{
	cameraComponent
		cdata = component_getData (entity_getAs (e, "camera"));
	int
		x, y,
		lo = 0;
	short
		r, k, i;
	CameraGroundLabel
		newLabel;
	Entity
		newOrigin;
/*
	VECTOR3
		newOffset;
*/
	WORLD * w = NULL;
	if (cdata == NULL)
		return;
	label_getXY (cdata->l, &x, &y);
	x += XY[t->directionOfMovement][0];
	y += XY[t->directionOfMovement][1];
	hex_xy2rki (x, y, &r, &k, &i);
	lo = hex_linearCoord (r, k, i);
	vector_at (newLabel, OriginCache->cache, lo);
	if (newLabel == NULL)
	{
		fprintf (stderr, "%s (#%d, ...): Active camera has walked outside the visible world. This is going to be a bumpy update.\n", __FUNCTION__, entity_GUID (e));
		newOrigin = label_getGroundReference (cdata->l);
		cameraCache_setGroundEntityAsOrigin (newOrigin);
		cameraCache_extend (2);
		newLabel = cameraCache_getOriginLabel ();
		w = obj_getClassData (WorldObject, "world");
		printf ("%s: resetting world origin to #%d (from #%d) \n", __FUNCTION__, entity_GUID (newOrigin), entity_GUID (w->groundOrigin));
		w->groundOrigin = newOrigin;
	}
	//printf ("%s: updated camera label\n", __FUNCTION__);
	//label_getXY (newLabel, &x, &y);
	//newOffset = label_getOriginOffset (newLabel);
	//printf ("\t@ %d,%d %5.2f, %5.2f, %5.2f\n", x, y, newOffset.x, newOffset.y, newOffset.z);
	cdata->l = newLabel;
}

void camera_setAsActive (Entity e)
{
	cameraComponent
		cdata = component_getData (entity_getAs (e, "camera"));
	positionComponent
		pdata = component_getData (entity_getAs (e, "position"));
	Entity origin = NULL;
	if (pdata == NULL || cdata == NULL)
		return;
	origin = position_getGroundEntity (pdata);
	cameraCache_setGroundEntityAsOrigin (origin);
	cdata->l = cameraCache_getOriginLabel ();
	camera_update (e);
}

int component_camera (Object * obj, objMsg msg, void * a, void * b) {
  struct camera_data ** cd = NULL;
  Vector * v = NULL;
  Entity e = NULL;
  int i = 0;
	char
		* message = NULL;
  switch (msg) {
    case OM_CLSNAME:
      strncpy (a, "camera", 32);
      return EXIT_SUCCESS;
    case OM_CLSINIT:
      OriginCache = cameraCache_create ();
      return EXIT_SUCCESS;
    case OM_CLSFREE:
      cameraCache_destroy (OriginCache);
      OriginCache = NULL;
      return EXIT_SUCCESS;
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
      *cd = xph_alloc (sizeof (struct camera_data));
      camera_setAsActive (b);
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
        camera_update (e);
      }
      vector_destroy (v);
      return EXIT_SUCCESS;

    case OM_POSTUPDATE:
      return EXIT_SUCCESS;

    case OM_COMPONENT_RECEIVE_MESSAGE:
      message = ((struct comp_message *)a)->message;
      if (strcmp (message, "GROUND_EDGE_TRAVERSAL") == 0) {
        e = component_entityAttached (((struct comp_message *)a)->to);
        camera_updateLabelsFromEdgeTraversal (e, b);
      }
      return EXIT_FAILURE;

    default:
      return obj_pass ();
  }
  return EXIT_FAILURE;
}
