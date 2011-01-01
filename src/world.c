#include "world.h"

Object * WorldObject = NULL;

WORLD * world_create ()
{
	WORLD
		* w = xph_alloc (sizeof (WORLD));
	Entity
		groundG = NULL,
		groundH = NULL,
		groundI = NULL,
		camera = NULL,
		plant = NULL;
	GroundMap
		g = NULL,
		h = NULL,
		i = NULL;
	Hex
		hex = NULL;

	groundG = entity_create ();
	groundH = entity_create ();
	groundI = entity_create ();
	component_instantiateOnEntity ("ground", groundG);
	component_instantiateOnEntity ("ground", groundH);
	component_instantiateOnEntity ("ground", groundI);
	g = component_getData (entity_getAs (groundG, "ground"));
	h = component_getData (entity_getAs (groundH, "ground"));
	i = component_getData (entity_getAs (groundI, "ground"));
	if (g == NULL || h == NULL || i == NULL)
	{
		fprintf (stderr, "%s: starter ground entities turned out NULL for some reason. We're doomed; nothing to do but\n", __FUNCTION__);
		exit (EXIT_FAILURE);
	}
	//printf ("initializing ground entities:\n");
	ground_initSize (g, 12);
	ground_fillFlat (g, 1.0);

	ground_initSize (h, 12);
	ground_fillFlat (h, 1.0);

	ground_initSize (i, 12);
	ground_fillFlat (i, 1.0);

	//printf ("sloping tiles:\n");
/*
	hex = ground_getHexatCoord (g, 0, 0, 0);
	hex_setSlope (hex, HEX_TOP, 0, 0, 0);
	hex = ground_getHexatCoord (g, 1, 0, 0);
	hex_setSlope (hex, HEX_TOP, 0, 0, 0);
	hex = ground_getHexatCoord (g, 1, 1, 0);
	hex_setSlope (hex, HEX_TOP, 0, 0, 0);
	hex = ground_getHexatCoord (g, 1, 2, 0);
	hex_setSlope (hex, HEX_TOP, 0, 0, 0);
	hex = ground_getHexatCoord (g, 1, 3, 0);
	hex_setSlope (hex, HEX_TOP, 0, 0, 0);
	hex = ground_getHexatCoord (g, 1, 4, 0);
	hex_setSlope (hex, HEX_TOP, 0, 0, 0);
	hex = ground_getHexatCoord (g, 1, 5, 0);
	hex_setSlope (hex, HEX_TOP, 0, 0, 0);
	hex = ground_getHexatCoord (g, 2, 0, 1);
	hex_setSlope (hex, HEX_TOP, 2, 2, 2);
	hex = ground_getHexatCoord (g, 2, 1, 1);
	hex_setSlope (hex, HEX_TOP, 2, 2, 2);
	hex = ground_getHexatCoord (g, 2, 2, 1);
	hex_setSlope (hex, HEX_TOP, 2, 2, 2);
	hex = ground_getHexatCoord (g, 2, 3, 1);
	hex_setSlope (hex, HEX_TOP, 2, 2, 2);
	hex = ground_getHexatCoord (g, 2, 4, 1);
	hex_setSlope (hex, HEX_TOP, 2, 2, 2);
	hex = ground_getHexatCoord (g, 2, 5, 1);
	hex_setSlope (hex, HEX_TOP, 2, 2, 2);
	hex = ground_getHexatCoord (g, 3, 0, 0);
	hex_setSlope (hex, HEX_TOP, 3, 3, 3);
	hex = ground_getHexatCoord (g, 3, 2, 0);
	hex_setSlope (hex, HEX_TOP, 3, 3, 3);
	hex = ground_getHexatCoord (g, 3, 4, 0);
	hex_setSlope (hex, HEX_TOP, 3, 3, 3);

	hex = ground_getHexatCoord (h, 1, 0, 0);
	hex_setSlope (hex, HEX_TOP, 2, 2, 2.5);
	hex = ground_getHexatCoord (h, 1, 1, 0);
	hex_setSlope (hex, HEX_TOP, 2, 2.5, 2);
	hex = ground_getHexatCoord (h, 2, 0, 1);
	hex_setSlope (hex, HEX_TOP, 2.5, 2, 2);
	hex = ground_getHexatCoord (h, 1, 3, 0);
	hex_setSlope (hex, HEX_TOP, 2, 2, 2.5);
	hex = ground_getHexatCoord (h, 1, 4, 0);
	hex_setSlope (hex, HEX_TOP, 2, 2.5, 2);
	hex = ground_getHexatCoord (h, 2, 3, 1);
	hex_setSlope (hex, HEX_TOP, 2.5, 2, 2);
	hex = ground_getHexatCoord (h, 3, 2, 0);
	hex_setSlope (hex, HEX_TOP, 3, 3.5, 2.5);
	hex = ground_getHexatCoord (h, 3, 5, 0);
	hex_setSlope (hex, HEX_TOP, 2.5, 3.5, 3);

	hex = ground_getHexatCoord (i, 1, 0, 0);
	hex_setSlope (hex, HEX_TOP, 2, 2, 2);
	hex = ground_getHexatCoord (i, 1, 2, 0);
	hex_setSlope (hex, HEX_TOP, 2, 2, 2);
	hex = ground_getHexatCoord (i, 1, 4, 0);
	hex_setSlope (hex, HEX_TOP, 2, 2, 2);
	hex = ground_getHexatCoord (i, 2, 1, 0);
	hex_setSlope (hex, HEX_TOP, 3, 3, 3);
	hex = ground_getHexatCoord (i, 2, 3, 0);
	hex_setSlope (hex, HEX_TOP, 3, 3, 3);
	hex = ground_getHexatCoord (i, 2, 5, 0);
	hex_setSlope (hex, HEX_TOP, 3, 3, 3);
	hex = ground_getHexatCoord (i, 3, 0, 0);
	hex_setSlope (hex, HEX_TOP, 2, 2, 2);
	hex = ground_getHexatCoord (i, 3, 1, 0);
	hex_setSlope (hex, HEX_TOP, 2, 2, 2);
	hex = ground_getHexatCoord (i, 3, 2, 0);
	hex_setSlope (hex, HEX_TOP, 2, 2, 2);
	hex = ground_getHexatCoord (i, 3, 3, 0);
	hex_setSlope (hex, HEX_TOP, 2, 2, 2);
	hex = ground_getHexatCoord (i, 3, 4, 0);
	hex_setSlope (hex, HEX_TOP, 2, 2, 2);
	hex = ground_getHexatCoord (i, 3, 5, 0);
	hex_setSlope (hex, HEX_TOP, 2, 2, 2);

	ground_link (groundG, groundH, 0);
	ground_link (groundG, groundH, 2);
	ground_link (groundG, groundH, 4);
	ground_link (groundH, groundI, 2);
	ground_link (groundH, groundI, 4);
	ground_link (groundH, groundI, 0);
	ground_link (groundI, groundG, 0);
	ground_link (groundI, groundG, 2);
	ground_link (groundI, groundG, 4);

/*
    h = ground_getHexatCoord (g, 1, 1, 0);
    hex_setSlope (h, HEX_TOP, 2, 2, 2);
    h = ground_getHexatCoord (g, 1, 4, 0);
    hex_setSlope (h, HEX_TOP, 2, 2, 2);
    h = ground_getHexatCoord (g, 3, 4, 0);
    hex_setSlope (h, HEX_TOP, 2, 2, 2);

    h = ground_getHexatCoord (g, 5, 1, 0);
    hex_setSlope (h, HEX_TOP, 2, 3, 3);
    h = ground_getHexatCoord (g, 5, 3, 0);
    hex_setSlope (h, HEX_TOP, 3, 2, 3);
    h = ground_getHexatCoord (g, 5, 5, 0);
    hex_setSlope (h, HEX_TOP, 3, 3, 2);
    h = NULL;
*/
/*
    ground_link (ground, ground, 0, 4);
    ground_link (ground, ground, 3, 2);
    ground_link (ground, ground, 4, 4);
*/
	ground_bakeTiles (groundG);
	ground_bakeTiles (groundH);
	ground_bakeTiles (groundI);

	w->groundOrigin = groundG;
	w->groundDistanceDraw = 6;

	plant = entity_create ();
	component_instantiateOnEntity ("position", plant);
	plant_generateRandom (plant);
	ground_placeOnTile (groundG, 0, 0, 0, plant);

	camera = entity_create ();
	if (component_instantiateOnEntity ("position", camera))
	{
		ground_placeOnTile (groundG, 3, 0, 0, camera);
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

  return w;
}

void world_destroy (WORLD * w) {
  xph_free (w);
}

void world_update () {
  // components are stored when first registered by the system object. you can
  // probably change this around at runtime, but it might not be a great idea.
  entitySubsystem_runOnStored (OM_UPDATE);
}

void world_postupdate () {
  entitySubsystem_runOnStored (OM_POSTUPDATE);
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

      w = world_create ();
      obj_addClassData (o, "world", w);
      WorldObject = o;

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
