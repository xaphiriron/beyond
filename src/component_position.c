#include "component_position.h"

struct position_data { // POSITION
	AXES
		view,
		move;
	QUAT
		orientation;
	VECTOR3
		pos;		// distance from the center of {ground}
	SUBHEX
		ground;
	float
		sensitivity;
	bool
		dirty;		// view axes don't match the orientation quaternion
};


static void position_messageGroundChange (const EntComponent c, SUBHEX oldGround, SUBHEX newGround);

static void position_updateAxesFromOrientation (POSITION pdata);

static void position_messageGroundChange (const EntComponent c, SUBHEX oldGround, SUBHEX newGround)
{
	static struct position_update
		posUpdate;
	posUpdate.oldGround = oldGround;
	posUpdate.newGround = newGround;
	posUpdate.difference = subhexVectorOffset (oldGround, newGround);
	component_messageEntity (c, "positionUpdate", &posUpdate);
}

void position_unset (Entity e)
{
	EntComponent
		pc = entity_getAs (e, "position");
	POSITION
		pdata = component_getData (pc);
	if (pdata == NULL)
		return;
	//printf ("%s (#%d)...\n", __FUNCTION__, entity_GUID (e));
	pdata->ground = NULL;
	position_messageGroundChange (pc, pdata->ground, NULL);
}

void position_destroy (Entity e)
{
	POSITION
		pdata = component_getData (entity_getAs (e, "position"));
	if (pdata == NULL)
		return;
	position_unset (e);
	xph_free (pdata);
}

void position_set (Entity e, VECTOR3 pos, SUBHEX ground)
{
	EntComponent
		pc = entity_getAs (e, "position");
	POSITION
		pdata = component_getData (pc);
	SUBHEX
		oldGround;
	if (pdata == NULL)
	{
		fprintf (stderr, "%s (#%d, ...): entity without position component\n", __FUNCTION__, entity_GUID (e));
		return;
	}
	pdata->pos = pos;
	oldGround = pdata->ground;
	pdata->ground = ground;
	if (oldGround != ground)
		position_messageGroundChange (pc, oldGround, pdata->ground);
}

bool position_move (Entity e, VECTOR3 move)
{
	POSITION
		pdata = component_getData (entity_getAs (e, "position"));
	VECTOR3
		newPosition,
		subhexCentreDistance;
	SUBHEX
		newGround;
	if (pdata == NULL)
	{
		ERROR ("Can't move: Entity #%d doesn't have a position component", entity_GUID (e));
		return FALSE;
	}
	newPosition = vectorAdd (&pdata->pos, &move);
	newGround = mapSubhexAtVectorOffset (pdata->ground, &newPosition);
	if (newGround == pdata->ground)
	{
		pdata->pos = newPosition;
		return TRUE;
	}
	if (subhexSpanLevel (pdata->ground) < subhexSpanLevel (newGround))
		INFO ("Entity #%d has moved from span level %d to %d and lost resolution in the process.", entity_GUID (e), subhexSpanLevel (pdata->ground), subhexSpanLevel (newGround));
	subhexCentreDistance = subhexVectorOffset (pdata->ground, newGround);
	newPosition = vectorSubtract (&subhexCentreDistance, &newPosition);
	position_set (e, newPosition, newGround);
	return TRUE;
}

void position_copy (Entity target, const Entity source)
{
	const POSITION
		sourcePosition = component_getData (entity_getAs (source, "position"));
	position_set (target, sourcePosition->pos, sourcePosition->ground);
	position_setOrientation (target, sourcePosition->orientation);
}

void position_setOrientation (Entity e, const QUAT q)
{
	POSITION
		pdata = component_getData (entity_getAs (e, "position"));
	if (pdata == NULL)
		return;
	pdata->orientation = q;
	pdata->dirty = TRUE;	// we assume
}

static void position_updateAxesFromOrientation (POSITION pdata)
{
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
	POSITION
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

/*
void position_updateOnEdgeTraversal (Entity e, struct ground_edge_traversal * t)
{
	POSITION
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
float position_getHeadingR (const POSITION p)
{
	if (p == NULL)
		return 0.0;
	if (p->dirty == TRUE)
		WARNING ("Whoops getting outdated axes (heading)", NULL);
	if (fcmp (p->view.up.x, 1.0) || fcmp (p->view.up.x, -1.0))
		return atan2 (p->view.side.z, p->view.front.z);
	return atan2 (-p->view.front.x, p->view.side.x);
}

float position_getPitchR (const POSITION p)
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

float position_getRollR (const POSITION p)
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
VECTOR3 position_getLocalOffsetR (const POSITION p)
{
	if (p == NULL)
		return vectorCreate (0, 0, 0);
	return p->pos;
}

QUAT position_getOrientation (const Entity e)
{
	struct position_data
		* pdata = component_getData (entity_getAs (e, "position"));
	return position_getOrientationR (pdata);
}
QUAT position_getOrientationR (const POSITION p)
{
	if (p == NULL)
		return quat_create (1, 0, 0, 0);
	return p->orientation;
}

SUBHEX position_getGround (const Entity e)
{
	struct position_data
		* pdata = component_getData (entity_getAs (e, "position"));
	return position_getGroundR (pdata);
}
SUBHEX position_getGroundR (const POSITION p)
{
	if (p == NULL)
		return NULL;
	return p->ground;
}

const AXES * const position_getViewAxes (const Entity e)
{
	POSITION
		pdata = component_getData (entity_getAs (e, "position"));
	return position_getViewAxesR (pdata);
}

const AXES * const position_getViewAxesR (const POSITION p)
{
	if (p->dirty)
		position_updateAxesFromOrientation (p);
	return &p->view;
}

const AXES * const position_getMoveAxes (const Entity e)
{
	POSITION
		pdata = component_getData (entity_getAs (e, "position"));
	return position_getMoveAxesR (pdata);
}

const AXES * const position_getMoveAxesR (const POSITION p)
{
	if (p->dirty)
		position_updateAxesFromOrientation (p);
	return &p->move;
}



int component_position (Object * obj, objMsg msg, void * a, void * b) {
  struct position_data ** cd = NULL;
	POSITION
		position = NULL;
/*
	struct ground_change
		* change;
*/
	char
		* message = NULL;
	Entity
		e = NULL;
/*
	GroundMap
		ground;
*/
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
		// this is literally the most tortous way to initialize this. why. why.
      *cd = xph_alloc (sizeof (struct position_data));
      (*cd)->pos = vectorCreate (0.0, 0.0, 0.0);
      (*cd)->ground = NULL;
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
			position = component_getData (((struct comp_message *)a)->to);
			
			if (strcmp (message, "CONTROL_INPUT") == 0)
			{
				position_rotateOnMouseInput (e, b);
				return EXIT_SUCCESS;
      		}
/* this will be hypothetically useful when thinking about how the new subdivs handle occupants; right now there's no occupant list. (well, /right now/ the new subdivs don't even work, so...
 * - xph 2011-05-21
			else if (!strcmp (message, "GROUND_CHANGE"))
			{
				change = b;
				//printf ("moving from %p (#%d) to %p (#%d)...\n", change->oldGround, entity_GUID (change->oldGround), change->newGround, entity_GUID (change->newGround));
				ground_removeOccupant (change->oldGround, e);
				if (change->newGround != NULL)
					ground_addOccupant (change->newGround, e);
			}
*/
			else if (!strcmp (message, "getWorldPosition"))
			{
				*(void **)b = subhexGeneratePosition (position->ground);
				return EXIT_SUCCESS;
/*
				//DEBUG ("position: responding to message. (%p, %p)", a, b);
				hex_space2coord (&position->pos, &x, &y);
				hex_xy2rki (x, y, &r, &k, &i);
				ground = component_getData (entity_getAs (position->mapEntity, "ground"));
				*(void **)b = worldhexFromPosition (ground_getWorldPos (ground), r, k, i);
				//DEBUG ("constructed %s", whxPrint (*(void **)b));
*/
			}
			return EXIT_FAILURE;

		default:
			return obj_pass ();
	}
	return EXIT_FAILURE;
}
