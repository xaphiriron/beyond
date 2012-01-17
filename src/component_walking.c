#include "component_walking.h"


enum walk_move {
  WALK_MOVE_NONE     = 0x00,
  WALK_MOVE_FORWARD  = 0x01,
  WALK_MOVE_BACKWARD = 0x02,
  WALK_MOVE_LEFT     = 0x04,
  WALK_MOVE_RIGHT    = 0x08
};

struct walkmove_data {
  float
    moveSpd,
    turnSpd;

  int dirsActive;
  bool automoveActive;
};

typedef struct walkmove_data * walkingComponent;
typedef struct walkmove_data * walkData;

void walking_start (Entity e)
{
	walkData
		walk = component_getData (entity_getAs (e, "walking"));
	if (!walk)
		return;
	walk->dirsActive = WALK_MOVE_FORWARD;
}

void walking_stop (Entity e)
{
	walkData
		walk = component_getData (entity_getAs (e, "walking"));
	if (!walk)
		return;
	walk->dirsActive = WALK_MOVE_NONE;
	walk->automoveActive = false;
}

void walking_begin_movement (Entity e, enum walk_move w);
void walking_end_movement (Entity e, enum walk_move w);

void walk_move (Entity e);


void walking_begin_movement (Entity e, enum walk_move mtype) {
	walkingComponent
		wdata = component_getData (entity_getAs (e, "walking"));
	if (wdata == NULL || wdata->dirsActive & mtype)
		return;
	wdata->dirsActive |= mtype;
}

void walking_end_movement (Entity e, enum walk_move mtype) {
	walkingComponent
		wdata = component_getData (entity_getAs (e, "walking"));
	if (wdata == NULL || !(wdata->dirsActive & mtype))
		return;
	wdata->dirsActive &= ~mtype;
}


void walk_move (Entity e)
{
  EntComponent
    p = NULL,
    w = NULL;
	POSITION
		pdata = NULL;
  walkingComponent wdata = NULL;
  //const PHYSICS * physics = obj_getClassData (PhysicsObject, "physics");
	const SYSTEM
		* const sys = System;
	const AXES
		* moveAxes;
	VECTOR3
		newpos,
		move,
		moveDirs = vectorCreate (0, 0, 0);
  int
    dirs = 0;
  p = entity_getAs (e, "position");
  w = entity_getAs (e, "walking");
  if ((p == NULL) || w == NULL) {
    return;
  }
  pdata = component_getData (p);
  wdata = component_getData (w);
	//printf ("%s: updating entity #%d (w/ %5.2f, %5.2f, %5.2f)\n", __FUNCTION__, e->guid, wdata->dir.x, wdata->dir.y, wdata->dir.z);
	if (wdata->dirsActive == WALK_MOVE_NONE)
	{
		return;
	}
	moveAxes = position_getMoveAxesR (pdata);
  if (wdata->dirsActive & WALK_MOVE_LEFT) {
    move = vectorMultiplyByScalar (&moveAxes->side, -wdata->moveSpd);
    moveDirs = vectorAdd (&moveDirs, &move);
    dirs++;
  } else if (wdata->dirsActive & WALK_MOVE_RIGHT) {
    move = vectorMultiplyByScalar (&moveAxes->side, wdata->moveSpd);
    moveDirs = vectorAdd (&moveDirs, &move);
    dirs++;
  }

  if (wdata->dirsActive & WALK_MOVE_FORWARD) {
    move = vectorMultiplyByScalar (&moveAxes->front, -wdata->moveSpd);
    moveDirs = vectorAdd (&moveDirs, &move);
    dirs++;
  } else if (wdata->dirsActive & WALK_MOVE_BACKWARD) {
    move = vectorMultiplyByScalar (&moveAxes->front, wdata->moveSpd);
    moveDirs = vectorAdd (&moveDirs, &move);
    dirs++;
  }
  if (dirs >= 2) {
    moveDirs = vectorMultiplyByScalar (&moveDirs, .7);
  }

    newpos = vectorMultiplyByScalar (&moveDirs, sys->timestep);
    position_move (e, newpos);
}

void walking_doControlInputResponse (Entity e, const struct input_event * ie)
{
	walkingComponent
		wdata = component_getData (entity_getAs (e, "walking"));
	if (wdata == NULL)
		return;
	switch (ie->ir)
	{
		case IR_FREEMOVE_AUTOMOVE:
			if (wdata->automoveActive)
				walking_end_movement (e, WALK_MOVE_FORWARD);
			else
				walking_begin_movement (e, WALK_MOVE_FORWARD);
			wdata->automoveActive ^= 1;
			break;
		case IR_FREEMOVE_MOVE_FORWARD:
			wdata->automoveActive = false;
			walking_begin_movement (e, WALK_MOVE_FORWARD);
			break;
		case IR_FREEMOVE_MOVE_BACKWARD:
			if (wdata->automoveActive)
			{
				walking_end_movement (e, WALK_MOVE_FORWARD);
				wdata->automoveActive = false;
			}
			walking_begin_movement (e, WALK_MOVE_BACKWARD);
			break;
		case IR_FREEMOVE_MOVE_LEFT:
			walking_begin_movement (e, WALK_MOVE_LEFT);
			break;
		case IR_FREEMOVE_MOVE_RIGHT:
			walking_begin_movement (e, WALK_MOVE_RIGHT);
			break;

		case ~IR_FREEMOVE_MOVE_FORWARD:
			walking_end_movement (e, WALK_MOVE_FORWARD);
			break;
		case ~IR_FREEMOVE_MOVE_BACKWARD:
			walking_end_movement (e, WALK_MOVE_BACKWARD);
			break;
		case ~IR_FREEMOVE_MOVE_LEFT:
			walking_end_movement (e, WALK_MOVE_LEFT);
			break;
		case ~IR_FREEMOVE_MOVE_RIGHT:
			walking_end_movement (e, WALK_MOVE_RIGHT);
			break;

		default:
			break;
	}
}

static void walking_create (EntComponent comp, EntSpeech speech);
static void walking_destroy (EntComponent comp, EntSpeech speech);
static void walking_inputResponse (EntComponent comp, EntSpeech speech);
static void walking_loseFocus (EntComponent comp, EntSpeech speech);

void walking_define (EntComponent comp, EntSpeech speech)
{

	component_registerResponse ("walking", "__create", walking_create);
	component_registerResponse ("walking", "__destroy", walking_destroy);

	component_registerResponse ("walking", "CONTROL_INPUT", walking_inputResponse);

	component_registerResponse ("walking", "loseFocus", walking_loseFocus);
}

static void walking_create (EntComponent comp, EntSpeech speech)
{
	walkData
		walk = xph_alloc (sizeof (struct walkmove_data));
	// the MOVE value is as follows: a value of 1 will result in the entity crossing tiles at a speed of one per second. 2 is two per second. etc.
	walk->moveSpd = 5.0 * 52.0;
	walk->turnSpd = 2.0;
	walk->dirsActive = 0;
	walk->automoveActive = false;

	component_setData (comp, walk);

}

static void walking_destroy (EntComponent comp, EntSpeech speech)
{
	walkData
		walk = component_getData (comp);
	xph_free (walk);

	component_clearData (comp);
}

static void walking_inputResponse (EntComponent comp, EntSpeech speech)
{
	Entity
		this = component_entityAttached (comp);
	walking_doControlInputResponse (this, speech->arg);
}

static void walking_loseFocus (EntComponent comp, EntSpeech speech)
{
	Entity
		this = component_entityAttached (comp);
	walking_stop (this);
}


void walking_system (Dynarr entities)
{
	Entity
		e;
	EntSpeech
		speech;
	int
		i = 0;

	while ((speech = entitySystem_dequeueMessage ("walking")))
	{
	}

	while ((e = *(Entity *)dynarr_at (entities, i++)))
	{
		walk_move (e);
	}
}