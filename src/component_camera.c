#include "component_camera.h"

/* it'd be nice if there was a faux video component/system so it could be
 * messaged so we wouldn't have to include this
 *  - xph 2011 08 20*/
#include "video.h"

#include "vector.h"
#include "quaternion.h"

#include "component_position.h"


/***
 * the camera component exists simply to visualize the data already stored in
 * the position component. orientation is a property of the position component,
 * not the camera.
 *
 * that being said, the camera component often caches its own position data and
 * the position data of its target in matrix form just to speed things up.
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
	enum camera_modes
		mode;
};


static Entity
	ActiveCamera = NULL;

void camera_setAsActive (Entity e);
void camera_switchMode (Entity camera, enum camera_modes mode);

void camera_update (Entity e);
void camera_updatePosition (Entity camera);

void camera_attachToTarget (Entity camera, Entity target);

void camera_updateTargetPositionData (Entity camera);
// azimuth is pitch from horizon; 0 means perpendicular with up vector, 90 means the opposite of the up vector, -90 means equal to the up vector. rotation is relative to the object's forward vector; 0 means equal to the object's forward vector, 180 means the opposite of it, and 90 and -90 are perpendicular.
// in first-person mode distance and rotation are clamped to 0 (and the target's orientation is used to calculate azimuth)
// in third-person mode rotation is clamped to 0 (and the target's orientation is used to calculate azimuth??)
// in isometric mode azimuth is clamped to 45 (or maybe 30-60)
void camera_setCameraOffset (Entity camera, float azimuth, float rotation, float distance);

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
	POSITION
		targetPosition = component_getData (entity_getAs (target, "position"));
	if (cd == NULL || targetPosition == NULL)
		return;
	if (cd->target != NULL)
		entity_unsubscribe (camera, cd->target);
	cd->target = target;
	entity_subscribe (camera, target);
	camera_updatePosition (camera);
}

void camera_setAsActive (Entity e)
{
	cameraComponent
		cdata = component_getData (entity_getAs (e, "camera"));
	POSITION
		pdata = component_getData (entity_getAs (e, "position"));
	if (pdata == NULL || cdata == NULL)
		return;
	camera_update (e);
	ActiveCamera = e;
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
}

void camera_updateTargetPositionData (Entity camera)
{
	cameraComponent
		cdata = component_getData (entity_getAs (camera, "camera"));
	POSITION
		targetPosition;
	const AXES
		* targetView;
	VECTOR3
		localOffset;
	if (cdata == NULL || cdata->target == NULL)
		return;
	targetPosition = component_getData (entity_getAs (cdata->target, "position"));
	localOffset = position_getLocalOffsetR (targetPosition);
	targetView = position_getViewAxesR (targetPosition);

/* originally this calculated the local offset (vector offset of the target
 * from the centre of the ground it's on) and ground offset (vector offset of
 * the target's ground from the 'origin ground'; the ground being drawn at
 * 0,0,0) but with the dissolution of the entire ground label system i don't
 * /think/ the latter is needed anymore, since the ground the camera is on
 * ought always be drawn at 0,0,0
 *  - xph 2011-05-12
 */

/*
 * if you're not up on your matrix math, what we do here is generate a
 * rotation matrix from the view axes and then premultiply it by a translation
 * matrix that holds the inverse of the full position as calculated above.
 * since multiplying a translation matrix and a rotation matrix together is
 * something of a special case, we do the calculation by hand instead of
 * calling some sort of matrix multiplication function.
 *  - xph 2010-11-25
 */
	cdata->targetMatrix[0] = targetView->side.x;
	cdata->targetMatrix[1] = targetView->up.x;
	cdata->targetMatrix[2] = targetView->front.x;
	cdata->targetMatrix[3] = 0;
	cdata->targetMatrix[4] = targetView->side.y;
	cdata->targetMatrix[5] = targetView->up.y;
	cdata->targetMatrix[6] = targetView->front.y;
	cdata->targetMatrix[7] = 0;
	cdata->targetMatrix[8] = targetView->side.z;
	cdata->targetMatrix[9] = targetView->up.z;
	cdata->targetMatrix[10] = targetView->front.z;
	cdata->targetMatrix[11] = 0.0;

	cdata->targetMatrix[12] =
		-localOffset.x * cdata->targetMatrix[0] +
		-localOffset.y * cdata->targetMatrix[4] +
		-localOffset.z * cdata->targetMatrix[8];
	cdata->targetMatrix[13] =
		-localOffset.x * cdata->targetMatrix[1] +
		-localOffset.y * cdata->targetMatrix[5] +
		-localOffset.z * cdata->targetMatrix[9];
	cdata->targetMatrix[14] =
		-localOffset.x * cdata->targetMatrix[2] +
		-localOffset.y * cdata->targetMatrix[6] +
		-localOffset.z * cdata->targetMatrix[10];
	cdata->targetMatrix[15] = 1.0;

}

void camera_setCameraOffset (Entity camera, float azimuth, float rotation, float distance)
{
	EntComponent
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

void camera_update (Entity e)
{
	cameraComponent
		cdata = component_getData (entity_getAs (e, "camera"));
	POSITION
		pdata = component_getData (entity_getAs (e, "position"));
	if (cdata == NULL)
	{
		WARNING ("Cannot update camera: camera (#%d) has no camera component", entity_GUID (e));
		return;
	}
	if (pdata == NULL)
	{
		WARNING ("Cannot update camera: camera (#%d) has no position component", entity_GUID (e));
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
//*/
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
	// update the final viewmatrix from position + offset data; derive position from that; get target ground data; position_set (camera, finalPosition, targetGround);
	// what is this.
	//DEBUG ("camera offset from target is %5.2f, %5.2f, %5.2f", cameraDistance.x, cameraDistance.y, cameraDistance.z);
	position_copy (camera, cdata->target);
	camera_updateTargetPositionData (camera);
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
	//position_move (camera, cameraDistance);
}

static Dynarr
	comp_entdata = NULL;
int component_camera (Object * obj, objMsg msg, void * a, void * b)
{
	switch (msg)
	{
		case OM_CLSNAME:
			strncpy (a, "camera", 32);
			return EXIT_SUCCESS;
		case OM_CLSINIT:
			comp_entdata = dynarr_create (8, sizeof (Entity));

			return EXIT_SUCCESS;
		case OM_CLSFREE:
			dynarr_destroy (comp_entdata);
			comp_entdata = NULL;
			return EXIT_SUCCESS;

		case OM_SHUTDOWN:
		case OM_DESTROY:
			obj_destroy (obj);
			return EXIT_SUCCESS;

		default:
			return obj_pass ();
	}
	return EXIT_FAILURE;
}

void component_cameraRegisterResponses (EntComponent camera, void * arg)
{

	component_registerResponse ("camera", "__init", component_cameraInitialize);
	component_registerResponse ("camera", "__destroy", component_cameraDestroy);

	component_registerResponse ("camera", "setTarget", component_cameraSetTarget);
	component_registerResponse ("camera", "activate", component_cameraActivate);

	component_registerResponse ("camera", "getMatrix", component_cameraGetMatrix);

	component_registerResponse ("camera", "CONTROL_INPUT", component_cameraControlResponse);
	/* here we are trusting if we get /any/ position/orientation updates
	 * they're from something we care about. right now this is fine; the
	 * camera is only subscribed to its target. but if speaking becomes a
	 * little more generalized this could lead to spurious updates
	 *  - xph 2011 08 19 */
	component_registerResponse ("camera", "orientationUpdate", component_cameraOrientResponse);
	component_registerResponse ("camera", "positionUpdate", component_cameraPositionResponse);
}

void component_cameraInitialize (EntComponent camera, void * arg)
{
	Entity
		camEntity = component_entityAttached (camera);
	cameraComponent
		camData = camera_create ();

	component_setData (camera, camData);

	camera_switchMode (camEntity, CAMERA_FIRST_PERSON);
	camera_setAsActive (camEntity);
	dynarr_push (comp_entdata, camEntity);
}

void component_cameraDestroy (EntComponent camera, void * arg)
{
	Entity
		camEntity = component_entityAttached (camera);
	cameraComponent
		camData = component_getData (camera);

	xph_free (camData);
	component_clearData (camera);

	dynarr_remove_condense (comp_entdata, camEntity);
}

void component_cameraActivate (EntComponent camera, void * arg)
{
	Entity
		camEntity = component_entityAttached (camera);

	camera_setAsActive (camEntity);
}

void component_cameraSetTarget (EntComponent camera, void * arg)
{
	Entity
		camEntity = component_entityAttached (camera),
		target = arg;

	camera_attachToTarget (camEntity, target);
}

void component_cameraControlResponse (EntComponent camera, void * arg)
{
	Entity
		camEntity = component_entityAttached (camera);
	cameraComponent
		camData = component_getData (camera);
	enum camera_modes
		newMode;
	const struct input_event
		* ie = arg;

	switch (ie->ir)
	{
		case IR_CAMERA_MODE_SWITCH:
			newMode = camData->mode + 1 == 4
				? 1
				: camData->mode + 1;
			camera_switchMode (camEntity, newMode);
			camera_update (camEntity);
			break;
		default:
			return;
	}
}

void component_cameraOrientResponse (EntComponent camera, void * arg)
{
	Entity
		camEntity = component_entityAttached (camera);

	camera_update (camEntity);
}

void component_cameraPositionResponse (EntComponent camera, void * arg)
{
	Entity
		camEntity = component_entityAttached (camera);
	POSITIONUPDATE
		update = arg;

	camera_update (camEntity);
	/* this should do an additional check on the platter distance between
	 * update->oldGround and update->newGround and only update the render
	 * cache if the value is above a certain value, dependant on the current
	 * view distance. But there's no generalized distance metric function for
	 * subhexes yet, so...
	 *  - xph 2011 08 19 */
	if (update != NULL)
	{
		worldSetRenderCacheCentre (update->newGround);
	}
}


void component_cameraGetMatrix (EntComponent camera, void * arg)
{
	cameraComponent
		camData = component_getData (camera);
	float
		** matrix = arg;

	if (camData == NULL)
	{
		*matrix = NULL;
		return;
	}
	*matrix = camData->m;
}
