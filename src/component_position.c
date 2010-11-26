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
	printf ("%s (#%d, %5.2f)\n", __FUNCTION__, entity_GUID (e), rotation);
	new = vectorMultiplyByMatrix (&pdata->pos, m);
	pdata->pos = new;
	new = vectorMultiplyByMatrix (&pdata->orient.forward, m);
	pdata->orient.forward = new;
	new = vectorMultiplyByMatrix (&pdata->orient.side, m);
	pdata->orient.side = new;
	new = vectorMultiplyByMatrix (&pdata->orient.up, m);
	pdata->orient.up = new;
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
	printf ("%s (#%d, ...): updated position to %5.2f, %5.2f, %5.2f from the local origin\n", __FUNCTION__, entity_GUID (e), newPosition.x, newPosition.y, newPosition.z);
	printf ("\tbased off of the new ground (%5.2f, %5.2f, %5.2f) @ %d\n", groundOrigin.x, groundOrigin.y, groundOrigin.z, t->directionOfMovement);
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
      (*cd)->orient.side = vectorCreate (1.0, 0.0, 0.0);
      (*cd)->orient.up = vectorCreate (0.0, 1.0, 0.0);
      (*cd)->orient.forward = vectorCreate (0.0, 0.0, 1.0);
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
			return EXIT_FAILURE;

		default:
			return obj_pass ();
	}
	return EXIT_FAILURE;
}
