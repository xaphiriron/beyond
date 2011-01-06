#include "component_camera.h"


/***
 * the camera component exists simply to visualize the data already stored in
 * the position component. orientation is a property of the position component,
 * not the camera.
 */
struct camera_data
{
	float
		m[16];
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

// these three functions are incorrect
float camera_getHeading (Entity e)
{
	cameraComponent
		cdata = component_getData (entity_getAs (e, "camera"));
	if (cdata == NULL)
		return 0.0;
	if (fcmp (cdata->m[1], 1.0) || fcmp (cdata->m[1], -1.0))
		return atan2 (cdata->m[8], cdata->m[10]);
	return atan2 (-cdata->m[2], cdata->m[0]);
}

float camera_getPitch (Entity e)
{
	cameraComponent
		cdata = component_getData (entity_getAs (e, "camera"));
	if (cdata == NULL)
		return 0.0;
	if (fcmp (cdata->m[1], 1.0))
		return M_PI_2;
	else if (fcmp (cdata->m[1], -1.0))
		return -M_PI_2;
	return asin (cdata->m[1]);
}

float camera_getRoll (Entity e)
{
	cameraComponent
		cdata = component_getData (entity_getAs (e, "camera"));
	if (cdata == NULL)
		return 0.0;
	if (fcmp (cdata->m[1], 1.0) || fcmp (cdata->m[1], -1.0))
		return 0.0;
	return atan2 (-cdata->m[9], cdata->m[5]);
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

void camera_updateLabelsFromEdgeTraversal (Entity e, struct ground_edge_traversal * t)
{
	cameraComponent
		cdata = component_getData (entity_getAs (e, "camera"));
	signed int
		x, y,
		lo = 0;
	unsigned int
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
	w = obj_getClassData (WorldObject, "world");
	label_getXY (cdata->l, &x, &y);
	x += XY[t->directionOfMovement][0];
	y += XY[t->directionOfMovement][1];
	hex_xy2rki (x, y, &r, &k, &i);
	lo = hex_linearCoord (r, k, i);
	newLabel = *(CameraGroundLabel *)dynarr_at (OriginCache->cache, lo);
	if (newLabel == NULL || (label_getCoordinateDistanceFromOrigin (newLabel) >= (w->groundDistanceDraw / 3) && label_getCoordinateDistanceFromOrigin (newLabel) > 1))
	{
		if (newLabel == NULL)
		{
			fprintf (stderr, "%s (#%d, ...): Active camera has walked outside the visible world. This is going to be a bumpy update.\n", __FUNCTION__, entity_GUID (e));
			newOrigin = label_getGroundReference (cdata->l);
		}
		else
		{
			newOrigin = label_getGroundReference (newLabel);
			lo = 0;
		}
		cameraCache_setGroundEntityAsOrigin (newOrigin);
		cameraCache_extend (w->groundDistanceDraw);
		//printf ("%s: resetting world origin to #%d (from #%d) \n", __FUNCTION__, entity_GUID (newOrigin), entity_GUID (w->groundOrigin));
		w->groundOrigin = newOrigin;
		if (newLabel == NULL)
		{
			x = XY[t->directionOfMovement][0];
			y = XY[t->directionOfMovement][1];
			hex_xy2rki (x, y, &r, &k, &i);
			lo = hex_linearCoord (r, k, i);
		}
		newLabel = *(CameraGroundLabel *)dynarr_at (OriginCache->cache, lo);
	}
	//printf ("%s: updated camera label\n", __FUNCTION__);
	//label_getXY (newLabel, &x, &y);
	//newOffset = label_getOriginOffset (newLabel);
	//printf ("\t@ %d,%d %5.2f, %5.2f, %5.2f\n", x, y, newOffset.x, newOffset.y, newOffset.z);
	cdata->l = newLabel;
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
	side = pdata->view.side;
	up = pdata->view.up;
	forward = pdata->view.front;

	fullPos.x =
		pdata->pos.x + groundOffset.x;
	fullPos.y =
		pdata->pos.y + groundOffset.y;
	fullPos.z =
		pdata->pos.z + groundOffset.z;

/*
 * if you're not up on your matrix math, what we do here is generate a
 * rotation matrix from the view axes and then premultiply it by a translation
 * matrix that holds the inverse of the full position as calculated above.
 * since multiplying a translation matrix and a rotation matrix together is
 * something of a special case, we do the calculation by hand instead of
 * calling some sort of matrix multiplication function.
 *  - xph 2010-11-25
 */
	if (pdata->dirty)
		position_updateAxesFromOrientation (e);
	cdata->m[0] = pdata->view.side.x;
	cdata->m[1] = pdata->view.up.x;
	cdata->m[2] = pdata->view.front.x;
	cdata->m[3] = 0;
	cdata->m[4] = pdata->view.side.y;
	cdata->m[5] = pdata->view.up.y;
	cdata->m[6] = pdata->view.front.y;
	cdata->m[7] = 0;
	cdata->m[8] = pdata->view.side.z;
	cdata->m[9] = pdata->view.up.z;
	cdata->m[10] = pdata->view.front.z;
	cdata->m[11] = 0.0;

	cdata->m[12] =
		-fullPos.x * cdata->m[0] +
		-fullPos.y * cdata->m[4] +
		-fullPos.z * cdata->m[8];
	cdata->m[13] =
		-fullPos.x * cdata->m[1] +
		-fullPos.y * cdata->m[5] +
		-fullPos.z * cdata->m[9];
	cdata->m[14] =
		-fullPos.x * cdata->m[2] +
		-fullPos.y * cdata->m[6] +
		-fullPos.z * cdata->m[10];
	cdata->m[15] = 1.0;

/*
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
 */

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


static Dynarr
	comp_entdata = NULL;
int component_camera (Object * obj, objMsg msg, void * a, void * b) {
	struct camera_data
		** cd = NULL;
	DynIterator
		it = NULL;
	Entity
		e = NULL;
	char
		* message = NULL;
	switch (msg)
	{
		case OM_CLSNAME:
			strncpy (a, "camera", 32);
			return EXIT_SUCCESS;
		case OM_CLSINIT:
			OriginCache = cameraCache_create ();
			comp_entdata = dynarr_create (8, sizeof (Entity));
			return EXIT_SUCCESS;
		case OM_CLSFREE:
			cameraCache_destroy (OriginCache);
			OriginCache = NULL;
			dynarr_destroy (comp_entdata);
			comp_entdata = NULL;
			return EXIT_SUCCESS;
    case OM_CLSVARS:
    case OM_CREATE:
      return EXIT_FAILURE;
    default:
      break;
	}
	switch (msg)
	{
		case OM_SHUTDOWN:
		case OM_DESTROY:
			obj_destroy (obj);
			return EXIT_SUCCESS;
		case OM_COMPONENT_INIT_DATA:
			e = (Entity)b;
			cd = a;
			*cd = xph_alloc (sizeof (struct camera_data));
			camera_setAsActive (b);
			dynarr_push (comp_entdata, e);
			return EXIT_SUCCESS;
		case OM_COMPONENT_DESTROY_DATA:
			e = (Entity)b;
			cd = a;
			xph_free (*cd);
			*cd = NULL;
			dynarr_remove_condense (comp_entdata, e);
			return EXIT_SUCCESS;

		case OM_UPDATE:
			it = dynIterator_create (comp_entdata);
			while (!dynIterator_done (it))
			{
				e = *(Entity *)dynIterator_next (it);
				camera_update (e);
			}
			dynIterator_destroy (it);
			return EXIT_SUCCESS;

		case OM_POSTUPDATE:
			return EXIT_SUCCESS;

    case OM_COMPONENT_RECEIVE_MESSAGE:
      message = ((struct comp_message *)a)->message;
      e = component_entityAttached (((struct comp_message *)a)->to);
      if (strcmp (message, "GROUND_EDGE_TRAVERSAL") == 0) {
        camera_updateLabelsFromEdgeTraversal (e, b);
      }
      return EXIT_FAILURE;

    default:
      return obj_pass ();
  }
  return EXIT_FAILURE;
}
