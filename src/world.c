#include "world.h"

Object * WorldObject = NULL;

// static void draw_grid ();

WORLD * world_create () {
  WORLD * w = xph_alloc (sizeof (WORLD), "WORLD");
  HEX * h = NULL;
  Entity e = NULL;
  w->origin = vectorCreate (0.0, 0.0, 0.0);
  w->map = map_create (8);
  vector_assign (w->map->tiles, 0, hex_create (0, 0, 0, 0.0));
  h = hex_create (1, 5, 0, 4);
  hex_slope_from_plane (h, HEX_TOP, .5, 1.0, .5);
  vector_assign (w->map->tiles, 6, h);
  h = hex_create (1, 4, 0, 4);
  hex_slope_from_plane (h, HEX_TOP, .5, .5, .0);
  vector_assign (w->map->tiles, 5, h);
  h = hex_create (1, 3, 0, 4);
  hex_slope_from_plane (h, HEX_TOP, .5, .0, .0);
  vector_assign (w->map->tiles, 4, h);
  h = hex_create (1, 2, 0, 4);
  hex_slope_from_plane (h, HEX_TOP, .5, .0, .5);
  vector_assign (w->map->tiles, 3, h);
  h = hex_create (1, 1, 0, 4);
  hex_slope_from_plane (h, HEX_TOP, .5, .5, 1.0);
  vector_assign (w->map->tiles, 2, h);
  h = hex_create (1, 0, 0, 4);
  hex_slope_from_plane (h, HEX_TOP, .5, 1.0, 1.0);
  vector_assign (w->map->tiles, 1, h);
  h = hex_create (2, 5, 1, 8);
  hex_slope_from_plane (h, HEX_TOP, 1.5, 1.5, 1.0);
  vector_assign (w->map->tiles, 18, h);
  vector_assign (w->map->tiles, 17, hex_create (2, 5, 0, 2.0));
  h = hex_create (2, 4, 1, 8);
  hex_slope_from_plane (h, HEX_TOP, 1.5, 2.0, 2.0);
  vector_assign (w->map->tiles, 16, h);
  vector_assign (w->map->tiles, 15, hex_create (2, 4, 0, 1.0));
  h = hex_create (2, 3, 1, 8);
  hex_slope_from_plane (h, HEX_TOP, 1.5, 1.0, 1.5);
  vector_assign (w->map->tiles, 14, h);
  vector_assign (w->map->tiles, 13, hex_create (2, 3, 0, 2.0));
  h = hex_create (2, 2, 1, 8);
  hex_slope_from_plane (h, HEX_TOP, 1.5, 1.5, 1.0);
  vector_assign (w->map->tiles, 12, h);
  vector_assign (w->map->tiles, 11, hex_create (2, 2, 0, 1.0));
  h = hex_create (2, 1, 1, 8);
  hex_slope_from_plane (h, HEX_TOP, 1.5, 2.0, 2.0);
  vector_assign (w->map->tiles, 10, h);
  vector_assign (w->map->tiles, 9, hex_create (2, 1, 0, 2.0));
  h = hex_create (2, 0, 1, 8);
  hex_slope_from_plane (h, HEX_TOP, 1.5, 1.0, 1.5);
  vector_assign (w->map->tiles, 8, h);
  vector_assign (w->map->tiles, 7, hex_create (2, 0, 0, 1.0));
  vector_assign (w->map->tiles, 36, hex_create (3, 5, 2, 3.0));
  vector_assign (w->map->tiles, 35, hex_create (3, 5, 1, 3.0));
  h = hex_create (3, 5, 0, 12);
  hex_slope_from_plane (h, HEX_TOP, 2.75, 3.0, 2.75);
  vector_assign (w->map->tiles, 34, h);
  vector_assign (w->map->tiles, 33, hex_create (3, 4, 2, 3.0));
  vector_assign (w->map->tiles, 32, hex_create (3, 4, 1, 3.0));
  h = hex_create (3, 4, 0, 12);
  hex_slope_from_plane (h, HEX_TOP, 2.75, 2.75, 2.5);
  vector_assign (w->map->tiles, 31, h);
  vector_assign (w->map->tiles, 30, hex_create (3, 3, 2, 3.0));
  vector_assign (w->map->tiles, 29, hex_create (3, 3, 1, 3.0));
  h = hex_create (3, 3, 0, 12);
  hex_slope_from_plane (h, HEX_TOP, 2.75, 2.5, 2.5);
  vector_assign (w->map->tiles, 28, h);
  vector_assign (w->map->tiles, 27, hex_create (3, 2, 2, 3.0));
  vector_assign (w->map->tiles, 26, hex_create (3, 2, 1, 3.0));
  h = hex_create (3, 2, 0, 12);
  hex_slope_from_plane (h, HEX_TOP, 2.75, 2.5, 2.75);
  vector_assign (w->map->tiles, 25, h);
  vector_assign (w->map->tiles, 24, hex_create (3, 1, 2, 3.0));
  vector_assign (w->map->tiles, 23, hex_create (3, 1, 1, 3.0));
  h = hex_create (3, 1, 0, 12);
  hex_slope_from_plane (h, HEX_TOP, 2.75, 2.75, 3.0);
  vector_assign (w->map->tiles, 22, h);
  vector_assign (w->map->tiles, 21, hex_create (3, 0, 2, 3.0));
  vector_assign (w->map->tiles, 20, hex_create (3, 0, 1, 3.0));
  h = hex_create (3, 0, 0, 12);
  hex_slope_from_plane (h, HEX_TOP, 2.75, 3.0, 3.0);
  vector_assign (w->map->tiles, 19, h);

  vector_assign (w->map->tiles, 37, hex_create (4, 0, 0, 3.0));
  vector_assign (w->map->tiles, 38, hex_create (4, 0, 1, 3.0));
  vector_assign (w->map->tiles, 39, hex_create (4, 0, 2, 4.0));
  vector_assign (w->map->tiles, 40, hex_create (4, 0, 3, 3.0));
  vector_assign (w->map->tiles, 41, hex_create (4, 1, 0, 3.0));
  vector_assign (w->map->tiles, 42, hex_create (4, 1, 1, 3.0));
  vector_assign (w->map->tiles, 43, hex_create (4, 1, 2, 4.0));
  vector_assign (w->map->tiles, 44, hex_create (4, 1, 3, 3.0));
  vector_assign (w->map->tiles, 45, hex_create (4, 2, 0, 3.0));
  vector_assign (w->map->tiles, 46, hex_create (4, 2, 1, 3.0));
  vector_assign (w->map->tiles, 47, hex_create (4, 2, 2, 4.0));
  vector_assign (w->map->tiles, 48, hex_create (4, 2, 3, 3.0));
  vector_assign (w->map->tiles, 49, hex_create (4, 3, 0, 3.0));
  vector_assign (w->map->tiles, 50, hex_create (4, 3, 1, 3.0));
  vector_assign (w->map->tiles, 51, hex_create (4, 3, 2, 4.0));
  vector_assign (w->map->tiles, 52, hex_create (4, 3, 3, 3.0));
  vector_assign (w->map->tiles, 53, hex_create (4, 4, 0, 3.0));
  vector_assign (w->map->tiles, 54, hex_create (4, 4, 1, 3.0));
  vector_assign (w->map->tiles, 55, hex_create (4, 4, 2, 4.0));
  vector_assign (w->map->tiles, 56, hex_create (4, 4, 3, 3.0));
  vector_assign (w->map->tiles, 57, hex_create (4, 5, 0, 3.0));
  vector_assign (w->map->tiles, 58, hex_create (4, 5, 1, 3.0));
  vector_assign (w->map->tiles, 59, hex_create (4, 5, 2, 4.0));
  vector_assign (w->map->tiles, 60, hex_create (4, 5, 3, 3.0));

  vector_assign (w->map->tiles, 61, hex_create (5, 0, 0, 3.0));
  vector_assign (w->map->tiles, 62, hex_create (5, 0, 1, 3.0));
  vector_assign (w->map->tiles, 63, hex_create (5, 0, 2, 3.0));
  vector_assign (w->map->tiles, 64, hex_create (5, 0, 3, 3.0));
  vector_assign (w->map->tiles, 65, hex_create (5, 0, 4, 3.0));
  vector_assign (w->map->tiles, 66, hex_create (5, 1, 0, 3.0));
  vector_assign (w->map->tiles, 67, hex_create (5, 1, 1, 3.0));
  vector_assign (w->map->tiles, 68, hex_create (5, 1, 2, 3.0));
  vector_assign (w->map->tiles, 69, hex_create (5, 1, 3, 3.0));
  vector_assign (w->map->tiles, 70, hex_create (5, 1, 4, 3.0));
  vector_assign (w->map->tiles, 71, hex_create (5, 2, 0, 3.0));
  vector_assign (w->map->tiles, 72, hex_create (5, 2, 1, 3.0));
  vector_assign (w->map->tiles, 73, hex_create (5, 2, 2, 3.0));
  vector_assign (w->map->tiles, 74, hex_create (5, 2, 3, 3.0));
  vector_assign (w->map->tiles, 75, hex_create (5, 2, 4, 3.0));
  vector_assign (w->map->tiles, 76, hex_create (5, 3, 0, 3.0));
  vector_assign (w->map->tiles, 77, hex_create (5, 3, 1, 3.0));
  vector_assign (w->map->tiles, 78, hex_create (5, 3, 2, 3.0));
  vector_assign (w->map->tiles, 79, hex_create (5, 3, 3, 3.0));
  vector_assign (w->map->tiles, 80, hex_create (5, 3, 4, 3.0));
  vector_assign (w->map->tiles, 81, hex_create (5, 4, 0, 3.0));
  vector_assign (w->map->tiles, 82, hex_create (5, 4, 1, 3.0));
  vector_assign (w->map->tiles, 83, hex_create (5, 4, 2, 3.0));
  vector_assign (w->map->tiles, 84, hex_create (5, 4, 3, 3.0));
  vector_assign (w->map->tiles, 85, hex_create (5, 4, 4, 3.0));
  vector_assign (w->map->tiles, 86, hex_create (5, 5, 0, 3.0));
  vector_assign (w->map->tiles, 87, hex_create (5, 5, 1, 3.0));
  vector_assign (w->map->tiles, 88, hex_create (5, 5, 2, 3.0));
  vector_assign (w->map->tiles, 89, hex_create (5, 5, 3, 3.0));
  vector_assign (w->map->tiles, 90, hex_create (5, 5, 4, 3.0));

  h = hex_create (6, 0, 0, 16);
  hex_slope_from_plane (h, HEX_TOP, 3.75, 4.25, 4.25);
  vector_assign (w->map->tiles, 91, h);
  h = hex_create (6, 0, 1, 16);
  hex_slope_from_plane (h, HEX_TOP, 3.5, 3.75, 4.0);
  vector_assign (w->map->tiles, 92, h);
  h = hex_create (6, 0, 2, 16);
  hex_slope_from_plane (h, HEX_TOP, 3.5, 3.75, 4.0);
  vector_assign (w->map->tiles, 93, h);
  h = hex_create (6, 0, 3, 16);
  hex_slope_from_plane (h, HEX_TOP, 3.5, 3.75, 4.0);
  vector_assign (w->map->tiles, 94, h);
  h = hex_create (6, 0, 4, 16);
  hex_slope_from_plane (h, HEX_TOP, 3.5, 3.75, 4.0);
  vector_assign (w->map->tiles, 95, h);
  h = hex_create (6, 0, 5, 16);
  hex_slope_from_plane (h, HEX_TOP, 3.5, 3.75, 4.0);
  vector_assign (w->map->tiles, 96, h);
  h = hex_create (6, 1, 0, 16);
  hex_slope_from_plane (h, HEX_TOP, 3.75, 3.75, 4.25);
  vector_assign (w->map->tiles, 97, h);
  h = hex_create (6, 1, 1, 16);
  hex_slope_from_plane (h, HEX_TOP, 3.5, 3.25, 3.75);
  vector_assign (w->map->tiles, 98, h);
  h = hex_create (6, 1, 2, 16);
  hex_slope_from_plane (h, HEX_TOP, 3.5, 3.25, 3.75);
  vector_assign (w->map->tiles, 99, h);
  h = hex_create (6, 1, 3, 16);
  hex_slope_from_plane (h, HEX_TOP, 3.5, 3.25, 3.75);
  vector_assign (w->map->tiles, 100, h);
  h = hex_create (6, 1, 4, 16);
  hex_slope_from_plane (h, HEX_TOP, 3.5, 3.25, 3.75);
  vector_assign (w->map->tiles, 101, h);
  h = hex_create (6, 1, 5, 16);
  hex_slope_from_plane (h, HEX_TOP, 3.5, 3.25, 3.75);
  vector_assign (w->map->tiles, 102, h);
  h = hex_create (6, 2, 0, 16);
  hex_slope_from_plane (h, HEX_TOP, 3.75, 3.25, 3.75);
  vector_assign (w->map->tiles, 103, h);
  h = hex_create (6, 2, 1, 16);
  hex_slope_from_plane (h, HEX_TOP, 3.5, 3.0, 3.25);
  vector_assign (w->map->tiles, 104, h);
  h = hex_create (6, 2, 2, 16);
  hex_slope_from_plane (h, HEX_TOP, 3.5, 3.0, 3.25);
  vector_assign (w->map->tiles, 105, h);
  h = hex_create (6, 2, 3, 16);
  hex_slope_from_plane (h, HEX_TOP, 3.5, 3.0, 3.25);
  vector_assign (w->map->tiles, 106, h);
  h = hex_create (6, 2, 4, 16);
  hex_slope_from_plane (h, HEX_TOP, 3.5, 3.0, 3.25);
  vector_assign (w->map->tiles, 107, h);
  h = hex_create (6, 2, 5, 16);
  hex_slope_from_plane (h, HEX_TOP, 3.5, 3.0, 3.25);
  vector_assign (w->map->tiles, 108, h);

  e = entity_create ();
  if (component_instantiateOnEntity ("position", e)) {
    setPosition (e, vectorCreate (0.0, -60.0, 0.0));
  }
  component_instantiateOnEntity ("integrate", e);
  component_instantiateOnEntity ("camera", e);
  if (component_instantiateOnEntity ("collide", e)) {
    setCollideType (e, COLLIDE_SPHERE);
    setCollideRadius (e, 30.0);
  }
  if (component_instantiateOnEntity ("input", e)) {
    input_addControlledEntity (e);
  }
  component_instantiateOnEntity ("walking", e);
  w->camera = e;

  return w;
}

void world_destroy (WORLD * w) {
//   camera_destroy (w->c);
  map_destroy (w->map);
  xph_free (w);
}

void world_update () {
  // components are stored when components are first registered by the system
  // object. you can probably change this around at runtime, but it might not
  // be the best idea.
  entitySubsystem_runOnStored (OM_UPDATE);
/*
  entitySubsystem_update ("position");
  entitySubsystem_update ("walking");
  entitySubsystem_update ("integrate");
  entitySubsystem_update ("camera");
  entitySubsystem_update ("collide");
  entitySubsystem_update ("input");
*/
}

void world_postupdate () {
  entitySubsystem_runOnStored (OM_POSTUPDATE);
/*
  entitySubsystem_postupdate ("position");
  entitySubsystem_postupdate ("walking");
  entitySubsystem_postupdate ("integrate");
  entitySubsystem_postupdate ("camera");
  entitySubsystem_postupdate ("collide");
  entitySubsystem_postupdate ("input");
*/
}

int world_handler (Object * o, objMsg msg, void * a, void * b) {
  WORLD * w = NULL;
  Component c = NULL;
  struct camera_data * cdata = NULL;
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
      c = entity_getAs (w->camera, "camera");
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glPushMatrix ();
      if (c == NULL) {
        glLoadIdentity ();
      } else {
        cdata = component_getData (c);
        glLoadMatrixf (cdata->m);
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
      map_draw (w->map);
      return EXIT_SUCCESS;

    case OM_POSTRENDER:
      // called after this:render. do we really need this?
      glPopMatrix ();
      return EXIT_SUCCESS;

    default:
      return obj_pass ();
  }
  return EXIT_FAILURE;
}

/*
static void draw_grid () {
  int
    x = 0,
    y = 0;
  int
    d = 150 + 1,
    s = 100,
    c = (d / 2.0) * s;
  glColor3f (0.8, 0.8, 0.1);
  while(x < d) {
    y = 0;
    glBegin(GL_QUAD_STRIP);
    glVertex3f (c - x * s, 0, c - y * s);
    glVertex3f (c - (x + 1) * s, 0, c - y * s);
    while(y < d) {
      glVertex3f (c - x * s, 0, c - (y + 1) * s);
      glVertex3f (c - (x + 1) * s, 0, c - (y + 1) * s);
      y++;
    }
    glEnd();
    x++;
  }
}
*/