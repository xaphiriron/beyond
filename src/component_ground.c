#include "component_ground.h"

struct ground_comp
{
	worldPosition
		wp;
	Dynarr
		tiles,
		occupants;
	int
		size;
};

static GroundMap ground_create ();
static void ground_destroy (GroundMap g);


Dynarr ground_getOccupants (GroundMap m)
{
	assert (m != NULL);
	return m->occupants;
}

/***
 * WORLD GEOMETRY AND COORDINATE VALUES
 */

short ground_getMapSize (const GroundMap g)
{
	return g->size;
}

const worldPosition ground_getWorldPos (const GroundMap g)
{
	return g->wp;
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


void ground_setWorldPos (GroundMap g, worldPosition wp)
{
	g->wp = wp;
}

bool ground_bridgeConnections (const Entity groundEntity, Entity e)
{
	signed int
		x, y;
	unsigned int
		r, k, i;
	Component
		p = entity_getAs (e, "position");
	positionComponent
		pdata = component_getData (p);
	GroundMap
		g = component_getData (entity_getAs (groundEntity, "ground"));
	worldPosition
		newp;
	Entity
		adj;
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
	hex_space2coord (&pdata->pos, &x, &y);
	hex_xy2rki (x, y, &r, &k, &i);
	if (r <= g->size)
		return TRUE;
	newp = wp_fromRelativeOffset (g->wp, world_getPoleRadius (), 1, k, 0);
	adj = world_loadGroundAt (newp);
	wp_destroy (newp);
	newp = NULL;
	trav = xph_alloc (sizeof (struct ground_edge_traversal));
	trav->oldGroundEntity = groundEntity;
	trav->newGroundEntity = adj;
	trav->directionOfMovement = k;
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
	position_set (e, hex_coord2space (r, k, i), groundEntity);
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
		map = component_getData (entity_getAs (g_entity, "ground"));
	DynIterator
		it;
	int
		index,
		edge,
		adjIndex,
		dir;
	signed int
		x, y;
	unsigned int
		r, k, i;
	Hex
		hex;
	//printf ("%s (#%d): start\n", __FUNCTION__, entity_GUID (g_entity));
	if (map == NULL)
		return;
	it = dynIterator_create (map->tiles);
	while (!dynIterator_done (it))
	{
		index = dynIterator_nextIndex (it);
		hex = *(Hex *)dynarr_at (map->tiles, index);
		//printf ("iterating over the %d-th index (with hex %p {%d %d %d})\n", index, hex, hex->r, hex->k, hex->i);
		edge = 0;
		while (edge < 6)
		{
			x = hex->x + XY[edge][0];
			y = hex->y + XY[edge][1];
			hex_xy2rki (x, y, &r, &k, &i);
			//printf ("\tedge %d: %d, %d {%d %d %d}\n", edge, x, y, r, k, i);
			dir = ground_getTileAdjacencyIndex (g_entity, r, k, i);
			if (dir < 0)
			{
				adjIndex = hex_linearCoord (r, k, i);
				hex->neighbors[edge] = *(Hex *)dynarr_at (map->tiles, adjIndex);
				//printf ("set %d-th edge of %p to %p\n", edge, hex, hex->neighbors[edge]);
				edge++;
				continue;
			}
			// hex at the edge of the ground. aah.
			/***
			 * we're just going to ignore this for now -- the proper thing to
			 * do is ask if the adjacent ground is loaded (but not require that
			 * it be) and if so, hook up its neighbors.
			 */
			edge++;
			continue;
		}
		hex_bakeEdges (hex);
	}
	dynIterator_destroy (it);
	//printf ("%s (#%d): done!\n", __FUNCTION__, entity_GUID (g_entity));
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
  g->tiles = NULL;
  g->size = -1;
  g->occupants = dynarr_create (1, sizeof (struct ground_occupant *));
  return g;
}

static void ground_destroy (GroundMap g)
{
	wp_destroy (g->wp);
	if (g->tiles == NULL)
	{
		xph_free (g);
		return;
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
        //printf ("created {%d %d %d} (%d)\n", r, k, i, hex_linearCoord (r, k, i));
        i++;
        o++;
      }
      k++;
    }
    r++;
  }
  //printf ("tile list: %d items long\n", dynarr_size (g->tiles));
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