#include "component_walking.h"

struct walkmove_data {
  float
    moveSpd,
    turnSpd;

  int dirsActive;
  bool automoveActive;
};

// the MOVE value is as follows: a value of 1 will result in the entity crossing tiles at a speed of one per second. 2 is two per second. etc.
walkingComponent walking_create (float move, float turn)
{
	walkingComponent w = xph_alloc (sizeof (struct walkmove_data));
	w->moveSpd = fabs (move) * 60.0;
	w->turnSpd = fabs (turn);
	w->dirsActive = 0;
	w->automoveActive = FALSE;
	return w;
}

void walking_destroy (walkingComponent w) {
  xph_free (w);
}

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

void walking_begin_turn (Entity e, enum walk_turn w) {
  printf ("%s (e#%d, %d)\n", __FUNCTION__, entity_GUID (e), w);
}

void walking_end_turn (Entity e, enum walk_turn w) {
}


void walk_move (Entity e)
{
  EntComponent
    p = NULL,
    i = NULL,
    w = NULL;
	POSITION
		pdata = NULL;
  struct integrate_data * idata = NULL;
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
  i = entity_getAs (e, "integrate");
  w = entity_getAs (e, "walking");
  if ((i == NULL && p == NULL) || w == NULL) {
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

  if (i != NULL) {
    idata = component_getData (i);
    addExtraVelocity (idata, &moveDirs);
  } else {
    newpos = vectorMultiplyByScalar (&moveDirs, sys->timestep);
    position_move (e, newpos);
  }
}

void walking_doControlInputResponse (Entity e, const struct input_event * ie)
{
	walkingComponent
		wdata = component_getData (entity_getAs (e, "walking"));
	if (wdata == NULL)
		return;
	switch (ie->ir)
	{
		case IR_AVATAR_AUTOMOVE:
			if (wdata->automoveActive)
				walking_end_movement (e, WALK_MOVE_FORWARD);
			else
				walking_begin_movement (e, WALK_MOVE_FORWARD);
			wdata->automoveActive ^= 1;
			break;
		case IR_AVATAR_MOVE_FORWARD:
			wdata->automoveActive = FALSE;
			walking_begin_movement (e, WALK_MOVE_FORWARD);
			break;
		case IR_AVATAR_MOVE_BACKWARD:
			if (wdata->automoveActive)
			{
				walking_end_movement (e, WALK_MOVE_FORWARD);
				wdata->automoveActive = FALSE;
			}
			walking_begin_movement (e, WALK_MOVE_BACKWARD);
			break;
		case IR_AVATAR_MOVE_LEFT:
			walking_begin_movement (e, WALK_MOVE_LEFT);
			break;
		case IR_AVATAR_MOVE_RIGHT:
			walking_begin_movement (e, WALK_MOVE_RIGHT);
			break;

		case ~IR_AVATAR_MOVE_FORWARD:
			walking_end_movement (e, WALK_MOVE_FORWARD);
			break;
		case ~IR_AVATAR_MOVE_BACKWARD:
			walking_end_movement (e, WALK_MOVE_BACKWARD);
			break;
		case ~IR_AVATAR_MOVE_LEFT:
			walking_end_movement (e, WALK_MOVE_LEFT);
			break;
		case ~IR_AVATAR_MOVE_RIGHT:
			walking_end_movement (e, WALK_MOVE_RIGHT);
			break;

		case IR_AVATAR_PAN_UP:
			walking_begin_turn (e, WALK_TURN_UP);
			break;
		case IR_AVATAR_PAN_DOWN:
			walking_begin_turn (e, WALK_TURN_DOWN);
			break;
		case IR_AVATAR_PAN_LEFT:
			walking_begin_turn (e, WALK_TURN_LEFT);
			break;
		case IR_AVATAR_PAN_RIGHT:
			walking_begin_turn (e, WALK_TURN_RIGHT);
			break;

		case ~IR_AVATAR_PAN_UP:
			walking_end_turn (e, WALK_TURN_UP);
			break;
		case ~IR_AVATAR_PAN_DOWN:
			walking_end_turn (e, WALK_TURN_DOWN);
			break;
		case ~IR_AVATAR_PAN_LEFT:
			walking_end_turn (e, WALK_TURN_LEFT);
			break;
		case ~IR_AVATAR_PAN_RIGHT:
			walking_end_turn (e, WALK_TURN_RIGHT);
			break;

		default:
			break;
	}
}

static Dynarr
	comp_entdata = NULL;
int component_walking (Object * o, objMsg msg, void * a, void * b)
{
	walkingComponent
		* wd;
	char
		* message = NULL;
	EntComponent
		c = NULL;
	DynIterator
		it = NULL;
	Entity
		e = NULL,
		ce = NULL;
	switch (msg)
	{
		case OM_CLSNAME:
			strncpy (a, "walking", 32);
			return EXIT_SUCCESS;
		case OM_CLSINIT:
			comp_entdata = dynarr_create (8, sizeof (Entity));
			return EXIT_SUCCESS;
		case OM_CLSFREE:
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
			obj_destroy (o);
			return EXIT_SUCCESS;
		case OM_COMPONENT_INIT_DATA:
			e = (Entity)b;
			wd = a;
			*wd = walking_create (3.0, 2.0);
			dynarr_push (comp_entdata, e);
			return EXIT_SUCCESS;
		case OM_COMPONENT_DESTROY_DATA:
			e = (Entity)b;
			wd = a;
			walking_destroy (*wd);
			*wd = NULL;
			dynarr_remove_condense (comp_entdata, e);
			return EXIT_SUCCESS;
		case OM_UPDATE:
			it = dynIterator_create (comp_entdata);
			while (!dynIterator_done (it))
			{
				e = *(Entity *)dynIterator_next (it);
				walk_move (e);
			}
			dynIterator_destroy (it);
			it = NULL;
			return EXIT_SUCCESS;

		case OM_POSTUPDATE:
			return EXIT_FAILURE;

		case OM_COMPONENT_RECEIVE_MESSAGE:
			c = ((struct comp_message *)a)->to;
			ce = component_entityAttached (c);
			message = ((struct comp_message *)a)->message;
			if (strcmp (message, "CONTROL_INPUT") == 0) {
				walking_doControlInputResponse (ce, b);
				return EXIT_SUCCESS;
			}
			return EXIT_FAILURE;

		case OM_SYSTEM_RECEIVE_MESSAGE:
			message = b;
			return EXIT_FAILURE;

		default:
			return obj_pass ();
	}
}
