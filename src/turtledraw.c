#include "turtledraw.h"

extern Object * VideoObject;

int turtle_drawLines (TURTLE * t) {
  const TLINES lines = turtle_getLines (t);
  const LINE * l = NULL;
  const VIDEO * video = obj_getClassData (VideoObject, "video");
  int i = 0;
  float
    x0 = 0, y0 = 0,
    x1 = 0, y1 = 0,
    scale = turtle_getScale (t);
  const VECTOR3 * center = turtle_getCenter (t);
  //printf ("WOOOO TURTLE_DRAWLINES!!!!\n");
  //printf ("%d lines\n", tline_count (lines));
  //printf ("scale: %.2f; center: %.2f,%.2f\n", scale, center->x, center->y);

  glMatrixMode (GL_MODELVIEW);
  glPushMatrix ();
  glLoadIdentity ();

  glBegin (GL_LINES);
  while (vector_at (l, *lines, i++) != NULL) {
    //printf ("LINE %d of %d/%d: %p\n", i, listItemCount (*(const struct list **)lines), tline_count (lines), l);
    line_coordsAtT (l, 0, &x0, &y0);
    line_coordsAtT (l, 1, &x1, &y1);
    //printf ("past line_coords...\n");
    //printf ("was %+8.2f,%+8.2f to %+8.2f,%+8.2f\n", x0, y0, x1, y1);
    x0 = (x0 - center->x) * scale;
    y0 = (y0 - center->y) * scale;
    x1 = (x1 - center->x) * scale;
    y1 = (y1 - center->y) * scale;
    //printf ("past scale and center calcs\n");
    //printf ("now %+8.2f,%+8.2f to %+8.2f,%+8.2f\n", x0, y0, x1, y1);

    glColor3f (0.3, 1.0, 0.3);
    glVertex3f (x0, y0, -video->near * 1.00001);
    glVertex3f (x1, y1, -video->near * 1.00001);
    //printf ("end of loop\n");

  }
  glEnd ();
  glPopMatrix ();

  //printf ("FINISHED W/ TURTLE_DRAWLINES\n");
  return i - 1;
}