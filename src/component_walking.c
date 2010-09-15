#include "component_walking.h"

// the MOVE value is as follows: a value of 1 will result in the entity crossing tiles at a speed of one per second. 2 is two per second. etc.
Walking * walking_create (float move, float turn) {
  new (Walking, w);
  w->moveSpd = fabs (move) * 60.0;
  w->turnSpd = fabs (turn);
  w->dirsActive = 0;
  return w;
}

void walking_destroy (Walking * w) {
  xph_free (w);
}

void walking_begin_movement (Entity * e, enum walk_move mtype) {
  Component
    * p = NULL,
    * w = NULL;
  struct position_data * pdata = NULL;
  Walking * wdata = NULL;
  VECTOR3 dir;
  p = entity_getAs (e, "position");
  w = entity_getAs (e, "walking");
  if (p == NULL || w == NULL) {
    return;
  }
  pdata = p->comp_data;
  wdata = w->comp_data;
  printf (
    "%s: starting movement in %s direction (w/ mask %d)\n",
    __FUNCTION__,
    mtype == WALK_MOVE_FORWARD ?
      "FORWARD" :
      mtype == WALK_MOVE_BACKWARD ?
        "BACKWARD" :
        mtype == WALK_MOVE_LEFT ?
          "LEFT" :
          "RIGHT",
    wdata->dirsActive
  );
  switch (mtype) {
    case WALK_MOVE_FORWARD:
      dir = vectorMultiplyByScalar (&pdata->orient.forward, wdata->moveSpd);
      break;
    case WALK_MOVE_BACKWARD:
      dir = vectorMultiplyByScalar (&pdata->orient.forward, -wdata->moveSpd / 2.0);
      break;
    case WALK_MOVE_RIGHT:
      dir = vectorMultiplyByScalar (&pdata->orient.side, -wdata->moveSpd / 2.0);
      break;
    case WALK_MOVE_LEFT:
      dir = vectorMultiplyByScalar (&pdata->orient.side, wdata->moveSpd / 2.0);
      break;
    default:
      dir = vectorCreate (0.0, 0.0, 0.0);
      break;
  }
  wdata->dirsActive |= mtype;
  if (wdata->dirsActive == mtype) {
    wdata->dir = dir;
  } else {
    wdata->dir = vectorMultiplyByScalar (&wdata->dir, .7);
    dir = vectorMultiplyByScalar (&dir, .7);
    wdata->dir = vectorAdd (&wdata->dir, &dir);
  }
}

void walking_end_movement (Entity * e, enum walk_move mtype) {
  Component
    * w = NULL,
    * p = NULL;
  Walking * wdata = NULL;
  struct position_data * pdata = NULL;
  VECTOR3 dir;
  w = entity_getAs (e, "walking");
  p = entity_getAs (e, "position");
  if (w == NULL || p == NULL) {
    return;
  }
  wdata = w->comp_data;
  pdata = p->comp_data;
  printf (
    "%s: stopping movement in %s direction (w/ mask %d)\n",
    __FUNCTION__,
    mtype == WALK_MOVE_FORWARD ?
      "FORWARD" :
      mtype == WALK_MOVE_BACKWARD ?
        "BACKWARD" :
        mtype == WALK_MOVE_LEFT ?
          "LEFT" :
          "RIGHT",
    wdata->dirsActive
  );
  switch (mtype) {
    case WALK_MOVE_FORWARD:
      dir = vectorMultiplyByScalar (&pdata->orient.forward, wdata->moveSpd);
      break;
    case WALK_MOVE_BACKWARD:
      dir = vectorMultiplyByScalar (&pdata->orient.forward, -wdata->moveSpd / 2.0);
      break;
    case WALK_MOVE_RIGHT:
      dir = vectorMultiplyByScalar (&pdata->orient.side, -wdata->moveSpd / 2.0);
      break;
    case WALK_MOVE_LEFT:
      dir = vectorMultiplyByScalar (&pdata->orient.side, wdata->moveSpd / 2.0);
      break;
    default:
      dir = vectorCreate (0.0, 0.0, 0.0);
      break;
  }
  if (wdata->dirsActive == mtype || wdata->dirsActive == WALK_MOVE_NONE) {
    wdata->dir = vectorCreate (0.0, 0.0, 0.0);
  } else {
    dir = vectorMultiplyByScalar (&dir, -.7);
    wdata->dir = vectorAdd (&wdata->dir, &dir);
    vectorMultiplyByScalar (&wdata->dir, 1.4);
  }
  wdata->dirsActive &= ~mtype;
}

void walking_begin_turn (Entity * e, enum walk_turn w) {
  printf ("%s (e#%d, %d)\n", __FUNCTION__, e->guid, w);
}

void walking_end_turn (Entity * e, enum walk_turn w) {
}


void walk_move (Entity * e) {
  Component
    * p = NULL,
    * i = NULL,
    * w = NULL;
  struct position_data * pdata = NULL;
  struct integrate_data * idata = NULL;
  Walking * wdata = NULL;
  p = entity_getAs (e, "position");
  i = entity_getAs (e, "integrate");
  w = entity_getAs (e, "walking");
  if ((i == NULL && p == NULL) || w == NULL) {
    return;
  }
  pdata = p->comp_data;
  wdata = w->comp_data;
  //printf ("%s: updating entity #%d (w/ %5.2f, %5.2f, %5.2f)\n", __FUNCTION__, e->guid, wdata->dir.x, wdata->dir.y, wdata->dir.z);
  if (wdata->dirsActive == WALK_MOVE_NONE) {
    return;
  }
  if (i != NULL) {
    idata = i->comp_data;
    addExtraVelocity (idata, &wdata->dir);
  }
}


int component_walking (Object * o, objMsg msg, void * a, void * b) {
  Walking ** wd;
  char * message = NULL;
  Component * c = NULL;
  enum input_responses ir = 0;
  int i = 0;
  Vector * v = NULL;
  Entity * e = NULL;
  switch (msg) {
    case OM_CLSNAME:
      strncpy (a, "walking", 32);
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
      obj_destroy (o);
      return EXIT_SUCCESS;

    case OM_COMPONENT_INIT_DATA:
      wd = a;
      *wd = walking_create (3.0, 2.0);
      return EXIT_SUCCESS;

    case OM_COMPONENT_DESTROY_DATA:
      wd = a;
      walking_destroy (*wd);
      *wd = NULL;
      return EXIT_SUCCESS;

    case OM_UPDATE:
      v = entity_getEntitiesWithComponent (2, "walking", "position");
      i = 0;
      while (i < vector_size (v)) {
        vector_at (e, v, i++);
        walk_move (e);
      }
      return EXIT_FAILURE;

    case OM_POSTUPDATE:
      return EXIT_FAILURE;

    case OM_COMPONENT_RECEIVE_MESSAGE:
      c = ((struct comp_message *)a)->to;
      message = ((struct comp_message *)a)->message;
      if (strcmp (message, "CONTROL_INPUT") == 0) {
        // CONTROL_INPUT second arg is input response index
        ir = (int)b;
        switch (ir) {
          case IR_AVATAR_MOVE_FORWARD:
            walking_begin_movement (c->e, WALK_MOVE_FORWARD);
            break;
          case IR_AVATAR_MOVE_BACKWARD:
            walking_begin_movement (c->e, WALK_MOVE_BACKWARD);
            break;
          case IR_AVATAR_MOVE_LEFT:
            walking_begin_movement (c->e, WALK_MOVE_LEFT);
            break;
          case IR_AVATAR_MOVE_RIGHT:
            walking_begin_movement (c->e, WALK_MOVE_RIGHT);
            break;

          case ~IR_AVATAR_MOVE_FORWARD:
            walking_end_movement (c->e, WALK_MOVE_FORWARD);
            break;
          case ~IR_AVATAR_MOVE_BACKWARD:
            walking_end_movement (c->e, WALK_MOVE_BACKWARD);
            break;
          case ~IR_AVATAR_MOVE_LEFT:
            walking_end_movement (c->e, WALK_MOVE_LEFT);
            break;
          case ~IR_AVATAR_MOVE_RIGHT:
            walking_end_movement (c->e, WALK_MOVE_RIGHT);
            break;

          case IR_AVATAR_PAN_UP:
            walking_begin_turn (c->e, WALK_TURN_UP);
            break;
          case IR_AVATAR_PAN_DOWN:
            walking_begin_turn (c->e, WALK_TURN_DOWN);
            break;
          case IR_AVATAR_PAN_LEFT:
            walking_begin_turn (c->e, WALK_TURN_LEFT);
            break;
          case IR_AVATAR_PAN_RIGHT:
            walking_begin_turn (c->e, WALK_TURN_RIGHT);
            break;

          case ~IR_AVATAR_PAN_UP:
            walking_end_turn (c->e, WALK_TURN_UP);
            break;
          case ~IR_AVATAR_PAN_DOWN:
            walking_end_turn (c->e, WALK_TURN_DOWN);
            break;
          case ~IR_AVATAR_PAN_LEFT:
            walking_end_turn (c->e, WALK_TURN_LEFT);
            break;
          case ~IR_AVATAR_PAN_RIGHT:
            walking_end_turn (c->e, WALK_TURN_RIGHT);
            break;

          default:
            return EXIT_FAILURE;
        }
      }
      return EXIT_SUCCESS;

    case OM_SYSTEM_RECEIVE_MESSAGE:
      message = b;
      return EXIT_FAILURE;

    default:
      return obj_pass ();
  }
}
