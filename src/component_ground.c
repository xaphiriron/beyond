#include "component_ground.h"

struct ground_comp
{
	Entity
		edges[6];
	Dynarr
		tiles,
		occupants;
	int
		size;
	enum border_type
	{					// THIS ISN'T USED YET.
		BORDER_VOID,	// a non-connected edge is a bottomless pit
		BORDER_WALL		// a non-connected edge is an infinitely-tall wall
	} border;
};

/*
struct ground_location
{
	VECTOR3
		distance;
	unsigned short
		rotation;
};
*/

static GroundMap ground_create ();
static void ground_destroy (GroundMap g);

/***
 * GROUND EDGE/LINKAGE
 */

bool ground_link (Entity a_entity, Entity b_entity, int direction) {
  int
    a_dir = 0,
    b_dir = 0;
  GroundMap a, b;
  if (a_entity == NULL || b_entity == NULL) {
    fprintf (stderr, "%s (%p, %p, ...): NULL entity.\n", __FUNCTION__, a_entity, b_entity);
    return FALSE;
  }
  a = component_getData (entity_getAs (a_entity, "ground"));
  b = component_getData (entity_getAs (b_entity, "ground"));
  if (a == NULL || b == NULL) {
    fprintf (stderr, "%s (%p, %p, ...): NULL ground. (%p, %p)\n", __FUNCTION__, a_entity, b_entity, a, b);
    return FALSE;
  } else if (a->size != b->size) {
    fprintf (stderr, "%s (%p, %p, ...): size mismatch (%d to %d)\n", __FUNCTION__, a_entity, b_entity, a->size, b->size);
    return FALSE;
  }
  // c's mod operator doesn't work right on negative numbers
  if (direction < 0) {
    direction = 6 - ((direction * -1) % 6);
    if (direction == 6)
      direction = 0;
  } else if (direction > 5) {
    direction = direction % 6;
  }
  a_dir = direction;
  b_dir = (3 + direction) % 6;
  if (a->edges[a_dir] != NULL || b->edges[b_dir] != NULL) {
    fprintf (stderr, "%s(%p, %p, ...): ground already linked in either direction (to %d/%p and %d/%p)\n", __FUNCTION__, a_entity, b_entity, a_dir, a->edges[a_dir], b_dir, b->edges[b_dir]);
    return FALSE;
  }
  if (a_dir == b_dir && a == b) {
    // if rotation == 3, the two grounds will be facing each other, and their *_dir values will be equal. this causes the universe to have infinite curvature (an edge is meeting itself) and is disallowed. also, it would create a memory leak because we still try to set up be. mostly it's because of the infinite curvature thing.
    fprintf (stderr, "%s(%p, %p, ...): with given rotation values a face would warp around to touch itself. this is impermissible.\n", __FUNCTION__, a_entity, b_entity);
    return FALSE;
  }
  // there needs to be a whole other section for checking to make sure this
  // connection or its degree of rotation doesn't cause the universe to twist
  // and furl.
  a->edges[a_dir] = b_entity;
  b->edges[b_dir] = a_entity;
  return TRUE;
}

Entity ground_getEdgeConnection (const GroundMap m, short i)
{
	if (m == NULL || i < 0 || i >= 6 || m->edges[i] == NULL)
		return NULL;
	return m->edges[i];
}

Dynarr ground_getOccupants (GroundMap m)
{
	assert (m != NULL);
	return m->occupants;
}

/***
 * WORLD GEOMETRY AND COORDINATE VALUES
 */

VECTOR3 ground_distanceBetweenAdjacentGrounds (int size, int dir) {
  VECTOR3
    r = vectorCreate (0.0, 0.0, 0.0),
    t, u;
  int dir_x = 0;
  if (size < 0) {
    return r;
  }
  if (dir < 0 || dir >= 6) {
    return r;
  }
  // this is the line that determines linkage. if it's (dir + 5) instead, then the linkages will be clockwise. this is not advised.
  dir_x = (dir + 1) % 6;
  //t = hex_linearTileDistance (size * 2 - 2, dir);
  //u = vectorMultiplyByScalar (&u, 0.5);
  t = hex_linearTileDistance (size + 1, dir);
  u = hex_linearTileDistance (size, dir_x);
  r = vectorAdd (&t, &u);
  //printf ("%s (%d, %d): return value is %5.2f, %5.2f, %5.2f\n", __FUNCTION__, size, dir, r.x, r.y, r.z);
  return r;
}

short ground_getMapSize (const GroundMap g)
{
	return g->size;
}

struct hex * ground_getHexAtOffset (GroundMap g, int o)
{
	return *(struct hex **)dynarr_at (g->tiles, o);
}

short ground_getTileAdjacencyIndex (const Entity groundEntity, short r, short k, short i)
{
	GroundMap
		map = component_getData (entity_getAs (groundEntity, "ground"));
	int
		dir;
	if (r <= map->size)
		return -1;
	dir = k;
/* these lines depend on the order of the linkage spin. if for some reason you want to set the linkage to be clockwise, you'll need this uncommented:
	if (i != 0)
		dir = (dir + 1) % 6;
*/
	return dir;
}

bool ground_bridgeConnections (const Entity groundEntity, Entity e)
{
	int
		x, y;
	short
		r, k, i,
		dir;
	Component
		p = entity_getAs (e, "position");
	positionComponent
		pdata = component_getData (p);
	GroundMap
		g = component_getData (entity_getAs (groundEntity, "ground")),
		adjacent = NULL;
	struct ground_edge_traversal
		* trav = NULL;
	//printf ("%s: called\n", __FUNCTION__);
	if (pdata == NULL)
	{
		fprintf (stderr, "%s (#%d, #%d): invalid entity[2] (no position data)\n", __FUNCTION__, entity_GUID (groundEntity), entity_GUID (e));
		return FALSE;
	}
	else if (g == NULL)
	{
		fprintf (stderr, "%s (#%d, #%d): invalid entity[1] (no ground data)\n", __FUNCTION__, entity_GUID (groundEntity), entity_GUID (e));
		return FALSE;
	}
	hex_coordinateAtSpace (&pdata->pos, &x, &y);
	hex_xy2rki (x, y, &r, &k, &i);
	dir = ground_getTileAdjacencyIndex (groundEntity, r, k, i);
	if (dir < 0)
	{
		return TRUE;
	}
	if (g->edges[dir] == NULL || (adjacent = component_getData (entity_getAs (g->edges[dir], "ground"))) == NULL)
		return FALSE;
	//printf ("%s: adjacent ground in %d dir; origin at offset %5.2f, %5.2f, %5.2f\n", __FUNCTION__, dir, newOrigin.x, newOrigin.y, newOrigin.z);
	trav = xph_alloc (sizeof (struct ground_edge_traversal));
	trav->oldGroundEntity = groundEntity;
	trav->newGroundEntity = g->edges[dir];
	trav->directionOfMovement = dir;
	component_messageEntity (p, "GROUND_EDGE_TRAVERSAL", trav);
	xph_free (trav);
	return TRUE;
}

bool ground_placeOnTile (Entity groundEntity, short r, short k, short i, Entity e)
{
	positionComponent
		pdata = component_getData (entity_getAs (e, "position"));
	GroundMap
		map = component_getData (entity_getAs (groundEntity, "ground"));
	struct ground_occupant
		* o;
	if (pdata == NULL || map == NULL || map->size < r)
		return FALSE;
	position_set (e, hex_coordOffset (r, k, i), groundEntity);
	o = xph_alloc (sizeof (struct ground_occupant));
	o->occupant = e;
	o->r = r;
	o->k = k;
	o->i = i;
	dynarr_push (map->occupants, o);
	return TRUE;
}

void ground_bakeTiles (Entity g_entity)
{
	GroundMap
		map = component_getData (entity_getAs (g_entity, "ground")),
		adj_map;
	DynIterator
		it;
	int
		index,
		edge,
		adjIndex,
		dir;
	int
		x, y;
	short
		r, k, i;
	Hex
		hex;
	VECTOR3
		adjPos,
		groundPos;
	if (map == NULL)
		return;
	it = dynIterator_create (map->tiles);
	while (!dynIterator_done (it))
	{
		index = dynIterator_nextIndex (it);
		hex = *(Hex *)dynarr_at (map->tiles, index);
		edge = 0;
		while (edge < 6)
		{
			x = hex->x + XY[edge][0];
			y = hex->y + XY[edge][1];
			hex_xy2rki (x, y, &r, &k, &i);
			dir = ground_getTileAdjacencyIndex (g_entity, r, k, i);
			if (dir < 0)
			{
				adjIndex = hex_linearCoord (r, k, i);
				hex->neighbors[edge] = *(Hex *)dynarr_at (map->tiles, adjIndex);
				edge++;
				continue;
			}
			// hex at the edge of the ground. aah.
			if (map->edges[dir] == NULL || (adj_map = component_getData (entity_getAs (map->edges[dir], "ground"))) == NULL)
			{
				edge++;
				continue;
			}
			// this gets a little too in-depth into vector calculations. i'm not really sure what to do about that. the same values can be calculated more simply if i had the x,y offset of the adjacent ground's center instead of a vector
			groundPos = ground_distanceBetweenAdjacentGrounds (map->size, dir);
			adjPos = hex_coordOffset (r, k, i);
			adjPos = vectorSubtract (&adjPos, &groundPos);
			hex_coordinateAtSpace (&adjPos, &x, &y);
			hex_xy2rki (x, y, &r, &k, &i);
			k = k % 6;
			adjIndex = hex_linearCoord (r, k, i);
			//printf ("from %d, %d {%d %d %d} in %d-ward direction leads to %d %d {%d %d %d}\n", hex->x, hex->y, hex->r, hex->k, hex->i, dir, x, y, r, k, i);
			hex->neighbors[edge] = *(Hex *)dynarr_at (adj_map->tiles, adjIndex);
			edge++;
		}
		hex_bakeEdges (hex);
	}
	dynIterator_destroy (it);
}

struct hex * ground_getHexatCoord (GroundMap g, short r, short k, short i) {
  if (!ground_isInitialized (g) || !ground_isValidRKI (g, r, k, i)) {
    return NULL;
  }
  return *(struct hex **)dynarr_at (g->tiles, hex_linearCoord (r, k, i));
}

/***
 * GROUND MAP FUNCTIONS
 */

static GroundMap ground_create () {
  new (struct ground_comp, g);
  memset (g->edges, '\0', sizeof (Entity) * 6);
  g->tiles = NULL;
  g->size = -1;
  g->border = BORDER_VOID;
  g->occupants = dynarr_create (1, sizeof (struct ground_occupant *));
  return g;
}

static void ground_destroy (GroundMap g)
{
	int
		i = 0;
	if (g->tiles == NULL)
	{
		xph_free (g);
		return;
	}
	while (i < 6)
	{
		g->edges[i++] = NULL;
	}
	dynarr_wipe (g->tiles, (void (*)(void *))hex_destroy);
	dynarr_wipe (g->occupants, xph_free);
	dynarr_destroy (g->tiles);
	dynarr_destroy (g->occupants);
	xph_free (g);
}


void ground_initSize (GroundMap g, int size) {
  if (size < 0) {
    fprintf (stderr, "%s (%p, %d): invalid size\n", __FUNCTION__, g, size);
    return;
  } if (g->size >= 0 || g->tiles != NULL) {
    fprintf (stderr, "%s (%p, %d): ground already has been sized.\n", __FUNCTION__, g, size);
    return;
  }
  g->size = size;
  g->tiles = dynarr_create (hex (size) + 1, sizeof (struct tile *));
}

void ground_fillFlat (GroundMap g, float height) {
  int
    o = 0,
    r = 0, k = 0, i = 0;
  struct hex * h = NULL;
  dynarr_assign (g->tiles, o, hex_create (r, k, i, height));
  o++;
  while (r <= g->size) {
    k = 0;
    while (k < 6) {
      i = 0;
      while (i < r) {
        h = *(struct hex **)dynarr_at (g->tiles, o);
        if (h != NULL) {
          hex_destroy (h);
        }
        dynarr_assign (g->tiles, o, hex_create (r, k, i, height));
        i++;
        o++;
      }
      k++;
    }
    r++;
  }
}

bool ground_isInitialized (const GroundMap g) {
  if (g == NULL || g->tiles == NULL || g->size <= 0) {
    return FALSE;
  }
  return TRUE;
}

bool ground_isValidRKI (const GroundMap g, short r, short k, short i) {
  if (!hex_wellformedRKI (r, k, i) || r > g->size) {
    return FALSE;
  }
  return TRUE;
}

/***
 * THE COMPONENT ITSELF
 */

int component_ground (Object * obj, objMsg msg, void * a, void * b) {
  GroundMap g = NULL;
  switch (msg) {
    case OM_CLSNAME:
      strncpy (a, "ground", 32);
      return EXIT_SUCCESS;
    case OM_CLSINIT:
      return EXIT_SUCCESS;
    case OM_CLSFREE:
      return EXIT_SUCCESS;
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
      g = ground_create ();
      *(void **)a = g;
      return EXIT_SUCCESS;

    case OM_COMPONENT_DESTROY_DATA:
      g = *(void **)a;
      ground_destroy (g);
      *(void **)a = NULL;
      return EXIT_SUCCESS;

    case OM_COMPONENT_RECEIVE_MESSAGE:
      return EXIT_FAILURE;

    case OM_SYSTEM_RECEIVE_MESSAGE:
      return EXIT_FAILURE;

    default:
      return obj_pass ();
  }
}