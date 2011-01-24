#include "turtle.h"

static TURTLE tDefault;
static bool tInit = FALSE;

static void turtle_recalcHeading (TURTLE * t);
static void tDefaults ();

static void turtle_createPoints (TURTLE * t);
static void turtle_recalcScaleCenter (TURTLE * t, float xRes, float yRes);

extern float video_getXResolution ();
extern float video_getYResolution ();
static float FAKEXRESOLUTION ();
static float FAKEYRESOLUTION ();
static float (*screen_x_func)() = video_getXResolution;
static float (*screen_y_func)() = video_getYResolution;
static float fake_x_resolution = 720.0;
static float fake_y_resolution = 540.0;

TURTLE * turtle_create () {
  if (tInit == FALSE) {
    tInit = TRUE;
    tDefaults ();
  }
  return turtle_createA (
    tDefault.position.x,
    tDefault.position.y,
    tDefault.position.z,
    tDefault.heading);
}

TURTLE * turtle_createA (float x, float y, float z, float rot) {
  TURTLE * t = xph_alloc (sizeof (TURTLE));
  t->heading = rot;
  t->position = vectorCreate (x, y, z);
  t->penDown = tDefault.penDown;
  t->locationStack = dynarr_create (4, sizeof (struct tloc *));
  t->lines = dynarr_create (4, sizeof (LINE *));
  t->points = dynarr_create (4, sizeof (VECTOR3 *));

  t->scale = 0;
  t->center = vectorCreate (0, 0, 0);
  t->scaleCenterClean = FALSE;
  t->lastXResolution = t->lastYResolution = 0;

  turtle_recalcHeading (t);
  return t;
}

static void tDefaults () {
  tDefault.heading = 0.0;
  tDefault.position = vectorCreate (0.0, 0.0, 0.0);
  tDefault.penDown = TURTLE_PENDOWN;
}

void turtle_destroy (TURTLE * t) {
  while (!dynarr_isEmpty (t->locationStack)) {
    xph_free (*(struct tloc **)dynarr_pop (t->locationStack));
  }
  dynarr_destroy (t->locationStack);
  while (!dynarr_isEmpty (t->points)) {
	// i have no clue what is stored in this. i think it stores more dynamic arrays and this line was originally copied from the one above and not updated. so, memory leak here.
    xph_free (*(struct tloc **)dynarr_pop (t->points));
  }
  dynarr_destroy (t->points);
  turtle_clearLines (t);
  dynarr_destroy (t->lines);
  xph_free (t);
}

float turtle_getHeading (const TURTLE * t) {
  return t->heading;
}

const struct vector * turtle_getPosition (const TURTLE * t) {
  return &t->position;
}

enum turtle_pen turtle_getPenStatus (const TURTLE * t) {
  return t->penDown;
}

static void turtle_recalcHeading (TURTLE * t) {
  float tr = t->heading / 180.0 * M_PI;
  t->headingVector = vectorCreate (sin (tr), cos (tr) , 0.0);
}

void turtle_rotate (TURTLE * t, float deg) {
  t->heading += deg;
  while (t->heading < -180.0) {
    t->heading += 360.0;
  }
  while (t->heading > 180.0) {
    t->heading -= 360.0;
  }
  turtle_recalcHeading (t);
}

void turtle_move (TURTLE * t, float mag) {
  VECTOR3
    v = vectorMultiplyByScalar (&t->headingVector, mag),
    n = vectorAdd (&t->position, &v);
  VECTOR3
    j = vectorCreate (0, 0, 0),
    k = vectorCreate (0, 0, 0);
  LINE * l = NULL;
  //printf ("TURTLE MOVED FROM %.2f,%.2f TO %.2f,%.2f ON A REQUEST TO MOVE %f FORWARD\n", t->position.x, t->position.y, n.x, n.y, mag);
  if (turtle_getPenStatus (t) == TURTLE_PENDOWN) {
    if (!dynarr_isEmpty (t->lines)) {
      l = *(LINE **)dynarr_back (t->lines);
      //printf ("got line %p\n", l);
      line_coordsAtT (l, 0, &j.x, &j.y);
      line_coordsAtT (l, 1, &k.x, &k.y);
      // if the turtle's previous line is AB, and its new line is BC, and ABC are all colinear points, then we can replace AB with AC with absolutely no loss of data. This is a massive help for a lot of l-systems with an 'X' => "XX" rule, since after a few interations each long line is actually 64 really short lines. This code won't catch all opportunities for line merging because of things like stack pushes and pops, but hopefully it will get enough to get some sort of performance boost -- and in fact in the first test it took the line count down from 15872 lines to 7504 lines.
      if (
        vector_cmp (&k, &t->position) == TRUE &&
        point_areColinear (3, &j, &t->position, &n)) {
        //printf ("woo resizing line\n");
        line_resize (l, 0, line_tNearestCoords (l, n.x, n.y));
      } else {
        dynarr_push (t->lines, line_createThroughPoints (&t->position, &n, LINE_SETENDPOINTS));
      }
    } else {
      l = line_createThroughPoints (&t->position, &n, LINE_SETENDPOINTS);
      //printf ("LINE DRAWN FROM %.2f,%.2f TO %.2f,%.2f (%.2f,%.2f)\n", l->x0, l->y0, l->x0 + l->f, l->y0 + l->g, l->f, l->g);
      dynarr_push (t->lines, l);
    }
    t->scaleCenterClean = FALSE;
  }
  //printf ("DONE W/ %s\n", __FUNCTION__);
  t->position = n;
}

void turtle_penUp (TURTLE * t) {
  t->penDown = TURTLE_PENUP;
}

void turtle_penDown (TURTLE * t) {
  t->penDown = TURTLE_PENDOWN;
}

void turtle_push (TURTLE * t) {
  struct tloc * tloc = xph_alloc (sizeof (struct tloc));
  tloc->heading = t->heading;
  tloc->position = t->position;
  dynarr_push (t->locationStack, tloc);
}

void turtle_pop (TURTLE * t) {
  struct tloc * tloc = NULL;
  tloc = *(struct tloc **)dynarr_pop (t->locationStack);
  if (tloc == NULL) {
    return;
  }
  t->position = tloc->position;
  t->heading = tloc->heading;
  xph_free (tloc);
  turtle_recalcHeading (t);
}

const TLINES turtle_getLines (const TURTLE * t) {
  return (const TLINES)&t->lines;
}

const POINTS turtle_getPoints (TURTLE * t) {
  if (t->scaleCenterClean == FALSE) {
    turtle_createPoints (t);
  }
  return t->points;
}

void turtle_SETFAKERESOLUTION (float x, float y) {
  if (x <= 0)
    x = 1;
  if (y <= 0)
    y = 1;
  fake_x_resolution = x;
  fake_y_resolution = y;
}

static float FAKEXRESOLUTION () {
  return fake_x_resolution;
}

static float FAKEYRESOLUTION () {
  return fake_y_resolution;
}


void turtle_FAKERESOLUTION (bool enable) {
  if (enable) {
    //printf ("%s: TURNING FAKE RESOLUTION ON; SHOULD BE USING FAKE FUNCTIONS\n", __FUNCTION__);
    screen_x_func = FAKEXRESOLUTION;
    screen_y_func = FAKEYRESOLUTION;
  } else {
    //printf ("%s: TURNING FAKE RESOLUTION OFF; SHOULD BE USING REAL FUNCTIONS\n", __FUNCTION__);
    screen_x_func = video_getXResolution;
    screen_y_func = video_getYResolution;
  }
}

float turtle_getScale (TURTLE * t) {
  float
    xRes = screen_x_func (),
    yRes = screen_y_func ();
  //printf ("%s: the wonky function pointers didn't segfault!\n", __FUNCTION__);
  if (
    t->scaleCenterClean == FALSE
      || fcmp (t->lastXResolution, xRes) == FALSE
      || fcmp (t->lastYResolution, yRes) == FALSE) {
    //printf ("RECALCULATING SCALE & CENTER FROM %s\n", __FUNCTION__);
    turtle_recalcScaleCenter (t, xRes, yRes);
  }
  //printf ("lol turtle_recalcScaleCenter didn't crash! LOL LIES\n");
  return t->scale;
}

const VECTOR3 * turtle_getCenter (TURTLE * t) {
  float
    xRes = screen_x_func (),
    yRes = screen_y_func ();
  //printf ("%s: the wonky function pointers didn't segfault!\n", __FUNCTION__);
  if (
    t->scaleCenterClean == FALSE
      || fcmp (t->lastXResolution, xRes) == FALSE
      || fcmp (t->lastYResolution, yRes) == FALSE) {
    //printf ("RECALCULATING SCALE & CENTER FROM %s\n", __FUNCTION__);
    turtle_recalcScaleCenter (t, xRes, yRes);
  }
  //printf ("lol turtle_recalcScaleCenter didn't crash! LOL LIES\n");
  return &t->center;
}

void turtle_clearLines (TURTLE * t) {
	LINE
		* l;
  //printf ("%s (%p)...\n", __FUNCTION__, t);
  while (!dynarr_isEmpty (t->lines)) {
    l = *(struct line **)dynarr_pop (t->lines);
    line_destroy (l);
  }
  while (!dynarr_isEmpty (t->points)) {
    xph_free (*(char **)dynarr_pop (t->points));
  }
  t->scaleCenterClean = FALSE;
  //printf ("...%s\n", __FUNCTION__);
}

void turtle_resetPosition (TURTLE * t) {
  t->position = tDefault.position;
  t->heading = tDefault.heading;
  turtle_recalcHeading (t);
}

static void turtle_recalcScaleCenter (TURTLE * t, float xResolution, float yResolution) {
  //printf ("WE'RE IN %s\n", __FUNCTION__);
  const POINTS points = turtle_getPoints (t);
  float
    xmin = 0, xmax = 0,
    ymin = 0, ymax = 0,
    xScale = 0, yScale = 0;
  //printf ("GOT POINTS WITHOUT CRASHING\n");
  if (!point_findMinMax (points, &xmin, &xmax, &ymin, &ymax)) {
    t->scale = 1;
    t->center = vectorCreate (0.0, 0.0, 0.0);
    //printf ("WE'RE DONE WITH %s, AND EARLY TOO\n", __FUNCTION__);
    return;
  }
  //printf ("FOUND REAL MIN/MAX VALUES WITHOUT CRASHING!\n");
  //printf ("xResolution: %f, yResolution: %f\n", xResolution, yResolution);
  //printf ("line x dimensions: %f; line y dimensions: %f\n", xmax - xmin, ymax - ymin);
  xScale = xResolution / (xmax - xmin);
  yScale = yResolution / (ymax - ymin);
  t->lastXResolution = xResolution;
  t->lastYResolution = yResolution;
  t->scale = xScale < yScale
    ? xScale
    : yScale;
  //printf ("XMIN: %.2f; XMAX: %.2f; YMIN: %.2f; YMAX: %.2f\n", xmin, xmax, ymin, ymax);
  t->center = vectorCreate (
    xmin + (xmax - xmin) / 2.0,
    ymin + (ymax - ymin) / 2.0,
    0
  );
  //printf ("scale: %.2f; center: %.2f,%.2f\n", t->scale, t->center.x, t->center.y);
  t->scaleCenterClean = TRUE;
  //printf ("WE'RE DONE WITH %s\n", __FUNCTION__);
}

static void turtle_createPoints (TURTLE * t) {
  //printf ("WE'RE IN %s\n", __FUNCTION__);
  LINE * line = NULL;
  POINTS points = dynarr_create (4, sizeof (VECTOR3 *));
  int i = 0;
  float
   x0, y0,
   x1, y1;
  if (t->points != NULL) {
    while (!dynarr_isEmpty (t->points)) {
      xph_free (*(char **)dynarr_pop (t->points));
    }
    dynarr_destroy (t->points);
    t->points = NULL;
  }
  while ((line = *(LINE **)dynarr_at (t->lines, i++)) != NULL) {
    line_coordsAtT (line, 0, &x0, &y0);
    line_coordsAtT (line, 1, &x1, &y1);
    //printf ("LINE %d of %d: x0 %.2f, y0 %.2f, f %.2f, g %.2f MAPS TO: T0 %.2f,%.2f; T1 %.2f,%.2f\n", i, dynarr_size (t->lines), line->x0, line->y0, line->f, line->g, x0, y0, x1, y1);
    point_addNoDup (points, x0, y0);
    point_addNoDup (points, x1, y1);
  }
  t->points = points;
  //printf ("WE'RE DONE WITH %s\n", __FUNCTION__);
}

void turtle_setDefault (enum turtle_settings s, ...) {
  enum turtle_pen p;
  VECTOR3 v;
  float h;
  va_list arg;
  va_start (arg, s);
  switch (s) {
    case TURTLE_PEN:
      p = va_arg (arg, enum turtle_pen);
      tDefault.penDown = p;
      break;
    case TURTLE_HEADING:
      h = (float)va_arg (arg, double);
      tDefault.heading = h;
      break;
    case TURTLE_POSITION:
      v = va_arg (arg, VECTOR3);
      tDefault.position = v;
      break;
  }
  va_end (arg);
}

int tline_count (const TLINES v) {
  return dynarr_size (*v);
}

void turtle_runPath (char * p, TURTLE * t, const SYMBOLSET * s) {
  int
    i = 0,
    l = strlen (p);
  SYMBOL * sym = NULL;
  while (i < l) {
    sym = symbol_getSymbol (s, p[i++]);
    if (sym == NULL) {
      continue;
    }
    switch (sym->type) {
      case SYM_ROTATE:
        turtle_rotate (t, sym->val);
        break;
      case SYM_MOVE:
        turtle_move (t, sym->val);
        break;
      case SYM_PENUP:
        turtle_penUp (t);
        break;
      case SYM_PENDOWN:
        turtle_penDown (t);
        break;
      case SYM_STACKPUSH:
        turtle_push (t);
        break;
      case SYM_STACKPOP:
        turtle_pop (t);
        break;
      default:
        break;
    }
  }
}

void turtle_runCycle (char * c, TURTLE * t, const SYMBOLSET * set) {
  int
    i = symbol_cyclesToClose (c, set);
  if (i < 0) {
    i = 1;
  }
  while (i-- > 0) {
    turtle_runPath (c, t, set);
  }
}

SYMBOLSET * symbol_createSet () {
  SYMBOLSET * s = xph_alloc (sizeof (SYMBOLSET));
  s->symbols = dynarr_create (2, sizeof (SYMBOL *));
  return s;
}

SYMBOL * symbol_create (char l, enum symbol_commands type, float val) {
  SYMBOL * s = xph_alloc (sizeof (SYMBOL));
  s->l = l;
  s->type = type;
  s->val = val;
  return s;
}

void symbol_destroy (SYMBOL * s) {
  xph_free (s);
}

void symbol_destroySet (SYMBOLSET * s) {
  while (!dynarr_isEmpty (s->symbols)) {
    symbol_destroy (*(SYMBOL **)dynarr_pop (s->symbols));
  }
  dynarr_destroy (s->symbols);
  xph_free (s);
}

SYMBOL * symbol_addOperation (SYMBOLSET * set, char s, enum symbol_commands type, ...) {
  SYMBOL * sym = symbol_getSymbol (set, s);
  bool overwrite = (sym == NULL) ? FALSE : TRUE;
  va_list arg;
  float v = 0;
  va_start (arg, type);
  switch (type) {
    case SYM_ROTATE:
    case SYM_MOVE:
      v = (float)va_arg (arg, double);
      if (sym == NULL) {
        sym = symbol_create (s, type, v);
      } else {
        sym->type = type;
        sym->val = v;
      }
      break;
    case SYM_PENUP:
    case SYM_PENDOWN:
    case SYM_STACKPUSH:
    case SYM_STACKPOP:
      if (sym == NULL) {
        sym = symbol_create (s, type, 0.0);
      } else {
        sym->type = type;
        sym->val = 0.0;
      }
      break;
    default:
      break;
  }
  va_end (arg);
  if (!overwrite) {
    dynarr_push (set->symbols, sym);
    dynarr_sort (set->symbols, symbol_sort);
  }
  return NULL;
}

bool symbol_isDefined (const SYMBOLSET * set, char s) {
  return (*(SYMBOL **)dynarr_search (set->symbols, symbol_search, s)) != NULL
    ? TRUE
    : FALSE;
}

SYMBOL * symbol_getSymbol (const SYMBOLSET * set, char s) {
  return *(SYMBOL **)dynarr_search (set->symbols, symbol_search, s);
}

int symbol_definedSymCount (const SYMBOLSET * set) {
  return dynarr_size (set->symbols);
}

int gcd (int m, int n) {
  int r = m % n;
  return (r == 0)
    ? n
    : gcd (n, r);
}

/*
def gcd (m, n):
  r = m % n;
  return n if r == 0 else gcd (n, r)
*/

int symbol_cyclesToClose (char * s, const SYMBOLSET * set) {
  float
    turn = 0;
  float * st = NULL;
  int
    i = 0,
    l = strlen (s);
  Dynarr rotationStack = dynarr_create (4, sizeof (float *));
  SYMBOL * sym = NULL;
  while (i < l) {
    sym = symbol_getSymbol (set, s[i++]);
    if (sym == NULL) {
      continue;
    }
    switch (sym->type) {
      case SYM_ROTATE:
        turn += sym->val;
        break;
      case SYM_STACKPUSH:
        st = xph_alloc (sizeof (float));
        *st = turn;
        dynarr_push (rotationStack, st);
        break;
      case SYM_STACKPOP:
        if (!dynarr_isEmpty (rotationStack)) {
          st = *(float **)dynarr_pop (rotationStack);
          turn = *st;
          xph_free (st);
        }
        break;
      default:
        break;
    }
  }
  while (!dynarr_isEmpty (rotationStack)) {
    xph_free (*(char **)dynarr_pop (rotationStack));
  }
  dynarr_destroy (rotationStack);
  // this probably won't work if the total turning isn't an integer.
  //printf ("total turning: %f\n", turn);
  //printf ("fmod (%f, 360.0) = %f\n", turn, fmod (turn, 360.0));
  if (turn == 0.0 || (int)turn == 0 || fcmp (fmod (turn, 360.0), 0.0) == TRUE) {
    //printf (">:( %d %d %f\n", turn == 0.0, (int)turn == 0, fmod (turn, 360.0));
    //printf ("%d\n", fcmp (-180.0, 0.0));
    return -1;
  }
  //printf ("gcd (360, %d): %d\n", (int)turn, gcd (360, (int)turn));
  //printf ("360 / %d = %d\n", gcd (360, (int)turn), abs (360 / gcd (360, (int)turn)));
  return 360 / gcd (360, abs((int)turn));
}

int symbol_sort (const void * a, const void * b) {
  return (*(SYMBOL **)a)->l - (*(SYMBOL **)b)->l;
}

int symbol_search (const void * keyp, const void * datum) {
  char
    key;
  memcpy (&key, keyp, 1);
  //printf ("%s: key: \'%c\' vs. datum: \'%c\'\n", __FUNCTION__, key, (*(SYMBOL **)datum)->l);
  return key - (*(SYMBOL **)datum)->l;
}

