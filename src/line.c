#include "line.h"

/*
extern void free (void *);
extern void * realloc (void *, size_t);
extern void qsort (void *, size_t, size_t,  int (*)(const void *, const void *));
*/

struct tval * create_tval (float t, enum line_ttype type, struct line * l) {
  struct tval * v = malloc (sizeof (struct tval));
  v->t = t;
  v->type = type;
  v->l = l;
  return v;
}

void destroy_tval (struct tval * t) {
  free (t);
}

int tval_sort (const void * a, const void * b) {
  return (*(struct tval **)a)->t - (*(struct tval **)b)->t < 0
   ? -1
   : (*(struct tval **)a)->t - (*(struct tval **)b)->t > 0
     ? 1
     : 0;
}

int tval_search (const void * key, const void * datum) {
  return ((struct tval *)key)->t - (*(struct tval **)datum)->t < 0
   ? -1
   : ((struct tval *)key)->t - (*(struct tval **)datum)->t > 0
     ? 1
     : 0;
}

struct line * line_create (float x0, float y0, float f, float g) {
  struct line * l = malloc (sizeof (struct line));
  l->x0 = x0;
  l->y0 = y0;
  l->f = f;
  l->g = g;
  l->tvalSpaceAllocated = 4;
  l->tvalCount = 0;
  l->orderedTvals = malloc (sizeof (struct tval *) * l->tvalSpaceAllocated);
  l->recentTvals = malloc (sizeof (struct tval *) * l->tvalSpaceAllocated);
  return l;
}

struct line * line_createThroughPoints (const struct vector * a, const struct vector * b, enum line_ctype tval) {
  LINE * l = line_create (a->x, a->y, b->x - a->x, b->y - a->y);
  // because of this line everything went screwy a billion levels up and
  // nothing made sense. way to wire things backwards, self.
  //struct line * l = line_create (b->x - a->x, b->y - a->y, a->x, a->y);
  if (tval != LINE_DONOTSET) {
    line_addTval (l, 0, tval == LINE_SETINTERSECTION ? LINE_INTERSECTION : LINE_ENDPOINT, NULL);
    line_addTval (l, 1, tval == LINE_SETINTERSECTION ? LINE_INTERSECTION : LINE_ENDPOINT, NULL);
  }
  return l;
}

void line_destroy (struct line * l) {
  while (l->tvalCount > 0) {
    destroy_tval (l->orderedTvals[--l->tvalCount]);
  }
  free (l->orderedTvals);
  free (l->recentTvals);
  free (l);
}

float line_findIntersection (struct line * a, struct line * b, enum line_ctype tval) {
  float ret = __builtin_nan("");
  float
    afbg = a->f * b->g,
    bfag = b->f * a->g,
    det = bfag - afbg,
    bay, bax, dinv;
  if (fabs (det) < FLT_EPSILON) {
    return ret;
  }
  bay = b->y0 - a->y0;
  bax = b->x0 - a->x0;
  dinv = 1.0 / det;
  ret = (b->f * bay - b->g * bax) * dinv;
  if (tval != LINE_DONOTSET) {
    line_addTval (a, ret, tval == LINE_SETINTERSECTION ? LINE_INTERSECTION : LINE_ENDPOINT, tval == LINE_SETINTERSECTION ? b : NULL);
    line_addTval (b, (a->f * bay - a->g * bax) * dinv, tval == LINE_SETINTERSECTION ? LINE_INTERSECTION : LINE_ENDPOINT, tval == LINE_SETINTERSECTION ? a : NULL);
  }
  /* for the record, we can get the x,y position of the intersection with
   * x = a->x0 + a->f * ret;
   * y = a->y0 + a->g * ret;
   */
  return ret;
}

bool line_coordsAtT (const struct line * l, float t, float * x, float * y) {
  if (l == NULL)
    return false;
  if (x != NULL)
    *x = l->x0 + l->f * t;
  if (y != NULL)
    *y = l->y0 + l->g * t;
  return true;
}

// we do the "distance of a point from a line" calculation and expect it to be right on the line, or close enough that we don't care.
float line_tNearestCoords (const LINE * l, float jx, float jy) {
  float
    fsq = l->f * l->f,
    gsq = l->g * l->g,
    fgsq = fsq + gsq,
    tj = 0;
  if (fcmp (fgsq, 0.0) == true) {
    // line is broken
    return 0;
  }
  tj = (l->f * (jx - l->x0) + l->g * (jy - l->y0)) * (1.0 / fgsq);
  return tj;
/* this is how you get distance from the line (so says "A programmer's
 * geometry", at least):
    xj0 = jx - l->x0,
    yj0 = jy - l->y0,
    finv = 1.0 / fgsq,

    dx = gsq * xj0 - fg * yj0,
    dy = fsq * yj0 - fg * xj0,
    rsq = (dx * dx + dy * dy) * finv * finv;
*/
}

/* I don't actually remember if this is the proper normalization math.
 */
void line_normalize (struct line * l) {
  int i = 0;
  float mag = l->f * l->f + l->g * l->g;
  l->f /= mag;
  l->g /= mag;
  while (i < l->tvalCount) {
    l->orderedTvals[i++]->t /= mag;
  }
}

bool line_resize (LINE * l, float t0, float t1) {
  VECTOR3
    a = vectorCreate (0, 0, 0),
    b = vectorCreate (0, 0, 0);
  if (fcmp (t0, t1) == true) {
    return false;
  }
  line_coordsAtT (l, t0, &a.x, &a.y);
  line_coordsAtT (l, t1, &b.x, &b.y);
  l->x0 = a.x;
  l->y0 = a.y;
  l->f = b.x - a.x;
  l->g = b.y - a.y;
  return true;
}

int line_countTvals (const struct line * l) {
  return l->tvalCount;
}

struct tval * line_mostRecentTval (const struct line * l) {
  return line_nthRecentTval (l, 0);
}

struct tval * line_nthRecentTval (const struct line * l, int i) {
  if (i < 0 || i >= line_countTvals (l))
    return NULL;
  return l->recentTvals[line_countTvals (l) - i - 1];
}

struct tval * line_nthOrderedTval (const struct line * l, int i) {
  if (i < 0 || i >= line_countTvals (l))
    return NULL;
  return l->orderedTvals[i];
}

struct tval * line_addTval (struct line * l, float t, enum line_ttype type, void * intersection) {
  struct tval ** newList = NULL;
  struct tval * new = create_tval (t, type, intersection);
  if (l->tvalCount == l->tvalSpaceAllocated) {
    l->tvalSpaceAllocated *= 2;
    newList = realloc (l->orderedTvals, sizeof (struct tval *) * l->tvalSpaceAllocated);
    // does realloc free the old memory? Is this unnecessary?
    free (l->orderedTvals);
    l->orderedTvals = newList;
    newList = realloc (l->recentTvals, sizeof (struct tval *) * l->tvalSpaceAllocated);
    free (l->recentTvals);
    l->recentTvals = newList;
    newList = NULL;
  }
  l->recentTvals[l->tvalCount] = new;
  l->orderedTvals[l->tvalCount++] = new;
  qsort (l->orderedTvals, l->tvalCount, sizeof (struct tval *), tval_sort);
  return new;
}

void line_rmTval (struct line * l, float t) {
/*
  int i = 0;
  while (i < l->toffset) {
    if (l->tvals[i]->t == t) {
      line_rmTvalByOffset (l, i);
      break;
    }
    i++;
  }
*/
}

void line_rmTvalByOffset (struct line * l, int i) {
/*
  if (l->toffset <= i) {
    return;
  }
  destroy_tval (l->tvals[i]);
  l->tvals[i] = l->tvals[--l->toffset];
  l->tvals[l->toffset] = NULL;
  qsort (l->tvals, l->toffset, sizeof (struct tval *), tval_sort);
*/
}