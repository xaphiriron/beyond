#include "component_position.h"

static void position_messageGroundChange (const Component c, Entity oldGround, Entity newGround);


static void position_messageGroundChange (const Component c, Entity oldGround, Entity newGround)
{
	struct ground_change
		* g;
	worldPosition
		oldPos, newPos;
	signed int
		x, y;
	unsigned int
		r, k, i;
	g = xph_alloc (sizeof (struct ground_change));
	g->oldGround = oldGround;
	g->newGround = newGround;
	// update this g->dir check by first checking if one of the grounds is null or the distance between them is > 1 (use wp_distance()). if so, it's GROUND_FAR, otherwise do a check for adjacent world position direction (which i will have to write)
	if (oldGround == NULL || newGround == NULL)
		g->dir = GROUND_FAR;
	else
	{
		oldPos = ground_getWorldPos (component_getData (entity_getAs (oldGround, "ground")));
		newPos = ground_getWorldPos (component_getData (entity_getAs (newGround, "ground")));
		wp_pos2xy (oldPos, newPos, groundWorld_getPoleRadius (), &x, &y);
		hex_xy2rki (-x, -y, &r, &k, &i);
		if (r > 1)
		{
			g->dir = GROUND_FAR;
		}
		else
		{
			g->dir = k;
		}
	}
	//printf ("GROUND DIR: %d\n", g->dir);
	component_messageEntity (c, "GROUND_CHANGE", g);
	xph_free (g);
}

void position_unset (Entity e)
{
	Component
		pc = entity_getAs (e, "position");
	positionComponent
		pdata = component_getData (pc);
	Entity
		oldGround;
	if (pdata == NULL)
		return;
	if (pdata->mapEntity)
	{
		ground_removeOccupant (pdata->mapEntity, e);
	}
	oldGround = pdata->mapEntity;
	pdata->mapEntity = NULL;
	position_messageGroundChange (pc, oldGround, NULL);
}

void position_destroy (Entity e)
{
	positionComponent
		pdata = component_getData (entity_getAs (e, "position"));
	if (pdata == NULL)
		return;
	position_unset (e);
	xph_free (pdata);
}

void position_set (Entity e, VECTOR3 pos, Entity mapEntity)
{
	Component
		pc = entity_getAs (e, "position");
	struct position_data
		* pdata = component_getData (pc);
	Entity
		oldGround;
	if (pdata == NULL)
	{
		fprintf (stderr, "%s (#%d, ...): entity without position component\n", __FUNCTION__, entity_GUID (e));
		return;
	}
	pdata->pos = pos;
	oldGround = pdata->mapEntity;
	pdata->mapEntity = mapEntity;
	if (oldGround != mapEntity)
		position_messageGroundChange (pc, oldGround, pdata->mapEntity);
}

bool position_move (Entity e, VECTOR3 move)
{
	Component
		pc = entity_getAs (e, "position");
	struct position_data
		* pos = component_getData (pc);
	GroundMap
		map = NULL,
		newMapData;
	Entity
		oldMap,
		newMap;
	VECTOR3
		oldPos,
		movedPos,
		hexCenter,
		newMapOrigin;
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
	oldMap = pos->mapEntity;
	//printf ("%s: over tile {%d %d %d} (%d, %d) @ %5.2f, %5.2f, %5.2f; real pos (locally): %5.2f, %5.2f, %5.2f\n", __FUNCTION__, r, k, i, x, y, hexCenter.x, hexCenter.y, hexCenter.z, pos->pos.x, pos->pos.y, pos->pos.z);
	oldPos = pos->pos;
	pos->pos = movedPos;
	if (r > groundSize)
	{
		//printf ("%s: outside ground boundary (%d tiles from center)\n", __FUNCTION__, r);
		/* ground_bridgeConnections is kind of a legacy function from the days
		 * when grounds were connected in a graph network instead of by the
		 * world position system, when the edges of a ground could in fact lead
		 * to nowhere. that being said, these days it still calculates the new
		 * ground for the position to be over and returns it. It's conceptually
		 * outdated; when grounds were arbitrarily connected by edges, the only
		 * part that could fully calculate the direction between the two
		 * grounds was right here, and that information needed to be
		 * transmitted to the other components. now with the world position
		 * system, in all cases except the most extreme (a poleradius of 0)
		 * neighboring grounds can only be connected in one direction.
		 * The most important part of this to note is that it requires the position component to have a position vector that's outside the bounds of the ground, but it doesn't do the position updating itself -- it shoots off a entity message, which is then used by position_updateOnEdgeTraversal() to update the position vector.
		 * this really needs to be made simpler.
		 *   - xph 2011-03-15/16
		 */
		if ((newMap = ground_bridgeConnections (pos->mapEntity, e)) == NULL)
		{
			// invalid move attempt: walked outside of ground into void or into ground edge wall, depending on the ground's settings. this should send off an entity message to e, but ihni what would pick it up. collision, i guess.
			pos->pos = oldPos;
			return FALSE;
		}
		else if (newMap != oldMap)
		{
			newMapData = component_getData (entity_getAs (newMap, "ground"));
			pos->mapEntity = newMap;
			newMapOrigin = hexGround_centerDistanceSpace (ground_getMapSize (newMapData), k);
			//printf ("origin of new ground: %5.2f, %5.2f, %5.2f (k: %d); moved position: %5.2f, %5.2f, %5.2f\n", newMapOrigin.x, newMapOrigin.y, newMapOrigin.z, k, movedPos.x, movedPos.y, movedPos.z);
			movedPos = vectorSubtract (&movedPos, &newMapOrigin);
			position_set (e, movedPos, newMap);
		}
	}
	return TRUE;
}

void position_copy (Entity target, const Entity source)
{
	const positionComponent
		sData = component_getData (entity_getAs (source, "position"));
	position_set (target, sData->pos, sData->mapEntity);
	position_setOrientation (target, sData->orientation);
}

void position_setOrientation (Entity e, const QUAT q)
{
	positionComponent
		pdata = component_getData (entity_getAs (e, "position"));
	if (pdata == NULL)
		return;
	pdata->orientation = q;
	pdata->dirty = TRUE;	// we assume
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
	/* FIXME?: wait, why are the side and front vectors transposed here? I
	 * guess it doesn't really matter, but...
	 *  - xph 2011-03-10
	 */
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

/*
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
*/

float position_getHeading (const Entity e)
{
	return position_getHeadingR (component_getData (entity_getAs (e, "position")));
}

float position_getPitch (const Entity e)
{
	return position_getPitchR (component_getData (entity_getAs (e, "position")));
}

float position_getRoll (const Entity e)
{
	return position_getRollR (component_getData (entity_getAs (e, "position")));
}

/* NOTE: I HAVE NO IDEA IF THESE FUNCTIONS WORK RIGHT. AT ALL. SERIOUSLY. SORRY.
 *
 */
float position_getHeadingR (const positionComponent p)
{
	if (p == NULL)
		return 0.0;
	if (p->dirty == TRUE)
		WARNING ("Whoops getting outdated axes (heading)", NULL);
	if (fcmp (p->view.up.x, 1.0) || fcmp (p->view.up.x, -1.0))
		return atan2 (p->view.side.z, p->view.front.z);
	return atan2 (-p->view.front.x, p->view.side.x);
}

float position_getPitchR (const positionComponent p)
{
	if (p == NULL)
		return 0.0;
	if (p->dirty == TRUE)
		WARNING ("Whoops getting outdated axes (pitch)", NULL);
	if (fcmp (p->view.up.x, 1.0))
		return M_PI_2;
	else if (fcmp (p->view.up.x, -1.0))
		return -M_PI_2;
	return asin (p->view.up.x);
}

float position_getRollR (const positionComponent p)
{
	if (p == NULL)
		return 0.0;
	if (p->dirty == TRUE)
		WARNING ("Whoops getting outdated axes (roll)", NULL);
	if (fcmp (p->view.up.x, 1.0) || fcmp (p->view.up.x, -1.0))
		return 0.0;
	return atan2 (p->view.up.z, p->view.up.x);
}

VECTOR3 position_getLocalOffset (const Entity e)
{
	struct position_data
		* pdata = component_getData (entity_getAs (e, "position"));
	return position_getLocalOffsetR (pdata);
}

QUAT position_getOrientation (const Entity e)
{
	struct position_data
		* pdata = component_getData (entity_getAs (e, "position"));
	return position_getOrientationR (pdata);
}

Entity position_getGroundEntity (const Entity e)
{
	struct position_data
		* pdata = component_getData (entity_getAs (e, "position"));
	return position_getGroundEntityR (pdata);
}
VECTOR3 position_getLocalOffsetR (const positionComponent p)
{
	if (p == NULL)
		return vectorCreate (0, 0, 0);
	return p->pos;
}
QUAT position_getOrientationR (const positionComponent p)
{
	if (p == NULL)
		return quat_create (1, 0, 0, 0);
	return p->orientation;
}

Entity position_getGroundEntityR (const positionComponent p)
{
	if (p == NULL)
		return NULL;
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
		position_destroy ((Entity)b);
      /*
      if ((*cd)->tileFootprint != NULL) {
        vector_destroy ((*cd)->tileFootprint);
      }
      xph_free (*cd);
      */
      return EXIT_SUCCESS;

    case OM_UPDATE:
      return EXIT_FAILURE;

		case OM_COMPONENT_RECEIVE_MESSAGE:
			message = ((struct comp_message *)a)->message;
			e = component_entityAttached (((struct comp_message *)a)->to);
			if (strcmp (message, "CONTROL_INPUT") == 0)
			{
				position_rotateOnMouseInput (e, b);
      		}
			return EXIT_FAILURE;

		default:
			return obj_pass ();
	}
	return EXIT_FAILURE;
}
