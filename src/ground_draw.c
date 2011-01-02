#include "ground_draw.h"

struct ground_origin_label {
  VECTOR3
    offset;
  Entity
    origin,	// the entity w/ ground centered at 0,0 "world" space
    this;	// the entity w/ ground at x,y in a fill out from the origin
    		// ground. note that "this" may not be unique if the ground
    		// edges wrap around.
  int		// location of this ground in a coordinate grid outward from
    x, y;	// the origin ground. the coordinate grid is not rotated with
    		// ground rotations; it remains clamped to the rotation of the
    		// origin grid (which, due to relativity, is always 0)
};

CameraCache OriginCache = NULL;

/***
 * CACHE
 */

CameraCache cameraCache_create () {
  CameraCache o = xph_alloc (sizeof (struct camera_cache));
  o->origin = NULL;
  o->extent = 0;
  o->cache = dynarr_create (1, sizeof (CameraGroundLabel));
  return o;
}

void cameraCache_destroy (CameraCache cache) {
  if (cache->cache != NULL) {
    dynarr_wipe (cache->cache, (void (*)(void *))ground_destroyLabel);
  }
  dynarr_destroy (cache->cache);
  xph_free (cache);
}


void cameraCache_extend (int size)
{
	Entity
		origin = NULL,
		adjEntity = NULL;
	CameraGroundLabel
		label = NULL,
		newLabel = NULL;
	int
		cacheOffset;
	worldPosition
		originPos,
		wp;
	signed int
		newX, newY;
	unsigned int
		r, k, i,
		newR, newK, newI;
	//printf ("%s (%d)...\n", __FUNCTION__, size);
	if (OriginCache == NULL || OriginCache->cache == NULL || dynarr_isEmpty (OriginCache->cache))
	{
		// augh
		fprintf (stderr, "%s: label cache nonexistant or empty.\n", __FUNCTION__);
		return;
	}
	label = *(CameraGroundLabel *)dynarr_at (OriginCache->cache, 0);
	if (label == NULL || label->origin != label->this)
	{
		fprintf (stderr, "%s: label cache is invalid or has no base entry.\n", __FUNCTION__);
		return;
	}
	origin = label->origin;
	originPos = ground_getWorldPos (component_getData (entity_getAs (origin, "ground")));
	label = NULL;
	while (OriginCache->extent < size)
	{
		r = OriginCache->extent + 1;
		k = 0;
		while (k < 6)
		{
			i = 0;
			while (i < r)
			{
				wp = wp_fromRelativeOffset (originPos, world_getPoleRadius (), r, k, i);
				wp_getCoords (wp, &newR, &newK, &newI);
				//printf ("from origin: {%d %d %d} makes '%c'{%d %d %d}\n", r, k, i, wp_getPole (wp), newR, newK, newI);
				hex_rki2xy (r, k, i, &newX, &newY);
				adjEntity = world_loadGroundAt (wp);
				newLabel = ground_createLabel (origin, adjEntity, newX, newY);
				cacheOffset = hex_linearCoord (r, k, i);
				dynarr_assign (OriginCache->cache, cacheOffset, newLabel);
				wp_destroy (wp);
				i++;
			}
			k++;
		}
		OriginCache->extent++;
	}
	printf ("%d unique grounds loaded into memory\n", world_getLoadedGroundCount ());
}

void cameraCache_setGroundEntityAsOrigin (Entity g)
{
	CameraGroundLabel
		label = NULL;
	if (OriginCache == NULL || OriginCache->cache == NULL)
	{
		// augh
		fprintf (stderr, "%s: label cache nonexistant or empty.\n", __FUNCTION__);
		return;
	}
	if (OriginCache->origin == g) {
		return;
	}
	// in most cases (i.e., any time the new origin is within the extent of
	// the cache, which should be "always" unless there is teleportation) the
	// new origin and the current origin floods will overlap to some degree,
	// and so if we can find the label of the new origin within the existing
	// cache we can reuse some of the labels with minimal updating.
	// as it is right now, the entire cache is destroyed with each origin
	// update, which is increasingly wasteful the closer the new origin is to
	// the existing one and the further out the cache has been calculated.
	if (OriginCache->cache != NULL) {
		dynarr_wipe (OriginCache->cache, (void (*)(void *))ground_destroyLabel);
	}
	OriginCache->extent = 0;
	OriginCache->origin = g;
	label = ground_createLabel (g, g, 0, 0);
	dynarr_assign (OriginCache->cache, 0, label);
	label = NULL;
}

CameraGroundLabel cameraCache_getOriginLabel ()
{
	if (OriginCache == NULL || OriginCache->cache == NULL || dynarr_isEmpty (OriginCache->cache))
	{
		return NULL;
	}
	return *(CameraGroundLabel *)dynarr_front (OriginCache->cache);
}

/***
 * LABEL FUNCTIONS
 */

CameraGroundLabel ground_createLabel (Entity origin, Entity this, int x, int y) {
  CameraGroundLabel l = xph_alloc (sizeof (struct ground_origin_label));
  GroundMap m = component_getData (entity_getAs (this, "ground"));
  l->origin = origin;
  l->this = this;
  l->x = x;
  l->y = y;
  l->offset = label_distanceFromOrigin (ground_getMapSize (m), x, y);
  //printf ("%s: label x,y offset is %d, %d; distance vector is %f, %f, %f\n", __FUNCTION__, x, y, l->offset.x, l->offset.y, l->offset.z);
  return l;
}

void ground_destroyLabel (CameraGroundLabel l) {
  xph_free (l);
}


VECTOR3 label_distanceFromOrigin (int size, short x, short y) {
  VECTOR3
    r = vectorCreate (0.0, 0.0, 0.0),
    x_dist, y_dist;
  if (size < 0 || (x == 0 && y == 0)) {
    return r;
  }
  x_dist = hexGround_centerDistanceSpace (size, 0);
  x_dist = vectorMultiplyByScalar (&x_dist, x);
  y_dist = hexGround_centerDistanceSpace (size, 1);
  y_dist = vectorMultiplyByScalar (&y_dist, y);
  r = vectorAdd (&x_dist, &y_dist);
  return r;
}

bool label_getXY (const CameraGroundLabel l, int * xp, int * yp)
{
	if (xp == NULL || yp == NULL)
		return FALSE;
	*xp = l->x;
	*yp = l->y;
	return TRUE;
}

Entity label_getGroundReference (const CameraGroundLabel l)
{
	if (l == NULL)
		return NULL;
	return l->this;
}

Entity label_getOriginReference (const CameraGroundLabel l)
{
	if (l == NULL)
		return NULL;
	return l->origin;
}

VECTOR3 label_getOriginOffset (const CameraGroundLabel l)
{
	if (l == NULL)
		return vectorCreate (0, 0, 0);
	return l->offset;
}

int label_getCoordinateDistanceFromOrigin (const CameraGroundLabel l)
{
	if (l == NULL)
		return -1;
	return (l->x * l->y >= 0)
		? abs (l->x + l->y)
		: (abs (l->x) > abs (l->y))
			? abs (l->x)
			: abs (l->y);
}


/***
 * DRAWING FUNCTIONS (WHICH CALL draw_hex, ULTIMATELY)
 */

void ground_draw (Entity g_entity, Entity camera, CameraGroundLabel g_label) {
  GroundMap
    g = NULL;
  int
    tilesPerGround = 0,
    i = 0;
  float
    red, green, blue;
	DynIterator
		it;
	Dynarr
		occupants;
	struct ground_occupant
		* o;
  struct hex * h = NULL;
  if (g_entity == NULL || g_label == NULL || g_label->this != g_entity) {
    fprintf (stderr, "%s (#%d, %p): nonexistant entity, invalid label, or label does not match ground.\n", __FUNCTION__, entity_GUID (g_entity), g_label);
    return;
  }
  g = component_getData (entity_getAs (g_entity, "ground"));
  if (g == NULL) {
    fprintf (stderr, "%s (#%d, %p): invalid entity (no ground component)\n", __FUNCTION__, entity_GUID (g_entity), g_label);
    return;
  }
  //printf ("%s: ground label at coord %d,%d\n", __FUNCTION__, g_label->x, g_label->y);
  red = (g_label->x + OriginCache->extent) / (float)(OriginCache->extent * 2);
  blue = (g_label->y + OriginCache->extent) / (float)(OriginCache->extent * 2);
  green = (red + blue) / 2.0;
  hex_setDrawColor (red, green, blue);
  tilesPerGround = hex (ground_getMapSize (g) + 1);
  i = 0;
  while (i < tilesPerGround) {
    h = ground_getHexAtOffset (g, i);
    if (h != NULL) {
      hex_draw (h, camera, g_label);
    }
    i++;
  }
	occupants = ground_getOccupants (g);
	if (dynarr_isEmpty (occupants))
		return;
	it = dynIterator_create (occupants);
	while (!dynIterator_done (it))
	{
		o = *(struct ground_occupant **)dynIterator_next (it);
		//printf ("%s: have occupant #%d @ %p\n", __FUNCTION__, entity_GUID (o->entity), occupant);
		entity_message (o->occupant, "RENDER", g_label);
	}
	dynIterator_destroy (it);
}

void ground_draw_fill (Entity origin, Entity camera, int stepsOutward) {
  CameraGroundLabel
    label = NULL;
  GroundMap
    origin_map = component_getData (entity_getAs (origin, "ground"));
  int
   i = 0,
   limit = 0;
  if (origin_map == NULL) {
    // oh i give up what is even the point???
    return;
  }
  if (OriginCache == NULL) {
    fprintf (stderr, "%s: called while cache uninitialized\n", __FUNCTION__);
    return;
  }
  if (OriginCache->origin != origin) {
	cameraCache_setGroundEntityAsOrigin (origin);
  }
  if (OriginCache->extent < stepsOutward) {
    cameraCache_extend (stepsOutward);
  }
  i = 0;
  limit = hex (stepsOutward + 1);
  while (i < limit) {
    //printf ("...[5.%d] (%d)\n", i, vector_size (OriginCache->cache));
    label = *(CameraGroundLabel *)dynarr_at (OriginCache->cache, i);
    //printf ("   %p, %p\n", label, label->this);
    if (label != NULL) {
      ground_draw (label->this, camera, label);
    }
    i++;
  }
}
