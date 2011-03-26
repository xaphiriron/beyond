#include "component_camera.h"

/***
 * the camera component exists simply to visualize the data already stored in
 * the position component. orientation is a property of the position component,
 * not the camera.
 */
struct camera_data
{
	float
		targetMatrix[16],
		offset[16],
		m[16],
		distance,
		azimuth,
		rotation;
	Entity
		target;
	CameraGroundLabel
		l;
	enum camera_modes
		mode;
};


static Entity ActiveCamera = NULL;
static void camera_doControlInputResponse (Entity camera, const struct input_event * ie);
static struct camera_data * camera_create ();


static struct camera_data * camera_create ()
{
	struct camera_data
		* cd = xph_alloc (sizeof (struct camera_data));
	memset (cd->targetMatrix, '\0', sizeof (float) * 16);
	memset (cd->offset, '\0', sizeof (float) * 16);
	memset (cd->m, '\0', sizeof (float) * 16);
	cd->targetMatrix[0] = cd->targetMatrix[5] = cd->targetMatrix[10] = cd->targetMatrix[15] = 1.0;
	cd->offset[0] = cd->offset[5] = cd->offset[10] = cd->offset[15] = 1.0;
	cd->m[0] = cd->m[5] = cd->m[10] = cd->m[15] = 1.0;

	return cd;
}

/* the args are 'reversed' since I wrote the code to postmultiply (i think)
 * when I really wanted it to premultiply (i think). I am not very good at
 * math. :(
 */
static void matrixMultiplyf (const float * b, const float * a, float * r)
{
	r[0]  = a[ 0] * b[ 0] +
			a[ 1] * b[ 4] +
			a[ 2] * b[ 8] +
			a[ 3] * b[12];
	r[1]  = a[ 0] * b[ 1] +
			a[ 1] * b[ 5] +
			a[ 2] * b[ 9] +
			a[ 3] * b[13];
	r[2]  = a[ 0] * b[ 2] +
			a[ 1] * b[ 6] +
			a[ 2] * b[10] +
			a[ 3] * b[14];
	r[3]  = a[ 0] * b[ 3] +
			a[ 1] * b[ 7] +
			a[ 2] * b[11] +
			a[ 3] * b[15];

	r[4]  = a[ 4] * b[ 0] +
			a[ 5] * b[ 4] +
			a[ 6] * b[ 8] +
			a[ 7] * b[12];
	r[5]  = a[ 4] * b[ 1] +
			a[ 5] * b[ 5] +
			a[ 6] * b[ 9] +
			a[ 7] * b[13];
	r[6]  = a[ 4] * b[ 2] +
			a[ 5] * b[ 6] +
			a[ 6] * b[10] +
			a[ 7] * b[14];
	r[7]  = a[ 4] * b[ 3] +
			a[ 5] * b[ 7] +
			a[ 6] * b[11] +
			a[ 7] * b[15];

	r[8]  = a[ 8] * b[ 0] +
			a[ 9] * b[ 4] +
			a[10] * b[ 8] +
			a[11] * b[12];
	r[9]  = a[ 8] * b[ 1] +
			a[ 9] * b[ 5] +
			a[10] * b[ 9] +
			a[11] * b[13];
	r[10] = a[ 8] * b[ 2] +
			a[ 9] * b[ 6] +
			a[10] * b[10] +
			a[11] * b[14];
	r[11] = a[ 8] * b[ 3] +
			a[ 9] * b[ 7] +
			a[10] * b[11] +
			a[11] * b[15];

	r[12] = a[12] * b[ 0] +
			a[13] * b[ 4] +
			a[14] * b[ 8] +
			a[15] * b[12];
	r[13] = a[12] * b[ 1] +
			a[13] * b[ 5] +
			a[14] * b[ 9] +
			a[15] * b[13];
	r[14] = a[12] * b[ 2] +
			a[13] * b[ 6] +
			a[14] * b[10] +
			a[15] * b[14];
	r[15] = a[12] * b[ 3] +
			a[13] * b[ 6] +
			a[14] * b[11] +
			a[15] * b[15];
}


const float * camera_getMatrix (Entity e)
{
	cameraComponent
		cdata = component_getData (entity_getAs (e, "camera"));
	if (cdata == NULL)
		return NULL;
	return cdata->m;
}

enum camera_modes camera_getMode (Entity camera)
{
	cameraComponent
		cd = component_getData (entity_getAs (camera, "camera"));
	if (cd == NULL)
		return CAMERA_INVALID;
	return cd->mode;
}

const CameraGroundLabel camera_getLabel (Entity e)
{
	cameraComponent
		cdata = component_getData (entity_getAs (e, "camera"));
	if (cdata == NULL)
		return NULL;
	return cdata->l;
}


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
// these two functions are incorrect (i think?)
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

const Entity camera_getActiveCamera ()
{
	return ActiveCamera;
}

void camera_attachToTarget (Entity camera, Entity target)
{
	cameraComponent
		cd = component_getData (entity_getAs (camera, "camera"));
	positionComponent
		targetPosition = component_getData (entity_getAs (target, "position"));
	if (cd == NULL || targetPosition == NULL)
		return;
	cd->target = target;
	camera_updatePosition (camera);
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
	origin = position_getGroundEntityR (pdata);
	cameraCache_setGroundEntityAsOrigin (origin);
	cdata->l = cameraCache_getOriginLabel ();
	camera_update (e);
	ActiveCamera = e;
}

static void camera_doControlInputResponse (Entity camera, const struct input_event * ie)
{
	cameraComponent
		cd = component_getData (entity_getAs (camera, "camera"));
	enum camera_modes
		newMode;
	if (cd == NULL)
		return;
	switch (ie->ir)
	{
		case IR_CAMERA_MODE_SWITCH:
			newMode = cd->mode + 1 == 4 ? 1 : cd->mode + 1;
			camera_switchMode (camera, newMode);
			break;
		default:
			return;
	}
}

void camera_switchMode (Entity camera, enum camera_modes mode)
{
	struct comp_message
		* msg = NULL;
	cameraComponent
		cd = component_getData (entity_getAs (camera, "camera"));
	if (cd->mode == CAMERA_ISOMETRIC && mode != CAMERA_ISOMETRIC)
	{
			msg = xph_alloc (sizeof (struct comp_message));
			msg->from = NULL;
			msg->message = xph_alloc (17);
			strcpy (msg->message, "ORTHOGRAPHIC_OFF");
			obj_message (VideoObject, OM_SYSTEM_RECEIVE_MESSAGE, msg, msg->message);
			xph_free (msg->message);
			xph_free (msg);
			msg = NULL;
	}
	cd->mode = mode;
	switch (mode)
	{
		case CAMERA_FIRST_PERSON:
			cd->distance = 0;
			cd->azimuth = position_getPitch (cd->target);
			cd->rotation = 0;
			camera_setCameraOffset (camera, position_getPitch (cd->target), 0, 0);
			break;
		case CAMERA_THIRD_PERSON:
			cd->distance = 30;
			cd->azimuth = position_getPitch (cd->target);
			cd->rotation = 0;
			camera_setCameraOffset (camera, position_getPitch (cd->target), 0, 300);
			break;
		case CAMERA_ISOMETRIC:
			cd->distance = 60;
			cd->azimuth = 45;
			cd->rotation = 0;
			camera_setCameraOffset (camera, 45, 0, 600);

			msg = xph_alloc (sizeof (struct comp_message));
			msg->from = NULL;
			msg->message = xph_alloc (16);
			strcpy (msg->message, "ORTHOGRAPHIC_ON");
			obj_message (VideoObject, OM_SYSTEM_RECEIVE_MESSAGE, msg, msg->message);
			xph_free (msg->message);
			xph_free (msg);
			msg = NULL;
			break;
		default:
			return;
	}
	//DEBUG ("updating position:", NULL);
	camera_updatePosition (camera);
	//ERROR ("%s does nothing! yet! (but it was called with entity #%d and mode %d)", __FUNCTION__, entity_GUID (camera), mode);
}

void camera_updateTargetPositionData (Entity camera)
{
	cameraComponent
		cdata = component_getData (entity_getAs (camera, "camera"));
	positionComponent
		targetPosition;
	VECTOR3
		groundOffset,
		fullPos;
	if (cdata == NULL || cdata->target == NULL)
		return;
	targetPosition = component_getData (entity_getAs (cdata->target, "position"));
	position_updateAxesFromOrientation (cdata->target);

	groundOffset = label_getOriginOffset (cdata->l);

	fullPos.x = targetPosition->pos.x + groundOffset.x;
	fullPos.y = targetPosition->pos.y + groundOffset.y;
	fullPos.z = targetPosition->pos.z + groundOffset.z;

/*
 * if you're not up on your matrix math, what we do here is generate a
 * rotation matrix from the view axes and then premultiply it by a translation
 * matrix that holds the inverse of the full position as calculated above.
 * since multiplying a translation matrix and a rotation matrix together is
 * something of a special case, we do the calculation by hand instead of
 * calling some sort of matrix multiplication function.
 *  - xph 2010-11-25
 */
	cdata->targetMatrix[0] = targetPosition->view.side.x;
	cdata->targetMatrix[1] = targetPosition->view.up.x;
	cdata->targetMatrix[2] = targetPosition->view.front.x;
	cdata->targetMatrix[3] = 0;
	cdata->targetMatrix[4] = targetPosition->view.side.y;
	cdata->targetMatrix[5] = targetPosition->view.up.y;
	cdata->targetMatrix[6] = targetPosition->view.front.y;
	cdata->targetMatrix[7] = 0;
	cdata->targetMatrix[8] = targetPosition->view.side.z;
	cdata->targetMatrix[9] = targetPosition->view.up.z;
	cdata->targetMatrix[10] = targetPosition->view.front.z;
	cdata->targetMatrix[11] = 0.0;

	cdata->targetMatrix[12] =
		-fullPos.x * cdata->targetMatrix[0] +
		-fullPos.y * cdata->targetMatrix[4] +
		-fullPos.z * cdata->targetMatrix[8];
	cdata->targetMatrix[13] =
		-fullPos.x * cdata->targetMatrix[1] +
		-fullPos.y * cdata->targetMatrix[5] +
		-fullPos.z * cdata->targetMatrix[9];
	cdata->targetMatrix[14] =
		-fullPos.x * cdata->targetMatrix[2] +
		-fullPos.y * cdata->targetMatrix[6] +
		-fullPos.z * cdata->targetMatrix[10];
	cdata->targetMatrix[15] = 1.0;

}

void camera_setCameraOffset (Entity camera, float azimuth, float rotation, float distance)
{
	Component
		cameraCamera = entity_getAs (camera, "camera");
	cameraComponent
		cdata = component_getData (cameraCamera);
	float
		rotationRadians, azimuthRadians;

	if (cdata == NULL)
		return;
	cdata->azimuth = azimuth;
	cdata->rotation = rotation;
	cdata->distance = distance;

	memset (cdata->offset, '\0', sizeof (float) * 16);
	cdata->offset[0] = cdata->offset[5] = cdata->offset[10] \
	 = cdata->offset[15] = 1.0;
	if (cdata->mode == CAMERA_ISOMETRIC)
	{
		rotationRadians = rotation / 180 * M_PI;
		azimuthRadians = azimuth / 180 * M_PI;
		cdata->offset[0] = cos (rotationRadians);
		cdata->offset[8] = sin (rotationRadians);
		cdata->offset[2] = sin (-rotationRadians);
		cdata->offset[10] = cos (rotationRadians);
		// something something azimuth
	}
	if (cdata->mode != CAMERA_FIRST_PERSON)
	{
		cdata->offset[14] = -distance;
	}
}

void camera_updateLabelsFromGroundChange (Entity camera, struct ground_change * ch)
{
	cameraComponent
		cdata = component_getData (entity_getAs (camera, "camera"));
	signed int
		x, y,
		lo = 0;
	unsigned int
		r, k, i,
		drawDistance;
	CameraGroundLabel
		newLabel;
	Entity
		newOrigin;
	if (cdata == NULL)
		return;
	if (ch->dir == GROUND_FAR)
	{
		// something something
		return;
	}
	drawDistance = groundWorld_getDrawDistance ();
	label_getXY (cdata->l, &x, &y);
	x += XY[ch->dir][0];
	y += XY[ch->dir][1];
	hex_xy2rki (x, y, &r, &k, &i);
	lo = hex_linearCoord (r, k, i);
	newLabel = *(CameraGroundLabel *)dynarr_at (OriginCache->cache, lo);
	if (newLabel == NULL || (label_getCoordinateDistanceFromOrigin (newLabel) >= (drawDistance / 3) && label_getCoordinateDistanceFromOrigin (newLabel) > 1))
	{
		if (newLabel == NULL)
		{
			WARNING ("%s (#%d, ...): Active camera has walked outside the visible world. This is going to be a bumpy update.\n", __FUNCTION__, entity_GUID (camera));
			newOrigin = label_getGroundReference (cdata->l);
		}
		else
		{
			newOrigin = label_getGroundReference (newLabel);
			lo = 0;
		}
		cameraCache_setGroundEntityAsOrigin (newOrigin);
		//printf ("%s: resetting world origin to #%d (from #%d) \n", __FUNCTION__, entity_GUID (newOrigin), entity_GUID (w->groundOrigin));
		groundWorld_updateEntityOrigin (camera, newOrigin);
		if (newLabel == NULL)
		{
			x = XY[ch->dir][0];
			y = XY[ch->dir][1];
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
	if (cdata == NULL || cdata->l == NULL)
	{
		WARNING ("%s (#%d): entity has no camera data (%p) or camera data has no label (%p)\n", __FUNCTION__, entity_GUID (e), cdata, cdata == NULL ? NULL : cdata->l);
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
	camera_updatePosition (e);

	matrixMultiplyf (cdata->offset, cdata->targetMatrix, cdata->m);
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

//*
void camera_updatePosition (Entity camera)
{
	cameraComponent
		cdata = component_getData (entity_getAs (camera, "camera"));
	VECTOR3
		cameraDistance;
	if (cdata == NULL || cdata->target == NULL)
		return;
	camera_updateTargetPositionData (camera);
	// update the final viewmatrix from position + offset data; derive position from that; get target ground data; position_set (camera, finalPosition, targetGround);
	// what is this.
	cameraDistance.x =
		cdata->offset[12] * cdata->offset[0] +
		cdata->offset[13] * cdata->offset[4] +
		cdata->offset[14] * cdata->offset[8];
	cameraDistance.y =
		cdata->offset[12] * cdata->offset[1] +
		cdata->offset[13] * cdata->offset[5] +
		cdata->offset[14] * cdata->offset[9];
	cameraDistance.z =
		cdata->offset[12] * cdata->offset[2] +
		cdata->offset[13] * cdata->offset[6] +
		cdata->offset[14] * cdata->offset[10];
	//DEBUG ("camera offset from target is %5.2f, %5.2f, %5.2f", cameraDistance.x, cameraDistance.y, cameraDistance.z);
	position_copy (camera, cdata->target);
	//position_move (camera, cameraDistance);
}
//*/

/*
void camera_updatePosition (Entity camera)
{
	cameraComponent
		cd = component_getData (entity_getAs (camera, "camera"));
	positionComponent
		camPos = component_getData (entity_getAs (camera, "position")),
		targetPos;
	QUAT
		targetOrient,
		camOrient;
	if (cd == NULL || cd->target == NULL)
		return;
	// some notes:
	// in first-person mode we just have to move the camera to its target and face it in the right direction
	// in third-person mode we have to use some camera internal coordinates to place the camera in space, relative to the target. (polar coordinates, one assumes) it still is clamped to be facing the same direction as the target
	// in isometric mode we use camera internal coordinates to place the camera in space, but the internal coordinates are clamped rather severely (very little capacity to pan the camera). also, now the camera rotation is decoupled from the target's rotation (so rotating the camera doesn't rotate the target and vice versa)

	//camera_setPositionRelativeTo (camera, cd->target, 0, 0, 0);

	targetPos = component_getData (entity_getAs (cd->target, "position"));
	position_set (camera, position_getLocalOffsetR (targetPos), position_getGroundEntityR (targetPos));
	//DEBUG ("First-person camera updating to be on ground %p (%p)", position_getGroundEntityR (targetPos), camPos->mapEntity);

	if (cd->mode == CAMERA_FIRST_PERSON || cd->mode == CAMERA_THIRD_PERSON)
	{
		targetOrient = position_getOrientationR (targetPos);
		camOrient = position_getOrientation (camera);
		if (!quat_cmp (&targetOrient, &camOrient))
		{
			position_setOrientation (camera, targetOrient);
			camOrient = targetOrient;
		}
		// this is dumb and inefficient
		if (targetPos->dirty)
			position_updateAxesFromOrientation (cd->target);
		if (camPos->dirty)
			position_updateAxesFromOrientation (camera);
	}
}
//*/

static Dynarr
	comp_entdata = NULL;
int component_camera (Object * obj, objMsg msg, void * a, void * b)
{
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
			*cd = camera_create ();
			//DEBUG ("switching to first person:", NULL);
			camera_switchMode (e, CAMERA_FIRST_PERSON);
			//DEBUG ("setting as active:", NULL);
			camera_setAsActive (b);
			//DEBUG ("done with camera init", NULL);
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
			if (strcmp (message, "GROUND_CHANGE") == 0)
			{
				DEBUG ("Camera labels have updated", NULL);
				camera_updateLabelsFromGroundChange (e, (struct ground_change *)b);
			}
			else if (strcmp (message, "CONTROL_INPUT") == 0)
			{
				camera_doControlInputResponse (e, (const struct input_event *)b);
			}
			return EXIT_FAILURE;

		default:
			return obj_pass ();
	}
	return EXIT_FAILURE;
}
