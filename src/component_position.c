#include "component_position.h"


void position_set (Entity e, VECTOR3 pos, Entity mapEntity)
{
	struct position_data
		* pdata = component_getData (entity_getAs (e, "position"));
	if (pdata == NULL)
	{
		fprintf (stderr, "%s (#%d, ...): entity without position component\n", __FUNCTION__, entity_GUID (e));
		return;
	}
	pdata->pos = pos;
	pdata->mapEntity = mapEntity;
}

bool position_move (Entity e, VECTOR3 move)
{
	struct position_data
		* pos = component_getData (entity_getAs (e, "position"));
	GroundMap
		map = NULL;
	VECTOR3
		oldPos,
		movedPos,
		hexCenter;
	signed int
		x, y;
	unsigned int
		r, k, i,
		groundSize = 0;
	if (pos == NULL)
	{
		return FALSE;
	}
	map = component_getData (entity_getAs (pos->mapEntity, "ground"));
	if (map == NULL)
		return FALSE;
	groundSize = ground_getMapSize (map);
	movedPos = vectorAdd (&pos->pos, &move);
	hex_space2coord (&movedPos, &x, &y);
	hex_xy2rki (x, y, &r, &k, &i);
	hexCenter = hex_coord2space (r, k, i);
	//printf ("%s: over tile {%d %d %d} (%d, %d) @ %5.2f, %5.2f, %5.2f; real pos (locally): %5.2f, %5.2f, %5.2f\n", __FUNCTION__, r, k, i, x, y, hexCenter.x, hexCenter.y, hexCenter.z, pos->pos.x, pos->pos.y, pos->pos.z);
	oldPos = pos->pos;
	pos->pos = movedPos;
	if (r > groundSize)
	{
		//printf ("%s: outside ground boundary (%d tiles from center)\n", __FUNCTION__, r);
		if (!ground_bridgeConnections (pos->mapEntity, e))
		{
			// invalid move attempt: walked outside of ground into void or into ground edge wall, depending on the ground's settings. this should send off an entity message to e, but ihni what would pick it up. collision, i guess. 
			pos->pos = oldPos;
			return FALSE;
		}
	}
	//printf ("%s: new local offset is %5.2f, %5.2f, %5.2f\n", __FUNCTION__, pos->pos.x, pos->pos.y, pos->pos.z);
	return TRUE;
}

void position_updateAxesFromOrientation (Entity e)
{
	positionComponent
		pdata = component_getData (entity_getAs (e, "position"));
	QUAT
		r;
	if (pdata == NULL || pdata->dirty == FALSE)
		return;
	r = pdata->orientation;
	// these values constitute a 3x3 rotation matrix. opengl offsets are commented on each value. see the camera component for how these are used to create the cameraview matrix.
	pdata->view.side.x =		// [0]
		1 -
		2 * r.y * r.y -
		2 * r.z * r.z;
	pdata->view.side.y =		// [4]
		2 * r.x * r.y -
		2 * r.w * r.z;
	pdata->view.side.z =		// [8]
		2 * r.x * r.z +
		2 * r.w * r.y;
	pdata->view.up.x =			// [1]
		2 * r.x * r.y +
		2 * r.w * r.z;
	pdata->view.up.y =			// [5]
		1 -
		2 * r.x * r.x -
		2 * r.z * r.z;
	pdata->view.up.z =			// [9]
		2 * r.y * r.z -
		2 * r.w * r.x;
	pdata->view.front.x =		// [2]
		2 * r.x * r.z -
		2 * r.w * r.y;
	pdata->view.front.y =		// [6]
		2 * r.y * r.z +
		2 * r.w * r.x;
	pdata->view.front.z =		// [10]
		1 -
		2 * r.x * r.x -
		2 * r.y * r.y;
	pdata->move.front = vectorCross (&pdata->view.side, &pdata->move.up);
	pdata->move.front = vectorNormalize (&pdata->move.front);
	pdata->move.side = vectorCross (&pdata->move.up, &pdata->view.front);
	pdata->move.side = vectorNormalize (&pdata->move.side);
	pdata->move.up = vectorCreate (0, 1, 0);
	pdata->orientation = quat_normalize (&pdata->orientation);
	pdata->dirty = FALSE;
}

void position_rotateOnMouseInput (Entity e, const struct input_event * ie)
{
	positionComponent
		pdata = NULL;
	QUAT
		q;
	float
		mag,
		newPitch;
	int
		sq,
		xrel,
		yrel;
	if (ie->event->type != SDL_MOUSEMOTION)
		return;
	pdata = component_getData (entity_getAs (e, "position"));
	if (pdata == NULL)
		return;
	xrel = ie->event->motion.xrel;
	yrel = ie->event->motion.yrel;
	sq = xrel * xrel + yrel * yrel;
	if (sq > 2500)
	{
		mag = 50.0 / sqrt (sq);
		xrel *= mag;
		yrel *= mag;
		return;
	}
	//position_updateAxesFromOrientation (e);
	// ^ we manually recalculate the one orientation axis value (front y component) we care about right here, instead of updating every single one
	newPitch =
		(pdata->orientation.y * pdata->orientation.z +
		pdata->orientation.w * pdata->orientation.x) * 180.0;
	newPitch += yrel * pdata->sensitivity;
	//printf ("front: %7.3f, %7.3f, %7.3f\n", pdata->view.front.x, pdata->view.front.y, pdata->view.front.z);
	//printf ("pitch: %7.3f\n", newPitch);
	if (newPitch < 90.0 && newPitch > -90.0)
	{
		// pitch relative to the object's axes
		q = quat_eulerToQuat (
			yrel * pdata->sensitivity,
			0,
			0
		);
		pdata->orientation = quat_multiply (&q, &pdata->orientation);
	}
	// (roll would also be relative to the world axis, if it's ever added)
	// heading relative to the world's axes
	q = quat_eulerToQuat (
		0,
		xrel * pdata->sensitivity,
		0
	);
	pdata->orientation = quat_multiply (&pdata->orientation, &q);
	pdata->dirty = TRUE;
	//printf ("view quat: %7.2f, %7.2f, %7.2f, %7.2f\n", cdata->viewQuat.w, cdata->viewQuat.x, cdata->viewQuat.y, cdata->viewQuat.z);
}


// THESE CALCULATIONS WERE WRITTEN BEFORE THE ORIENTATION QUATERNION EXISTED. THIS FUNCTION WILL NOT WORK UNLESS IT IS UPDATED TO ROTATE THE ORIENTATION QUATERNION. DO NOT USE IT.
bool position_rotateAroundGround (Entity e, float rotation)
{
	positionComponent pdata = component_getData (entity_getAs (e, "position"));
	VECTOR3
		new;
	float
		rad = rotation / 180.0 * M_PI,
		m[16] =
		{
			cos (rad), 0, -sin (rad), 0,
			0, 1, 0, 0,
			sin (rad), 0, cos (rad), 0,
			0, 0, 0, 1
		};
/*
	QUAT
		q;
*/
	if (pdata == NULL)
	{
		return FALSE;
	}
/*
	q = quat_eulerToQuat (0, rotation, 0);
	pdata->orientation = quat_multiply (&pdata->orientation, &q);
	^ this might be a fix for orientation that works with quaternions
 */
	//printf ("%s (#%d, %5.2f)\n", __FUNCTION__, entity_GUID (e), rotation);
	new = vectorMultiplyByMatrix (&pdata->pos, m);
	pdata->pos = new;
	new = vectorMultiplyByMatrix (&pdata->view.front, m);
	pdata->view.front = new;
	new = vectorMultiplyByMatrix (&pdata->view.side, m);
	pdata->view.side = new;
	new = vectorMultiplyByMatrix (&pdata->view.up, m);
	pdata->view.up = new;
	return TRUE;
}

void position_updateOnEdgeTraversal (Entity e, struct ground_edge_traversal * t)
{
	positionComponent
		pdata = component_getData (entity_getAs (e, "position"));
	GroundMap
		newGround = component_getData (entity_getAs (t->newGroundEntity, "ground"));
	VECTOR3
		groundOrigin = hexGround_centerDistanceSpace (ground_getMapSize (newGround), t->directionOfMovement),
		newPosition = vectorSubtract (&pdata->pos, &groundOrigin);
	//printf ("%s (#%d, ...): updated position to %5.2f, %5.2f, %5.2f from the local origin\n", __FUNCTION__, entity_GUID (e), newPosition.x, newPosition.y, newPosition.z);
	//printf ("\tbased off of the new ground (%5.2f, %5.2f, %5.2f) @ %d\n", groundOrigin.x, groundOrigin.y, groundOrigin.z, t->directionOfMovement);
	position_set (e, newPosition, t->newGroundEntity);
}

VECTOR3 position_getLocalOffset (const Entity e)
{
	struct position_data
		* pdata = component_getData (entity_getAs (e, "position"));
	if (pdata == NULL)
		return vectorCreate (0, 0, 0);
	return pdata->pos;
}

Entity position_getGroundEntity (const positionComponent p)
{
	return p->mapEntity;
}


int component_position (Object * obj, objMsg msg, void * a, void * b) {
  struct position_data ** cd = NULL;
	char
		* message = NULL;
	Entity
		e = NULL;
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
      *cd = xph_alloc (sizeof (struct position_data));
      (*cd)->pos = vectorCreate (0.0, 0.0, 0.0);
      (*cd)->mapEntity = NULL;
      (*cd)->view.side = vectorCreate (1.0, 0.0, 0.0);
      (*cd)->move.side = vectorCreate (1.0, 0.0, 0.0);
      (*cd)->view.up = vectorCreate (0.0, 1.0, 0.0);
      (*cd)->move.up = vectorCreate (0.0, 1.0, 0.0);
      (*cd)->view.front = vectorCreate (0.0, 0.0, 1.0);
      (*cd)->move.front = vectorCreate (0.0, 0.0, 1.0);
      (*cd)->orientation = quat_create (1.0, 0.0, 0.0, 0.0);
      (*cd)->sensitivity = 0.20;
      //(*cd)->tileOccupying = NULL;
      //(*cd)->tileFootprint = NULL;
      return EXIT_SUCCESS;

    case OM_COMPONENT_DESTROY_DATA:
      cd = a;
      /*
      if ((*cd)->tileFootprint != NULL) {
        vector_destroy ((*cd)->tileFootprint);
      }
      */
      xph_free (*cd);
      return EXIT_SUCCESS;

    case OM_UPDATE:
      return EXIT_FAILURE;

		case OM_COMPONENT_RECEIVE_MESSAGE:
			message = ((struct comp_message *)a)->message;
			e = component_entityAttached (((struct comp_message *)a)->to);
			if (strcmp (message, "GROUND_EDGE_TRAVERSAL") == 0)
			{
				position_updateOnEdgeTraversal (e, b);
				return EXIT_SUCCESS;
			}
			else if (strcmp (message, "CONTROL_INPUT") == 0)
			{
				position_rotateOnMouseInput (e, b);
      		}
			return EXIT_FAILURE;

		default:
			return obj_pass ();
	}
	return EXIT_FAILURE;
}
