#include "component_position.h"

struct position_data { // POSITION
	AXES
		view,
		move;
	QUAT
		orientation;
	bool
		dirty;		// view axes don't match the orientation quaternion

	VECTOR3
		pos;		// distance from the center of {ground}
	SUBHEX
		ground;

	hexPos
		position;

	float
		sensitivity;
};

static void position_create (EntComponent comp, EntSpeech speech);
static void position_destroy (EntComponent comp, EntSpeech speech);

static void position_inputResponse (EntComponent comp, EntSpeech speech);


static void position_messageGroundChange (const EntComponent c, SUBHEX oldGround, SUBHEX newGround);
static void position_updateAxesFromOrientation (POSITION pdata);

void position_define (EntComponent position, EntSpeech speech)
{
	component_registerResponse ("position", "__create", position_create);
	component_registerResponse ("position", "__destroy", position_destroy);

	component_registerResponse ("position", "CONTROL_INPUT", position_inputResponse);

	component_registerResponse ("position", "getHex", position_getHex);
	component_registerResponse ("position", "getHexAngle", position_getHexAngle);
}

static void position_create (EntComponent comp, EntSpeech speech)
{
	struct position_data
		* position = xph_alloc (sizeof (struct position_data));
	position->sensitivity = 0.20;

	position->view.side = vectorCreate (1.0, 0.0, 0.0);
	position->move.side = vectorCreate (1.0, 0.0, 0.0);
	position->view.up = vectorCreate (0.0, 1.0, 0.0);
	position->move.up = vectorCreate (0.0, 1.0, 0.0);
	position->view.front = vectorCreate (0.0, 0.0, 1.0);
	position->move.front = vectorCreate (0.0, 0.0, 1.0);
	position->orientation = quat_create (1.0, 0.0, 0.0, 0.0);

	component_setData (comp, position);
}

static void position_destroy (EntComponent comp, EntSpeech speech)
{
	Entity
		this = component_entityAttached (comp);
	POSITION
		position = component_getData (comp);
	position_unset (this);
	xph_free (position);

	component_clearData (comp);
}

static void position_inputResponse (EntComponent comp, EntSpeech speech)
{
	Entity
		this = component_entityAttached (comp);
	position_rotateOnMouseInput (this, speech->arg);
}

void position_set (Entity e, hexPos pos)
{
	POSITION
		positionData = component_getData (entity_getAs (e, "position"));
	if (!positionData)
		return;
	if (positionData->position)
		map_freePos (positionData->position);
	positionData->position = pos;
	entity_speak (e, "positionSet", NULL);
}

hexPos position_get (Entity e)
{
	POSITION
		positionData = component_getData (entity_getAs (e, "position"));
	if (!positionData)
		return NULL;
	return positionData->position;
}

VECTOR3 position_renderCoords (Entity e)
{
	VECTOR3
		platter,
		total;
	POSITION
		position = component_getData (entity_getAs (e, "position"));
	platter = renderOriginDistance (position->ground);
	total = vectorAdd (&platter, &position->pos);
	return total;
}


static void position_messageGroundChange (const EntComponent c, SUBHEX oldGround, SUBHEX newGround)
{
	FUNCOPEN ();
	Entity
		this = component_entityAttached (c);
	static struct position_update
		posUpdate;
	posUpdate.oldGround = oldGround;
	posUpdate.newGround = newGround;
	if (oldGround == NULL || newGround == NULL)
	{
		WARNING ("No point in generating ground change between %p and %p (one or both is NULL)", oldGround, newGround);
		FUNCCLOSE ();
		return;
	}
	else
	{
		posUpdate.relPosition = mapRelativeSubhexWithSubhex (oldGround, newGround);
		if (posUpdate.relPosition == NULL)
		{
			ERROR ("CAN'T HANDLE THIS; NEED TO GENERATE A RELATIVEHEX FROM SCRATCH BUT CAN'T AAA (one that connects %p to %p", oldGround, newGround);
			FUNCCLOSE ();
			return;
		}
	}
	posUpdate.difference = mapRelativeDistance (posUpdate.relPosition);
	entity_speak (this, "positionUpdate", &posUpdate);
	memset (&posUpdate, '\0', sizeof (struct position_update));
	FUNCCLOSE ();
}

void position_unset (Entity e)
{
	FUNCOPEN ();

	EntComponent
		pc = entity_getAs (e, "position");
	SUBHEX
		oldGround;
	POSITION
		pdata = component_getData (pc);
	if (pdata == NULL)
		return;
	//printf ("%s (#%d)...\n", __FUNCTION__, entity_GUID (e));
	oldGround = pdata->ground;
	pdata->ground = NULL;
	position_messageGroundChange (pc, oldGround, NULL);

	FUNCCLOSE ();
}

void position_setDirect (Entity e, VECTOR3 pos, SUBHEX ground)
{
	FUNCOPEN ();

	EntComponent
		pc = entity_getAs (e, "position");
	POSITION
		pdata = component_getData (pc);
	SUBHEX
		oldGround;
	if (pdata == NULL)
	{
		fprintf (stderr, "%s (#%d, ...): entity without position component\n", __FUNCTION__, entity_GUID (e));
		FUNCCLOSE ();
		return;
	}
	pdata->pos = pos;
	oldGround = pdata->ground;
	pdata->ground = ground;

	entity_speak (e, "positionUpdate", NULL);
	if (oldGround != ground)
		position_messageGroundChange (pc, oldGround, pdata->ground);

	FUNCCLOSE ();
}

bool position_move (Entity e, VECTOR3 move)
{
	FUNCOPEN ();

	POSITION
		pdata = component_getData (entity_getAs (e, "position"));
	VECTOR3
		newRawPosition,
		newPosition;
	SUBHEX
		newGround = NULL;
	bool
		validMove = true;
	signed int
		newX, newY;

	if (pdata == NULL)
	{
		ERROR ("Can't move: Entity #%d doesn't have a position component", entity_GUID (e));
		FUNCCLOSE ();
		return false;
	}
	newRawPosition = vectorAdd (&pdata->pos, &move);

	validMove = mapMove (pdata->ground, &newRawPosition, &newGround, &newPosition);
	hex_space2coord (&newPosition, &newX, &newY);
	newPosition.y = subhexGetHeight (subhexData (newGround, newX, newY)) + 90.0;
	if (!validMove)
	{
		if (newGround != NULL && subhexSpanLevel (pdata->ground) < subhexSpanLevel (newGround))
		{
			INFO ("Entity #%d's move has lost resolution: moving to span-%d platter from span-%d platter", entity_GUID (e), subhexSpanLevel (newGround), subhexSpanLevel (pdata->ground));
		}
		else
		{
			ERROR ("Entity #%d's move has gone awry somehow???", entity_GUID (e));
			return false;
		}
	}

	position_setDirect (e, newPosition, newGround);
	FUNCCLOSE ();
	return true;
}

void position_copy (Entity target, const Entity source)
{
	const POSITION
		sourcePosition = component_getData (entity_getAs (source, "position"));
	position_setOrientation (target, sourcePosition->orientation);
	position_setDirect (target, sourcePosition->pos, sourcePosition->ground);
}

void position_setOrientation (Entity e, const QUAT q)
{
	POSITION
		pdata = component_getData (entity_getAs (e, "position"));
	if (pdata == NULL)
		return;
	pdata->orientation = q;
	pdata->dirty = true;	// we assume
}

static void position_updateAxesFromOrientation (POSITION pdata)
{
	QUAT
		r;
	if (pdata == NULL || pdata->dirty == false)
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
	pdata->dirty = false;
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
	// we manually recalculate the one orientation axis value (front y component) we care about right here, instead of updating every single one
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
	pdata->dirty = true;
	entity_speak (e, "orientationUpdate", NULL);
	//printf ("view quat: %7.2f, %7.2f, %7.2f, %7.2f\n", cdata->viewQuat.w, cdata->viewQuat.x, cdata->viewQuat.y, cdata->viewQuat.z);
}

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
	if (p->dirty == true)
		WARNING ("Whoops getting outdated axes (heading)");
	if (fcmp (p->view.up.x, 1.0) || fcmp (p->view.up.x, -1.0))
		return atan2 (p->view.side.z, p->view.front.z);
	return atan2 (-p->view.front.x, p->view.side.x);
}

float position_getPitchR (const POSITION p)
{
	if (p == NULL)
		return 0.0;
	if (p->dirty == true)
		WARNING ("Whoops getting outdated axes (pitch)");
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
	if (p->dirty == true)
		WARNING ("Whoops getting outdated axes (roll)");
	if (fcmp (p->view.up.x, 1.0) || fcmp (p->view.up.x, -1.0))
		return 0.0;
	return atan2 (p->view.up.z, p->view.up.x);
}



bool position_getCoordOffset (const Entity e, signed int * xp, signed int * yp)
{
	struct position_data
		* pdata = component_getData (entity_getAs (e, "position"));
	return position_getCoordOffsetR (pdata, xp, yp);
}

bool position_getCoordOffsetR (const POSITION p, signed int * xp, signed int * yp)
{
	if (p == NULL)
	{
		if (xp)
			*xp = 0;
		if (yp)
			*yp = 0;
		return false;
	}
	hex_space2coord (&p->pos, xp, yp);
	return true;
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


/***
 * POSITION COMPONENT
 */

void position_getHex (EntComponent position, EntSpeech speech)
{
	POSITION
		pData = component_getData (position);
	SUBHEX
		* hex = speech->arg;
	signed int
		x, y;
	hex_space2coord (&pData->pos, &x, &y);
	*hex = subhexData (pData->ground, x, y);
}

void position_getHexAngle (EntComponent position, EntSpeech speech)
{
	POSITION
		pData = component_getData (position);
	float
		* hexAngle = speech->arg;
	*hexAngle = position_getHeadingR (pData);
}