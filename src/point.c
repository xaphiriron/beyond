#include "point.h"

bool point_addNoDup (POINTS v, float x, float y) {
  int i = 0;
  bool dup = FALSE;
  VECTOR3
    * t = NULL,
    p;
  //printf ("IN %s WHICH I REALLY HOPE DOESN'T CRASH\n", __FUNCTION__);
  p.z = 0;
  p.x = x;
  p.y = y;
  while ((vector_at (t, (Vector *)v, i++)) != NULL) {
    if (pointcmp (t, &p)) {
      dup = TRUE;
      break;
    }
  }
  if (dup == FALSE) {
    t = xph_alloc (sizeof (VECTOR3));
    *t = p;
    vector_push_back ((Vector *)v, t);
    //printf ("%s: DONE; POINT ADDED\n", __FUNCTION__);
    return TRUE;
  }
  //printf ("%s: DONE; DUPLICATE NOT ADDED\n", __FUNCTION__);
  return FALSE;
}

bool point_findMinMax (const POINTS v, float * xminp, float * xmaxp, float * yminp, float * ymaxp) {
  //printf ("WE'RE IN %s\n", __FUNCTION__);
  float
    xmin = 0,
    ymin = 0,
    xmax = 0,
    ymax = 0;
  int i = 0;
  VECTOR3 * p = NULL;
  if (vector_size ((Vector *)v) < 1) {
    return FALSE;
  }
  vector_at (p, (Vector *)v, i);
  xmin = xmax = p->x;
  ymin = ymax = p->y;
  while ((vector_at (p, (Vector *)v, i++)) != NULL) {
    //printf ("POINT #%d: %f, %f\n", i, p->x, p->y);
    if (p->x <= xmin) {
      xmin = p->x;
    } else if (p->x > xmax) {
      xmax = p->x;
    }
    if (p->y <= ymin) {
      ymin = p->y;
    } else if (p->y > ymax) {
      ymax = p->y;
    }
  }
  if (xminp != NULL) {
    *xminp = xmin;
  }
  if (xmaxp != NULL) {
    *xmaxp = xmax;
  }
  if (yminp != NULL) {
    *yminp = ymin;
  }
  if (ymaxp != NULL) {
    *ymaxp = ymax;
  }
  //printf ("WE'RE DONE WITH %s\n", __FUNCTION__);
  return TRUE;
}

bool pointcmp (const VECTOR3 * a, const VECTOR3 * b) {
  return fcmp (a->x, b->x)
    ? fcmp (a->y, b->y)
    : FALSE;
}


// expecting const VECTOR3 *
bool point_areColinear (int n, ...) {
  const VECTOR3
    * p = NULL,
    * q = NULL;
  VECTOR3 diff = vectorCreate (0, 0, 0);
  float slope = 0;
  va_list points;
  //printf ("%s ()...\n", __FUNCTION__);
  va_start (points, n);
  if (n <= 2) {
    //printf ("...%s () [early]\n", __FUNCTION__);
    return TRUE;
  }
  n -= 2;
  p = va_arg (points, const VECTOR3 *);
  //printf ("  got %p as p\n", p);
  q = va_arg (points, const VECTOR3 *);
  //printf ("  got %p as q\n", q);
  diff = vectorSubtract (p, q);
  slope = diff.x / diff.y;
  while (n-- > 0) {
    q = va_arg (points, const VECTOR3 *);
    //printf ("  rolling: got %p as q\n", q);
    diff = vectorSubtract (p, q);
    if (fcmp (slope, diff.x / diff.y) != TRUE) {
      va_end (points);
      //printf ("...%s () [f]\n", __FUNCTION__);
      return FALSE;
    }
  }
  va_end (points);
  //printf ("...%s () [t]\n", __FUNCTION__);
  return TRUE;
}
