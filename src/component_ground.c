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
	unsigned int
		far;
	GLuint
		displayList;
};

struct ground_world
{
	unsigned int
		poleRadius,
		groundRadius,
		drawDistance,
		partialUnloadDistance,
		loadedUnloadDistance;
	Dynarr
		loadedGrounds,
		partlyLoadedGrounds;

	Entity
		origin;
	worldPosition
		player;
	bool
		originChanged;
};

struct ground_world
	* World = NULL;


static struct ground_world * groundWorld_create ();
static void groundWorld_init ();
static void groundWorld_destroy ();

static int world_entwp_search (const void * key, const void * datum);
static int world_entwp_sort (const void * a, const void * b);

static GroundMap ground_create ();
static void ground_destroy (GroundMap g);


static struct ground_world * groundWorld_create ()
{
	struct ground_world
		* w = xph_alloc (sizeof (struct ground_world));

	w->poleRadius = 6;
	w->groundRadius = 6;
	w->drawDistance = 5;
	w->partialUnloadDistance = 2;
	w->loadedUnloadDistance = 4;

	w->loadedGrounds = dynarr_create (hx (w->drawDistance + 1), sizeof (Entity));
	w->partlyLoadedGrounds = dynarr_create (hx (w->drawDistance + 1), sizeof (Entity));

	w->originChanged = FALSE;

	return w;
}

static void groundWorld_init ()
{
	Entity
		pole,
		player,
		camera,
		plant;
	worldPosition
		wp;

	/* this entire function should be doing something else; the world init
	 * should /maybe/ queue worldgen and then return. maybe it should just
	 * make sure everything was generated empty correctly. at any rate, the
	 * code that handles placing the player at start shouldn't load anything;
	 * it shouldn't be invoked until the spawn has been loaded in the first
	 * place. (which ought to solve one of the camera display issues.)
	 *  - xph 2011-04-03
	 */

	//printf ("%s...\n", __FUNCTION__);

	wp = wp_create ('a', 0, 0, 0);
	pole = groundWorld_loadGroundAt (wp);
	World->origin = pole;
	wp_destroy (wp);

	//printf ("%s: forcing component to generate pole\n", __FUNCTION__);
	// vvv this is to force the ground load started with the loadGround to complete before we start adding occupants to it. this is not a good way to do that.
	component_forceRunLoader (0);

	//printf ("%s: placing plant entity\n", __FUNCTION__);
	plant = entity_create ();
	component_instantiateOnEntity ("position", plant);
	plant_createRandom (plant);
	ground_placeOnTile (pole, 0, 0, 0, plant);

	//printf ("%s: placing player entity\n", __FUNCTION__);
	player = entity_create ();
	if (component_instantiateOnEntity ("position", player))
	{
		ground_placeOnTile (pole, 0, 0, 0, player);
		position_move (player, vectorCreate (0.0, 90.0, 0.0));
	}
	if (component_instantiateOnEntity ("input", player)) {
		input_addEntity (player, INPUT_CONTROLLED);
	}
	component_instantiateOnEntity ("walking", player);

	camera = entity_create ();
	component_instantiateOnEntity ("position", camera);
	if (component_instantiateOnEntity ("camera", camera))
	{
		camera_attachToTarget (camera, player);
		camera_setAsActive (camera);
	}
	if (component_instantiateOnEntity ("input", camera))
	{
		input_addEntity (camera, INPUT_CONTROLLED);
	}

	groundWorld_updateEntityOrigin (player, pole);
	//printf ("...%s\n", __FUNCTION__);
}

static void groundWorld_destroy ()
{
	dynarr_destroy (World->loadedGrounds);
	dynarr_destroy (World->partlyLoadedGrounds);
	xph_free (World);
	World = NULL;
}

unsigned int groundWorld_getPoleRadius ()
{
	return World->poleRadius;
}

unsigned int groundWorld_getGroundRadius ()
{
	return World->groundRadius;
}

unsigned int groundWorld_getDrawDistance ()
{
	return World->drawDistance;
}


// TODO: these are placeholder functions. if i ever add multiplayer support... well, this will be one of the many things i will have to revise.
Entity groundWorld_getEntityOrigin (const Entity e)
{
	return World->origin;
}

void groundWorld_updateEntityOrigin (const Entity e, Entity newOrigin)
{
	World->origin = newOrigin;
	World->originChanged = TRUE;
}

Entity groundWorld_loadGroundAt (const worldPosition wp)
{
	Entity
		x = groundWorld_getGroundAt (wp);
	if (x)
		return x;
	if (groundWorld_groundFileExists (wp))
	{
		x = groundWorld_queueLoad (wp);
		dynarr_push (World->partlyLoadedGrounds, x);
		return x;
	}
	x = groundWorld_queueGeneration (wp);
	dynarr_push (World->partlyLoadedGrounds, x);
	return x;
}

Entity groundWorld_getGroundAt (const worldPosition wp)
{
	Entity
		x = *(Entity *)dynarr_search (World->loadedGrounds, world_entwp_search, wp);
	DynIterator
		it;
	if (x)
		return x;
	//printf ("checking for partly-loaded ground...\n");
	it = dynIterator_create (World->partlyLoadedGrounds);
	//printf ("iterating...\n");
	while (!dynIterator_done (it))
	{
		x = *(Entity *)dynIterator_next (it);
		//printf ("got %p\n", x);
		if (world_entwp_search (&wp, &x) == 0)
		{
			//printf ("%s: found partly loaded ground; using that\n", __FUNCTION__);
			dynIterator_destroy (it);
			return x;
		}
	}
	//printf ("no match\n");
	dynIterator_destroy (it);
	return NULL;
}

void groundWorld_pruneDistantGrounds ()
{
	DynIterator
		it;
	int
		i,
		pcount = 0,
		lcount = 0;
	unsigned int
		distance;
	Entity
		x;
	Component
		c,
		p;
	GroundMap
		g;
	worldPosition
		origin;
	//printf ("%s...\n", __FUNCTION__);
	origin = ground_getWorldPos (component_getData (entity_getAs (groundWorld_getEntityOrigin (input_getPlayerEntity ()), "ground")));
	it = dynIterator_create (World->partlyLoadedGrounds);
	while (!dynIterator_done (it))
	{
		i = dynIterator_nextIndex (it);
		x = *(Entity *)dynarr_at (World->partlyLoadedGrounds, i);
		c = entity_getAs (x, "ground");
		p = entity_getAs (x, "pattern");
		g = component_getData (c);
		if (g == NULL)
		{
			//fprintf (stderr, "...?\n");
			continue;
		}
		component_reweigh (c);
		component_reweigh (p);
		distance = wp_distance (g->wp, origin, World->poleRadius);
		//wp_print (g->wp);
		//wp_print (origin);
		//printf ("distance between the two above: %d\n", distance);
		if (distance <= groundWorld_getDrawDistance ())
		{
			g->far = 0;
			continue;
		}
		g->far++;
		if (g->far >= World->partialUnloadDistance)
		{
			//printf ("UNLOADING PARTIAL ENTITY %p/COMPONENT %p & %p\n", x, c, p);
			component_dropLoad (p);
			component_dropLoad (c);
			entity_destroy (x);
			dynarr_unset (World->partlyLoadedGrounds, i);
			pcount++;
		}
	}
	component_forceLoaderResort ();
	dynIterator_destroy (it);
	if (pcount)
		dynarr_condense (World->partlyLoadedGrounds);
	it = dynIterator_create (World->loadedGrounds);
	while (!dynIterator_done (it))
	{
		i = dynIterator_nextIndex (it);
		x = *(Entity *)dynarr_at (World->loadedGrounds, i);
		c = entity_getAs (x, "ground");
		g = component_getData (c);

		distance = wp_distance (g->wp, origin, World->poleRadius);
		//wp_print (g->wp);
		//wp_print (origin);
		//printf ("distance between the two above: %d\n", distance);
		if (distance <= groundWorld_getDrawDistance ())
		{
			g->far = 0;
			continue;
		}
		g->far++;
		if (g->far >= World->loadedUnloadDistance)
		{
			//printf ("UNLOADING LOADED ENTITY %p/COMPONENT %p\n", x, c);
			// TODO: save ground state before unloading it (once there's a savefile)
			entity_destroy (x);
			dynarr_unset (World->loadedGrounds, i);
			lcount++;
		}
	}
	dynIterator_destroy (it);
	if (lcount)
		dynarr_condense (World->loadedGrounds);

	//printf ("%s: unloaded %d partly-loaded ground%s and %d ground%s\n", __FUNCTION__, pcount, pcount == 1 ? "" : "s", lcount, lcount == 1 ? "" : "s");
}

static int world_entwp_search (const void * key, const void * datum)
{
	const worldPosition
		d = ground_getWorldPos (component_getData (entity_getAs (*(Entity *)datum, "ground")));
/*
	printf ("KEY (%p):\n", *(void **)key);
	wp_print (*(const worldPosition *)key);
	printf ("DATUM (%p):\n", d);
	wp_print (d);
*/
	return wp_compare (*(const worldPosition *)key, d);
}

static int world_entwp_sort (const void * a, const void * b)
{
	const worldPosition
		aa = ground_getWorldPos (component_getData (entity_getAs (*(Entity *)a, "ground"))),
		bb = ground_getWorldPos (component_getData (entity_getAs (*(Entity *)b, "ground")));
/*
	printf ("A (%p):\n", a);
	wp_print (aa);
	printf ("B (%p):\n", b);
	wp_print (bb);
*/
	return wp_compare (aa, bb);
}







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
	if (g == NULL)
		return NULL;
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

GLuint ground_getDisplayList (const GroundMap g)
{
	return g->displayList;
}

void ground_setDisplayList (GroundMap g, GLuint list)
{
	g->displayList = list;
}

void ground_setWorldPos (GroundMap g, worldPosition wp)
{
	g->wp = wp;
}

Entity ground_bridgeConnections (const Entity groundEntity, Entity e)
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
/*
	struct ground_edge_traversal
		* trav = NULL;
*/
	//printf ("%s: called\n", __FUNCTION__);
	if (pdata == NULL)
	{
		fprintf (stderr, "%s (#%d, #%d): invalid entity[2] (no position data)\n", __FUNCTION__, entity_GUID (groundEntity), entity_GUID (e));
		return NULL;
	}
	else if (g == NULL)
	{
		fprintf (stderr, "%s (#%d, #%d): invalid entity[1] (no ground data)\n", __FUNCTION__, entity_GUID (groundEntity), entity_GUID (e));
		return NULL;
	}
	hex_space2coord (&pdata->pos, &x, &y);
	hex_xy2rki (x, y, &r, &k, &i);
	if (r <= g->size)
		return groundEntity;
	newp = wp_fromRelativeOffset (g->wp, World->poleRadius, 1, k, 0);
	adj = groundWorld_loadGroundAt (newp);
	wp_destroy (newp);
	newp = NULL;
/*
	trav = xph_alloc (sizeof (struct ground_edge_traversal));
	trav->oldGroundEntity = groundEntity;
	trav->newGroundEntity = adj;
	trav->directionOfMovement = k;
	component_messageEntity (p, "GROUND_EDGE_TRAVERSAL", trav);
	xph_free (trav);
*/
	return adj;

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
	//printf ("added entity #%d to ground\n", entity_GUID (e));
	dynarr_push (map->occupants, o);
	return TRUE;
}

bool ground_removeOccupant (Entity groundEntity, const Entity e)
{
	
	GroundMap
		map = component_getData (entity_getAs (groundEntity, "ground"));
	DynIterator
		it;
	unsigned int
		i;
	struct ground_occupant
		* oc;
	if (map == NULL)
		return FALSE;
	it = dynIterator_create (map->occupants);
	while (!dynIterator_done (it))
	{
		i = dynIterator_nextIndex (it);
		oc = *(struct ground_occupant **)dynarr_at (map->occupants, i);
		if (oc->occupant == e)
		{
			dynarr_unset (map->occupants, i);
			dynarr_condense (map->occupants);
			return TRUE;
		}
	}
	return FALSE;
}

void ground_bakeInternalTiles (Entity g_entity)
{
	GroundMap
		map = component_getData (entity_getAs (g_entity, "ground"));
	Hex
		h,
		adj;
	signed int
		x = 0, y = 0,
		ax, ay;
	unsigned int
		dir,
		adjIndex,
		mag, k, i,
		hexes = 0,
		radius = groundWorld_getGroundRadius ();
	
	while (hexes < hx (radius + 1))
	{
		h = *(Hex *)dynarr_at (map->tiles, hexes);
		x = h->x;
		y = h->y;
		//printf ("%s: got tile %p (%d, %d)\n", __FUNCTION__, h, x, y);
		dir = 0;
		while (dir < 6)
		{
			ax = h->x + XY[(dir + 1) % 6][0];
			ay = h->y + XY[(dir + 1) % 6][1];
			hex_xy2rki (ax, ay, &mag, &k, &i);
			mag = hex_coordinateMagnitude (ax, ay);
			if (mag > radius)
			{
				dir++;
				continue;
			}
			adjIndex = hex_linearCoord (mag, k, i);
			adj = *(Hex *)dynarr_at (map->tiles, adjIndex);
			h->edgeBase[dir * 2] = FULLHEIGHT (adj, (dir + 4) % 6);
			h->edgeBase[dir * 2 + 1] = FULLHEIGHT (adj, (dir + 3) % 6);
			dir++;
		}
		hexes++;
	}
}

void ground_bakeEdgeTiles (Entity g_entity, unsigned int edge, Entity adj_entity)
{
	Component
		c = entity_getAs (g_entity, "ground"),
		a = entity_getAs (adj_entity, "ground");
	GroundMap
		map = component_getData (c),
		adjMap = component_getData (a);
	Hex
		h,
		adj;
	unsigned int
		radius = groundWorld_getGroundRadius (),
		r = radius,
		k = edge,
		i = 0,
		ak, ai,
		dir, adjDir,
		count;
	
	if (a == NULL || !component_isFullyLoaded (a))
	{
		return;
	}
	while (!(k != edge && i == 1)) // stop after {r k+1 0}
	{
		if (r != radius)
			r = radius;
		h = *(Hex *)dynarr_at (map->tiles, hex_linearCoord (r, k, i));
		//printf ("%s: got tile %p at {%d %d %d}\n", __FUNCTION__, h, r, k, i);
		dir = (edge + 5) % 6;
		count = 0;
		while (count < 2)
		{
			ak = (k + 3) % 6;
			ai = r - (i + (dir == edge));
			if (ai == r)
			{
				ak = (ak + 1) % 6;
				ai = 0;
			}
			//DEBUG ("from {%d %d %d} along %d to {%d %d %d}", r, k, i, dir, r, ak, ai);
			adj = *(HEX *)dynarr_at (adjMap->tiles, hex_linearCoord (r, ak, ai));
			adjDir = (dir + 3) % 6;

			h->edgeBase[dir * 2] = FULLHEIGHT (adj, (dir + 4) % 6);
			h->edgeBase[dir * 2 + 1] = FULLHEIGHT (adj, (dir + 3) % 6);
			adj->edgeBase[adjDir * 2] = FULLHEIGHT (adj, (dir + 1) % 6);
			adj->edgeBase[adjDir * 2 + 1] = FULLHEIGHT (adj, dir);
			dir = (dir + 1) % 6;
			count++;
		}
		hex_nextValidCoord (&r, &k, &i);
	}
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
	struct ground_comp
		* g = xph_alloc (sizeof (struct ground_comp));
  g->tiles = NULL;
  g->size = -1;
	g->far = 0;
  g->occupants = dynarr_create (1, sizeof (struct ground_occupant *));
	g->displayList = 0;
  return g;
}

static void ground_destroy (GroundMap g)
{
	DynIterator
		it;
	struct ground_occupant
		* o;
	if (g->occupants != NULL)
	{
		it = dynIterator_create (g->occupants);
		while (!dynIterator_done (it))
		{
			o = *(struct ground_occupant **)dynIterator_next (it);
			//printf ("%s: unsetting position of entity #%d\n", __FUNCTION__, entity_GUID (o->occupant));
			position_unset (o->occupant);
		}
		dynIterator_destroy (it);
		dynarr_wipe (g->occupants, xph_free);
		dynarr_destroy (g->occupants);
	}
	glDeleteLists (g->displayList, 1);
	wp_destroy (g->wp);
	if (g->tiles == NULL)
	{
		xph_free (g);
		return;
	}
	dynarr_wipe (g->tiles, (void (*)(void *))hex_destroy);
	dynarr_destroy (g->tiles);
	xph_free (g);
}


void ground_initSize (GroundMap g, int size)
{
	//printf ("%s (%p, %d)...\n", __FUNCTION__, g, size);
  if (size < 0) {
    fprintf (stderr, "%s (%p, %d): invalid size\n", __FUNCTION__, g, size);
    return;
  }
  if (g->size >= 0 || g->tiles != NULL) {
    fprintf (stderr, "%s (%p, %d): ground has already been sized. (size: %d, tiles: %p)\n", __FUNCTION__, g, size, g->size, g->tiles);
    return;
  }
  g->size = size;
  g->tiles = dynarr_create (hx (size) + 1, sizeof (struct tile *));
	//printf ("...%s\n", __FUNCTION__);
}

void ground_fillFlat (GroundMap g) {
  int
    o = 0,
    r = 0, k = 0, i = 0;
  struct hex * h = NULL;
  dynarr_assign (g->tiles, o, hex_create (r, k, i));
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
        dynarr_assign (g->tiles, o, hex_create (r, k, i));
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




unsigned int ground_entDistance (const Entity a, const Entity b)
{
	const worldPosition
		awp = ground_getWorldPos (component_getData (entity_getAs (a, "ground"))),
		bwp = ground_getWorldPos (component_getData (entity_getAs (b, "ground")));
	return wp_distance (awp, bwp, World->poleRadius);
}







void groundWorld_patternLoad (Component c)
{
	component_setLoadGoal (c, 1);

	// WORLD GEN STUFF HERE !!!!

	component_updateLoadAmount (c, 1);
	component_setLoadComplete (c);
	return;
}

void groundWorld_groundLoad (Component c)
{
	worldPosition
		* adjacent;
	Entity
		x = component_entityAttached (c),
		adj;
	GroundMap
		g = component_getData (c);
	Hex
		hex;
	unsigned char
		p;
	unsigned int
		r, k, i;
	int
		index;
	DynIterator
		it;
	while (!component_isFullyLoaded (entity_getAs (x, "pattern")))
	{
		fprintf (stderr, "cannot load ground component (%p): pattern component (%p) isn't loaded yet. (on entity #%d)\n", c, entity_getAs (x, "pattern"), entity_GUID (x));
		return;
	}
	component_setLoadGoal (c, 1);

	p = wp_getPole (g->wp);
	wp_getCoords (g->wp, &r, &k, &i);

	ground_initSize (g, World->groundRadius);

	ground_fillFlat (g);
	it = dynIterator_create (g->tiles);
	i = 0;
	while (!dynIterator_done (it))
	{
		i = dynIterator_nextIndex (it);
		hex = *(HEX *)dynarr_at (g->tiles, i);
		if (i <= 6)
		{
			hexSetHeight (hex, i);
			hexSetCorners (hex, 0, -2, -2, 0, 2, 2);
		}
		else if (i >= hx (World->groundRadius))
		{
			hexSetHeight (hex, 4);
			hexSetCorners (hex,
				i % 2 ? -1 : 1,
				i % 2 ? -1 : 1,
				0,
				i % 2 ? 1 : -1,
				i % 2 ? 1 : -1,
				0
			);
		}
		else
		{
			hexSetHeight (hex, 2);
			hexSetCorners (hex, 0, 0, 0, 0, 0, 0);
			//hexSetCornersRandom (hex);
		}
	}

	ground_bakeInternalTiles (x);
	adjacent = wp_adjacent (g->wp, World->poleRadius);
	i = 0;
	while (i < 6)
	{
		adj = groundWorld_getGroundAt (adjacent[i]);
		if (adj != NULL)
		{
			ground_bakeEdgeTiles (x, i, adj);
		}
		i++;
	}
	wp_destroyAdjacent (adjacent);

	dynarr_push (World->loadedGrounds, x);
	dynarr_sort (World->loadedGrounds, world_entwp_sort);
	index = in_dynarr (World->partlyLoadedGrounds, x);
	if (index >= 0)
		dynarr_unset (World->partlyLoadedGrounds, index);

	component_updateLoadAmount (c, 1);
	component_setLoadComplete (c);

	//printf ("loaded ground at '%c'{%d %d %d}\n", p, r, k, i);
	return;
}

unsigned char groundWorld_patternWeigh (Component c)
{
	worldPosition
		w = ground_getWorldPos (component_getData (entity_getAs (groundWorld_getEntityOrigin (input_getPlayerEntity ()), "ground"))),
		l = ground_getWorldPos (component_getData (entity_getAs (component_entityAttached (c), "ground")));
	unsigned int
		dd = groundWorld_getDrawDistance (),
		d;
	unsigned char
		r;
	if (w == NULL || l == NULL)
		return 255;
	d = wp_distance (l, w, World->poleRadius);
	r = (dd - d) / (float)dd * 192 + 63;
	//printf ("%s: weight %d on pattern %d distant (of %d) from origin\n", __FUNCTION__, r, d, dd);
	return r;
}

unsigned char groundWorld_groundWeigh (Component c)
{
	worldPosition
		w = ground_getWorldPos (component_getData (entity_getAs (groundWorld_getEntityOrigin (input_getPlayerEntity ()), "ground")));
	unsigned int
		dd = groundWorld_getDrawDistance (),
		d;
	unsigned char
		r;
	if (!component_isFullyLoaded (entity_getAs (component_entityAttached (c), "pattern")))
		return 0;
	if (w == NULL)
		return 127;
	d = wp_distance (ground_getWorldPos (component_getData (c)), w, World->poleRadius);
	r = (dd - d) / (float)dd * 192 + 63;
	//printf ("%s: weight %d on ground %d distant (of %d) from origin\n", __FUNCTION__, r, d, dd);
	return r;
}


bool groundWorld_groundFileExists (const worldPosition wp)
{
	return FALSE;
}

Entity groundWorld_queueLoad (const worldPosition wp)
{
	return NULL;
}

Entity groundWorld_queueGeneration (const worldPosition wp)
{
	Entity
		x = entity_create ();
	worldPosition
		dup = wp_duplicate (wp);
	GroundMap
		g;
	Component
		pattern,
		ground;
	//printf ("%s (%p)...\n", __FUNCTION__, wp);
	component_instantiateOnEntity ("pattern", x);
	component_instantiateOnEntity ("ground", x);
	pattern = entity_getAs (x, "pattern");
	ground = entity_getAs (x, "ground");
	g = component_getData (ground);
	ground_setWorldPos (g, dup);
	component_setAsLoadable (pattern);
	component_setAsLoadable (ground);
	// ^ or component_activateLoading or SOMETHING. the actual function just adds that component to the weighed list of COMPONENTS TO LOAD, which will be processed in any order.
	// (the idea is that components are initialized to some base state (here) and then stream loaded from there (in a callback function; currently the groundWorld_[x]Load placeholders))
	//printf ("...%s\n", __FUNCTION__);
	return x;
}




/***
 * THE COMPONENT ITSELF
 */

int component_ground (Object * obj, objMsg msg, void * a, void * b)
{
	GroundMap
		g = NULL;
	switch (msg)
	{
		case OM_CLSNAME:
			strncpy (a, "ground", 32);
			return EXIT_SUCCESS;
		case OM_CLSINIT:
			World = groundWorld_create ();
			return EXIT_SUCCESS;
		case OM_CLSFREE:
			groundWorld_destroy ();
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
      obj_destroy (obj);
      return EXIT_SUCCESS;

		case OM_START:
			printf ("TRYING GROUND/WORLD INIT\n");
			groundWorld_init ();
			printf ("DONE GROUND/WORLD INIT\n");
			return EXIT_SUCCESS;

		case OM_UPDATE:
			if (World->originChanged == TRUE)
			{
				groundWorld_pruneDistantGrounds ();
				World->originChanged = FALSE;
			}
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

		case OM_COMPONENT_GET_LOADER_CALLBACK:
			*(void **)a = groundWorld_groundLoad;
			return EXIT_SUCCESS;
		case OM_COMPONENT_GET_WEIGH_CALLBACK:
			*(void **)a = groundWorld_groundWeigh;
			return EXIT_SUCCESS;

    default:
      return obj_pass ();
  }
}

// this is a placeholder
int component_pattern (Object * obj, objMsg msg, void * a, void * b)
{
	switch (msg)
	{
		case OM_CLSNAME:
			strncpy (a, "pattern", 32);
			return EXIT_SUCCESS;
		case OM_CLSINIT:
		case OM_CLSFREE:
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
			obj_destroy (obj);
			return EXIT_SUCCESS;

		case OM_COMPONENT_GET_LOADER_CALLBACK:
			*(void **)a = groundWorld_patternLoad;
			return EXIT_SUCCESS;
		case OM_COMPONENT_GET_WEIGH_CALLBACK:
			*(void **)a = groundWorld_patternWeigh;
			return EXIT_SUCCESS;

		default:
			return EXIT_FAILURE;
	}
}
