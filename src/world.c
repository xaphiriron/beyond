#include "world.h"

Object * WorldObject = NULL;

static int world_entwp_search (const void * key, const void * datum);
static int world_entwp_sort (const void * a, const void * b);

WORLD * world_create ()
{
	WORLD
		* w = xph_alloc (sizeof (WORLD));

	//printf ("%s: started\n", __FUNCTION__);
	w->poleRadius = 3;
	w->groundRadius = 2;

	w->groundDistanceDraw = 6;
	w->loadedGrounds = dynarr_create (hex (w->groundDistanceDraw + 1), sizeof (Entity));
	

	return w;
}

void world_destroy (WORLD * w)
{
	DynIterator
		it;
	Entity
		map;
	if (!dynarr_isEmpty (w->loadedGrounds))
	{
		it = dynIterator_create (w->loadedGrounds);
		while (!dynIterator_done (it))
		{
			map = *(Entity *)dynIterator_next (it);
/*
			entity_destroy (map);
*/
		}
	}
	dynarr_destroy (w->loadedGrounds);
	xph_free (w);
}

void world_init ()
{
	WORLD
		* w = obj_getClassData (WorldObject, "world");
	Entity
		poleA = NULL,
		camera = NULL,
		plant = NULL;
	worldPosition
		wp;

	//printf ("creating ground entities:\n");
	wp = wp_create ('a', 0, 0, 0);
	poleA = world_loadGroundAt (wp);
	w->groundOrigin = poleA;
	wp_destroy (wp);

	//printf ("placing plant:\n");
	plant = entity_create ();
	component_instantiateOnEntity ("position", plant);
	plant_generateRandom (plant);
	ground_placeOnTile (poleA, 0, 0, 0, plant);

	//printf ("placing camera:\n");
	camera = entity_create ();
	if (component_instantiateOnEntity ("position", camera))
	{
		ground_placeOnTile (poleA, 0, 0, 0, camera);
		position_move (camera, vectorCreate (0.0, 90.0, 0.0));
	}
	component_instantiateOnEntity ("integrate", camera);
	component_instantiateOnEntity ("camera", camera);
  /*
  if (component_instantiateOnEntity ("collide", e)) {
    setCollideType (e, COLLIDE_SPHERE);
    setCollideRadius (e, 30.0);
  }
  */
	if (component_instantiateOnEntity ("input", camera)) {
		input_addEntity (camera, INPUT_CONTROLLED);
	}
	component_instantiateOnEntity ("walking", camera);
	w->camera = camera;

	//printf ("%s: done!\n", __FUNCTION__);
}

void world_update () {
  // components are stored when first registered by the system object. you can
  // probably change this around at runtime, but it might not be a great idea.
  entitySubsystem_runOnStored (OM_UPDATE);
}

void world_postupdate () {
  entitySubsystem_runOnStored (OM_POSTUPDATE);
}



unsigned int world_getLoadedGroundCount ()
{
	WORLD
		* w = obj_getClassData (WorldObject, "world");
	return dynarr_size (w->loadedGrounds);
}

unsigned int world_getPoleRadius ()
{
	WORLD
		* w = obj_getClassData (WorldObject, "world");
	return w->poleRadius;
}

Entity world_loadGroundAt (const worldPosition wp)
{
	WORLD
		* w = obj_getClassData (WorldObject, "world");
	Entity
		m = *(Entity *)dynarr_search (w->loadedGrounds, world_entwp_search, wp);
	GroundMap
		g;
	worldPosition
		mp;
	unsigned int
		r, k, i;
	if (m)
		return m;

	// this is where the whole "generate ground from scratch or load existant chunk from save" decision has to be made. since we don't have saves, the decision is simple right now!
	m = entity_create ();
	component_instantiateOnEntity ("ground", m);
	g = component_getData (entity_getAs (m, "ground"));
	wp_getCoords (wp, &r, &k, &i);
	printf ("CREATING NEW GROUND AT '%c'{%d %d %d}\n", wp_getPole (wp), r, k, i);
	if (r > w->poleRadius)
		assert (0 && "ground requested with an invalid radius");
	mp = wp_create (wp_getPole (wp), r, k, i);
	ground_initSize (g, w->groundRadius);
	ground_fillFlat (g, ((w->poleRadius - r) / (float)w->poleRadius) * 4.0);
	ground_setWorldPos (g, mp);
	ground_bakeTiles (m);
	// ADD TO LOADED GROUNDS
	dynarr_push (w->loadedGrounds, m);
	dynarr_sort (w->loadedGrounds, world_entwp_sort);
	return m;
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

int world_handler (Object * o, objMsg msg, void * a, void * b) {
  WORLD * w = NULL;
  SYSTEM * s = obj_getClassData (SystemObject, "SYSTEM");
  //struct camera_data * cdata = NULL;
  const float * matrix = NULL;
  char * message = NULL;
  switch (msg) {
    case OM_CLSNAME:
      strncpy (a, "world", 32);
      return EXIT_SUCCESS;
    case OM_CLSINIT:
    case OM_CLSFREE:
      return EXIT_FAILURE;
    case OM_CLSVARS:
      return EXIT_FAILURE;

    case OM_CREATE:
      if (WorldObject != NULL) {
        obj_destroy (o);
        return EXIT_FAILURE;
      }

      //printf ("CREATING WORLD DATA...\n");
      w = world_create ();
      obj_addClassData (o, "world", w);
      WorldObject = o;

      world_init ();

      //printf ("CREATED WORLD DATA\n");
      return EXIT_SUCCESS;

    default:
      break;
  }
  w = obj_getClassData (o, "world");
  switch (msg) {
    case OM_SHUTDOWN:
    case OM_DESTROY:
      WorldObject = NULL;
      world_destroy (w);
      obj_rmClassData (o, "world");
      obj_destroy (o);
      return EXIT_SUCCESS;

    case OM_UPDATE:
      // integrate kids or prepare kids to be integrated
      obj_halt ();
      world_update ();
      return EXIT_FAILURE;

    case OM_POSTUPDATE:
      // do integration things which depend on all world objects having their
      // new position and momentum and the like
      world_postupdate ();
      return EXIT_FAILURE;

    case OM_PRERENDER:
      // called after video:prerender and before this:render
      matrix = camera_getMatrix (w->camera);
      //cdata = component_getData (entity_getAs (w->camera, "camera"));
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glPushMatrix ();
      if (matrix == NULL) {
        glLoadIdentity ();
      } else {
        glLoadMatrixf (matrix);
/*
        printf ("%6.3f %6.3f %6.3f %6.3f\n",
                cdata->m[0], cdata->m[1], cdata->m[2], cdata->m[3]);
        printf ("%6.3f %6.3f %6.3f %6.3f\n",
                cdata->m[4], cdata->m[5], cdata->m[6], cdata->m[7]);
        printf ("%6.3f %6.3f %6.3f %6.3f\n",
                cdata->m[8], cdata->m[9], cdata->m[10], cdata->m[11]);
        printf ("%6.3f %6.3f %6.3f %6.3f\n",
                cdata->m[12], cdata->m[13], cdata->m[14], cdata->m[15]);
        printf ("\n");
 */
      }

      return EXIT_SUCCESS;

		case OM_RENDER:
			// DRAW THINGS
			//draw_grid ();
			if (w->renderWireframe)
				glPolygonMode (GL_FRONT, GL_LINE);
			ground_draw_fill (w->groundOrigin, w->camera, w->groundDistanceDraw);
			if (w->renderWireframe)
				glPolygonMode (GL_FRONT, GL_FILL);
			if (system_getState (s) == STATE_FIRSTPERSONVIEW)
			{
				camera_drawCursor (w->camera);
			}
			return EXIT_SUCCESS;

		case OM_POSTRENDER:
			// called after this:render. do we really need this?
			glPopMatrix ();
			return EXIT_SUCCESS;

		case OM_SYSTEM_RECEIVE_MESSAGE:
			message = b;
			if (strcmp (message, "WIREFRAME_SWITCH") == 0)
			{
				w->renderWireframe ^= 1;
				return EXIT_SUCCESS;
			}
			return EXIT_FAILURE;

		default:
			return obj_pass ();
	}
	return EXIT_FAILURE;
}
