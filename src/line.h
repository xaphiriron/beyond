/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#ifndef LINE_H
#define LINE_H

#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include "vector.h"

enum line_ttype {
  LINE_ENDPOINT,
  LINE_INTERSECTION
};

struct tval {
  float t;
  enum line_ttype type;
  struct line * l;
};

typedef struct line {
  float
    x0, y0,
    f, g;
  struct tval
    ** orderedTvals,
    ** recentTvals;
  int
    tvalSpaceAllocated,
    tvalCount;
} LINE;

enum line_ctype {
  LINE_DONOTSET,
  LINE_SETINTERSECTION,
  LINE_SETENDPOINTS
};

/* anywhere a line_ctype is required, setting it to _SETINTERSECTION or _SETENDPOINTS will alter the argument lines by adding new tvals.
 */

struct tval * create_tval (float t, enum line_ttype type, struct line * l);
void destroy_tval (struct tval * t);

int tval_sort (const void * a, const void * b);
int tval_search (const void * key, const void * datum);

struct line * line_create (float x0, float y0, float f, float g);
struct line * line_createThroughPoints (const struct vector * a, const struct vector * b, enum line_ctype tval);
void line_destroy (struct line * l);

// returns the t value of the intersection on line a. if line_ctype is set, then it adds tvals on both lines.
float line_findIntersection (struct line * a, struct line * b, enum line_ctype);

bool line_coordsAtT (const struct line * l, float t, float * x, float * y);
float line_tNearestCoords (const LINE * l, float x, float y);

void line_normalize (struct line * l);
bool line_resize (LINE * l, float t0, float t1);

int line_countTvals (const struct line * l);
// equal to line_nthRecentTval(l, 0). does this even need to exist?
struct tval * line_mostRecentTval (const struct line * l);
// i is 0..line_countTvals()-1 inclusive
struct tval * line_nthRecentTval (const struct line * l, int i);
struct tval * line_nthOrderedTval (const struct line * l, int i);


struct tval * line_addTval (struct line * l, float t, enum line_ttype, void * intersection);

#endif /* LINE_H */