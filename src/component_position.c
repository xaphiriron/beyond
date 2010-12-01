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
/*
void setPosition (Entity e, VECTOR3 pos, GroundMap map) {
  struct position_data * pdata = component_getData (entity_getAs (e, "position"));
  if (pdata == NULL) {
    return;
  }
  pdata->pos = pos;
  pdata->mapLabel = map;

*
  prev = pdata->tileOccupying;
  if (WorldObject != NULL) {
    w = obj_getClassData (WorldObject, "world");
    pdata->tileOccupying = map_hex_at_point (w->map, pos.x, pos.z);
    /
    if (pdata->tileOccupying != NULL) {
      printf ("entity #%d: over tile coordinate %d,%d {%d %d %d}\n", e->guid, pdata->tileOccupying->x, pdata->tileOccupying->y, pdata->tileOccupying->r, pdata->tileOccupying->k, pdata->tileOccupying->i);
    }
    /
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
  *
}
*/

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
	int
		x, y;
	short
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
	hex_coordinateAtSpace (&movedPos, &x, &y);
	hex_xy2rki (x, y, &r, &k, &i);
	hexCenter = hex_coordOffset (r, k, i);
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
	pdata->view.forward.x =		// [2]
		2 * r.x * r.z -
		2 * r.w * r.y;
	pdata->view.forward.y =		// [6]
		2 * r.y * r.z -
		2 * r.w * r.x;
	pdata->view.forward.z =		// [10]
		1 -
		2 * r.x * r.x -
		2 * r.y * r.y;
	pdata->move.forward = vectorCross (&pdata->view.side, &pdata->move.up);
	pdata->move.forward = vectorNormalize (&pdata->move.forward);
	pdata->move.side = vectorCross (&pdata->move.up, &pdata->view.forward);
	pdata->move.side = vectorNormalize (&pdata->move.side);
	pdata->move.up = vectorCreate (0, 1, 0);
	pdata->dirty = FALSE;
}

void position_rotateOnMouseInput (Entity e, const struct input_event * ie)
{
	positionComponent
		pdata = NULL;
	QUAT
		q;
	float
		mag;
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
	//printf ("aaaah mousemotion!  xrel: %d; yrel: %d\n", ie->event->motion.xrel, ie->event->motion.yrel);
	q = quat_eulerToQuat (
		yrel * pdata->sensitivity,
		xrel * pdata->sensitivity,
		0
	);
	pdata->orientation = quat_multiply (&q, &pdata->orientation);
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
	if (pdata == NULL)
	{
		return FALSE;
	}
	//printf ("%s (#%d, %5.2f)\n", __FUNCTION__, entity_GUID (e), rotation);
	new = vectorMultiplyByMatrix (&pdata->pos, m);
	pdata->pos = new;
	new = vectorMultiplyByMatrix (&pdata->view.forward, m);
	pdata->view.forward = new;
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
		groundOrigin = ground_distanceBetweenAdjacentGrounds (ground_getMapSize (newGround), t->directionOfMovement),
		newPosition = vectorSubtract (&pdata->pos, &groundOrigin);
	//printf ("%s (#%d, ...): updated position to %5.2f, %5.2f, %5.2f from the local origin\n", __FUNCTION__, entity_GUID (e), newPosition.x, newPosition.y, newPosition.z);
	//printf ("\tbased off of the new ground (%5.2f, %5.2f, %5.2f) @ %d\n", groundOrigin.x, groundOrigin.y, groundOrigin.z, t->directionOfMovement);
	position_set (e, newPosition, t->newGroundEntity);
	if (t->rotIndex != 0)
	{
		position_rotateAroundGround (e, t->rotIndex * 60.0);
	}
}

VECTOR3 position_getLocalOffset (const positionComponent p)
{
	return p->pos;
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
      (*cd)->view.forward = vectorCreate (0.0, 0.0, 1.0);
      (*cd)->move.forward = vectorCreate (0.0, 0.0, 1.0);
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
