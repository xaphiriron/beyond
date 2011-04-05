#include "ground_draw.h"

struct ground_origin_label {
  VECTOR3
    offset;
  Entity
    origin,	// the entity w/ ground centered at 0,0 "world" space
    this;	// the entity w/ ground at x,y in a fill out from the origin
    		// ground. note that "this" may not be unique if the ground
    		// edges wrap around.
	int			// location of this ground in a coordinate grid outward from
		x, y;	// the origin ground.
	unsigned char
		angle;
};

CameraCache OriginCache = NULL;

/***
 * CACHE
 */

CameraCache cameraCache_create ()
{
	CameraCache
		o = xph_alloc (sizeof (struct camera_cache));
	o->origin = NULL;
	o->desiredExtent = 0;
	o->done = TRUE;
	o->nextR = o->nextK = o->nextI = 0;
	o->cache = dynarr_create (1, sizeof (CameraGroundLabel));
	return o;
}

void cameraCache_destroy (CameraCache cache)
{
	if (cache->cache != NULL)
	{
		dynarr_wipe (cache->cache, (void (*)(void *))ground_destroyLabel);
		dynarr_destroy (cache->cache);
	}
	xph_free (cache);
}

void cameraCache_setExtent (unsigned int size)
{
	OriginCache->desiredExtent = size;
}

void cameraCache_update (const TIMER t)
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
/*
	unsigned int
		newR, newK, newI;
*/
	//printf ("%s (%d)...\n", __FUNCTION__, size);
	if (OriginCache == NULL || OriginCache->cache == NULL || dynarr_isEmpty (OriginCache->cache))
	{
		fprintf (stderr, "%s: label cache nonexistant or empty.\n", __FUNCTION__);
		return;
	}
	if (OriginCache->done)
		return;
	label = *(CameraGroundLabel *)dynarr_at (OriginCache->cache, 0);
	if (label == NULL || label->origin != label->this)
	{
		fprintf (stderr, "%s: label cache is invalid or has no base entry. (%p, %p, %p)\n", __FUNCTION__, label, label == NULL ? NULL : label->origin, label == NULL ? NULL : label->this);
		return;
	}
	origin = label->origin;
	originPos = ground_getWorldPos (component_getData (entity_getAs (origin, "ground")));
	label = NULL;
	while (OriginCache->nextR <= OriginCache->desiredExtent)
	{
		cacheOffset = hex_linearCoord (OriginCache->nextR, OriginCache->nextK, OriginCache->nextI);
		if (*(void **)dynarr_at (OriginCache->cache, cacheOffset) != NULL)
		{
			hex_nextValidCoord (&OriginCache->nextR, &OriginCache->nextK, &OriginCache->nextI);
			continue;
		}
		wp = wp_fromRelativeOffset (originPos, groundWorld_getPoleRadius (), OriginCache->nextR, OriginCache->nextK, OriginCache->nextI);
		hex_rki2xy (OriginCache->nextR, OriginCache->nextK, OriginCache->nextI, &newX, &newY);
		adjEntity = groundWorld_loadGroundAt (wp);
		newLabel = ground_createLabel (origin, adjEntity, newX, newY);
		dynarr_assign (OriginCache->cache, cacheOffset, newLabel);
		//wp_getCoords (wp, &newR, &newK, &newI);
		//printf ("from origin: {%d %d %d} makes '%c'{%d %d %d}\n", OriginCache->nextR, OriginCache->nextK, OriginCache->nextI, wp_getPole (wp), newR, newK, newI);
		//printf ("  got entity #%d (%p)\n", entity_GUID (adjEntity), adjEntity);
		//printf ("  placed into cache at %d, %d (%d)\n", newX, newY, cacheOffset);
		wp_destroy (wp);

		hex_nextValidCoord (&OriginCache->nextR, &OriginCache->nextK, &OriginCache->nextI);
		if (outOfTime (t))
		{
			return;
		}
	}
	OriginCache->done = TRUE;

	//printf ("%s (%d): done\n", __FUNCTION__, size);
}

void cameraCache_setGroundEntityAsOrigin (Entity newOrigin)
{
	CameraGroundLabel
		label = NULL;
	Dynarr
		newCache;
	worldPosition
		oldOriginPos,
		newOriginPos;
	signed int
		x, y;
	unsigned int
		distance,
		index,
		r, k, i;
/*
	int
		copied = 0,
		destroyed = 0;
*/
	//printf ("%s...\n", __FUNCTION__);
	if (OriginCache == NULL || OriginCache->cache == NULL)
	{
		fprintf (stderr, "%s: label cache nonexistant or empty.\n", __FUNCTION__);
		return;
	}
	if (OriginCache->origin == newOrigin) {
		return;
	}

	oldOriginPos = ground_getWorldPos (component_getData (entity_getAs (OriginCache->origin, "ground")));
	newOriginPos = ground_getWorldPos (component_getData (entity_getAs (newOrigin, "ground")));
	if (OriginCache->origin == NULL)
	{
		dynarr_wipe (OriginCache->cache, (void (*)(void *))ground_destroyLabel);
		OriginCache->done = FALSE;
		OriginCache->nextR = 1;
		OriginCache->nextK = 0;
		OriginCache->nextI = 0;
		OriginCache->origin = newOrigin;
		label = ground_createLabel (newOrigin, newOrigin, 0, 0);
		dynarr_assign (OriginCache->cache, 0, label);
		label = NULL;
		//printf ("...%s (wipe)\n", __FUNCTION__);
		return;
	}
	distance = wp_pos2xy (oldOriginPos, newOriginPos, groundWorld_getPoleRadius (), &x, &y);
	if (distance > OriginCache->desiredExtent)
	{
		dynarr_wipe (OriginCache->cache, (void (*)(void *))ground_destroyLabel);
		OriginCache->done = FALSE;
		OriginCache->nextR = 1;
		OriginCache->nextK = 0;
		OriginCache->nextI = 0;
		OriginCache->origin = newOrigin;
		label = ground_createLabel (newOrigin, newOrigin, 0, 0);
		dynarr_assign (OriginCache->cache, 0, label);
		label = NULL;
		//printf ("...%s (wipe)\n", __FUNCTION__);
		return;
	}
	//printf ("[recalc]\n");
	OriginCache->done = FALSE;
	OriginCache->nextR = 1;
	OriginCache->nextK = 0;
	OriginCache->nextI = 0;
	OriginCache->origin = newOrigin;
	newCache = dynarr_create (dynarr_capacity (OriginCache->cache), sizeof (CameraGroundLabel));
	while (!dynarr_isEmpty (OriginCache->cache))
	{
		label = *(CameraGroundLabel *)dynarr_pop (OriginCache->cache);
		label->x += x;
		label->y += y;
		distance = hex_coordinateMagnitude (label->x, label->y);
		if (distance > OriginCache->desiredExtent)
		{
			ground_destroyLabel (label);
			//destroyed++;
			continue;
		}
		label->origin = newOrigin;
		label->offset = label_distanceFromOrigin (groundWorld_getGroundRadius (), label->x, label->y);
		label->angle = hex_dirHashFromCoord (x, y);
		hex_xy2rki (label->x, label->y, &r, &k, &i);
		index = hex_linearCoord (r, k, i);
		dynarr_assign (newCache, index, label);
		//copied++;
	}
	dynarr_destroy (OriginCache->cache);
	OriginCache->cache = newCache;
	if (*(void **)dynarr_at (OriginCache->cache, 0) == NULL)
	{
		label = ground_createLabel (newOrigin, newOrigin, 0, 0);
		dynarr_assign (OriginCache->cache, 0, label);
		label = NULL;
	}
	//printf ("%s: copied %d camera labels and destroyed %d. Cache size: %d\n", __FUNCTION__, copied, destroyed, dynarr_size (OriginCache->cache));
	//printf ("...%s\n", __FUNCTION__);
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
  l->origin = origin;
  l->this = this;
  l->x = x;
  l->y = y;
  l->offset = label_distanceFromOrigin (groundWorld_getGroundRadius (), x, y);
	l->angle = hex_dirHashFromCoord (x, y);
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


static CameraGroundLabel lastUsed = NULL;
static bool matrixEstablished = FALSE;
void label_setMatrix (const CameraGroundLabel l)
{
	VECTOR3
		o = label_getOriginOffset (l);
	static const float
		* matrix;
	if (!matrixEstablished)
	{
		matrix = camera_getMatrix (input_getPlayerEntity ());
		matrixEstablished = TRUE;
		if (matrix == NULL)
			glLoadIdentity ();
		else
			glLoadMatrixf (matrix);
	}
	if (lastUsed != NULL)
	{
		fprintf (stderr, "%s: overriding prior used label. (stored: %p)\n", __FUNCTION__, lastUsed);
	}
	lastUsed = l;
	glTranslatef (o.x, o.y, o.z);
}

void label_unsetMatrix (const CameraGroundLabel l)
{
	VECTOR3
		o = label_getOriginOffset (l);
	if (lastUsed != l)
	{
		fprintf (stderr, "%s: used label mismatch. (stored: %p. expecting: %p) unsetting anyway.\n", __FUNCTION__, lastUsed, l);
	}
	glTranslatef (-o.x, -o.y, -o.z);
	lastUsed = NULL;
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
	Component
		g_comp,
		p_comp;
	DynIterator
		it;
	Dynarr
		occupants;
	struct ground_occupant
		* o;
/*
	GLuint
		displayList;
*/

	unsigned int
		r, k, j,
		size = groundWorld_getGroundRadius ();
	unsigned char
		p;

  struct hex * h = NULL;
	//printf ("%s (%p, %p, %p)...\n", __FUNCTION__, g_entity, camera, g_label);

	if (g_entity == NULL || g_label == NULL || g_label->this != g_entity)
	{
		fprintf (stderr, "%s (#%d/%p, ..., %p): nonexistant entity, invalid label, or label does not match ground.\n", __FUNCTION__, entity_GUID (g_entity), g_entity, g_label);
		hex_setDrawColor (1.0, 0.0, 0.0);
		hex_drawFiller (g_label, size);
		return;
	}
	p_comp = entity_getAs (g_entity, "pattern");
	g_comp = entity_getAs (g_entity, "ground");
	g = component_getData (g_comp);
	if (g_comp == NULL || p_comp == NULL)
	{
		fprintf (stderr, "%s (#%d/%p, ..., %p): invalid entity (lacking ground or pattern component)\n", __FUNCTION__, entity_GUID (g_entity), g_entity, g_label);
		hex_setDrawColor (0.8, 0.3, 0.0);
		hex_drawFiller (g_label, size);
		return;
	}
	if (!component_isFullyLoaded (p_comp))
	{
		hex_setDrawColor (0.0, 1.0, 0.2);
		hex_drawFiller (g_label, size);
		return;
	}
	if (!component_isFullyLoaded (g_comp))
	{
		hex_setDrawColor (0.0, 0.4, 1.0);
		hex_drawFiller (g_label, size);
		//fprintf (stderr, "%s (#%d, %p): skipping partially-loaded ground.\n", __FUNCTION__, entity_GUID (g_entity), g_label);
		return;
	}
	label_setMatrix (g_label);
/*
	displayList = ground_getDisplayList (g);
	if (displayList != 0)
	{
		glCallList (displayList);
		label_unsetMatrix (g_label);
		return;
	}
	displayList = glGenLists (1);
	ground_setDisplayList (g, displayList);
	glNewList (displayList, GL_COMPILE);
*/

	p = wp_getPole (ground_getWorldPos (g));
	wp_getCoords (ground_getWorldPos (g), &r, &k, &j);
/*
	printf ("GROUND AT '%c'{%d %d %d}\n", p, r, k, j);
*/
  //printf ("%s: ground label at coord %d,%d\n", __FUNCTION__, g_label->x, g_label->y);
  red = (g_label->x + OriginCache->desiredExtent) / (float)(OriginCache->desiredExtent * 2);
  blue = (g_label->y + OriginCache->desiredExtent) / (float)(OriginCache->desiredExtent * 2);
  green = (red + blue) / 2.0;
  hex_setDrawColor (red, green, blue);
  tilesPerGround = hx (ground_getMapSize (g) + 1);
  i = 0;
	//printf ("DRAWING GROUND AT '%c'{%d %d %d} (%d, %d :: %f %f %f)\n", p, r, k, j, g_label->x, g_label->y, g_label->offset.x, g_label->offset.y, g_label->offset.z);
  while (i < tilesPerGround) {
    h = ground_getHexAtOffset (g, i);
    assert (h != NULL);
    hex_draw (h, camera);
    i++;
  }
/*
	glEndList ();
	glCallList (displayList);
*/
	label_unsetMatrix (g_label);
	occupants = ground_getOccupants (g);
	if (dynarr_isEmpty (occupants))
		return;
	it = dynIterator_create (occupants);
	while (!dynIterator_done (it))
	{
		o = *(struct ground_occupant **)dynIterator_next (it);
		//printf ("%s: have occupant #%d @ %p\n", __FUNCTION__, entity_GUID (o->occupant), o->occupant);
		entity_message (o->occupant, "RENDER", g_label);
	}
	dynIterator_destroy (it);
	//printf ("%s: DONE\n", __FUNCTION__);
}

void ground_draw_fill (Entity camera) {
	//printf ("%s...\n", __FUNCTION__);
	CameraGroundLabel
// 		cameraLabel = camera_getLabel (camera),
		label = NULL;
	Entity
		origin = groundWorld_getEntityOrigin (camera);
  GroundMap
    origin_map = component_getData (entity_getAs (origin, "ground"));
  int
   stepsOutward = groundWorld_getDrawDistance ();
	DynIterator
		it;
/*
	signed int
		x = cameraLabel->x,
		y = cameraLabel->y;
	unsigned char
		cameraDir = hex_dirHashFromYaw (camera_getHeading (camera)),
		labelDir;
*/
	//printf ("YAW: %02x\n", cameraDir);
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
  if (OriginCache->desiredExtent < stepsOutward) {
    cameraCache_setExtent (stepsOutward);
  }
	it = dynIterator_create (OriginCache->cache);
	while (!dynIterator_done (it))
	{
		label = *(CameraGroundLabel *)dynIterator_next (it);
/*
		labelDir = hex_dirHashFromCoord (label->x + x, label->y + y);
		printf ("ground at %d, %d (%d, %d from player): %02x = %02x\n", label->x, label->y, label->x + x, label->y + y, labelDir, hex_dirHashCmp (labelDir, cameraDir));
		if (hex_dirHashCmp (labelDir, cameraDir) > 0x3f)
			continue;
*/
		if (label != NULL && label->this != NULL)
			ground_draw (label->this, camera, label);
	}
	dynIterator_destroy (it);
	//printf ("...%s\n", __FUNCTION__);
}
