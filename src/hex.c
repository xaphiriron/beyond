#include "hex.h"

/* The mapping between actual coordinates and xy values is completely and
 * utterly arbitrary. There are likely several ways to improve performance if
 * we made them not arbitrary (e.g., something to do with the result of atan2
 * or sin and cos), but I haven't bothered to think about it in detail.
 *  - xph 2010-08-24
 */
static const int hxo [6][2] = {
  {-30,  0},
  {-15,-26},
  { 15,-26},
  { 30,  0},
  { 15, 26},
  {-15, 26}
};

static const int xy[6][2] = {
  {0,1},
  {1,0},
  {1,-1},
  {0,-1},
  {-1,0},
  {-1,1}
};

int hex (int n) {
  return 3 * n * (n + 1) + 1;
}

int hex_coord_linear_offset (short r, short k, short i) {
  return (r == 0 && k == 0 && i == 0)
    ? 0
    : hex (r - 1) + (k * r) + i;
}


// this doesn't sanity-check its arguments at all, so be wary of passing invalid rki values to it.
void rki_to_xy (int r, int k, int i, int * xp, int * yp) {
  int
    kval = k % 6,
    ival = (i + 2) % 6;
  if (xp != NULL) {
    *xp = xy[kval][0] * r + xy[ival][0] * i;
  }
  if (yp != NULL) {
    *yp = xy[kval][1] * r + xy[ival][1] * i;
  }
}

void xy_to_rki (int x, int y, int * rp, int * kp, int * ip) {
  // yeah idk how to do this yet.
  int r;
  if (x <= 0 && y <= 0) {
    r = abs(x + y);
  } else if (x >= 0 && y >= 0) {
    r = x + y;
  } else {
    r = abs (x) > abs (y) ? abs (x) : abs (y);
  }

  if (rp != NULL) {
    *rp = r;
  }
}

/* expects a point, relative to the center of a hexagon, which is to be tested
 * to be inside or outside of the hexagon (as based on the hxo values). For
 * various stupid reasons (not changing the default camera angle, basically),
 * the vector's z value will be used as a y value in the same way a tile's y
 * value is actually its z value.
 */
bool hex_point_inside (const VECTOR3 * v) {
  int i = 0;
  bool inside = TRUE;
  float
    sq, rsq;
  while (i < 6) {
    sq = v->x * hxo[i][0] + v->z * hxo[i][1] + 30.01;
    rsq = sq * sq / 901.0;
    //printf ("%s: point %f,%f vs. the %d,%d line. result was %f\n", __FUNCTION__, v->x, v->z, hxo[i][0], hxo[i][1], rsq);
    if (rsq < 0) {
      inside = FALSE;
      break;
    }
    i++;
  }
  return inside;
}

struct tile * hex_create (short r, short k, short i, float c) {
  struct tile * h = xph_alloc (sizeof (struct tile), "struct tile");
  h->r = r;
  h->k = k;
  h->i = i;
  h->p = hex_position_from_coord (r, k, i);
  rki_to_xy (r, k, i, &h->x, &h->y);
  hex_slope_from_plane (h, HEX_TOP, c, c, c);
  hex_slope_from_plane (h, HEX_BOTTOM, -FLT_MAX, -FLT_MAX, -FLT_MAX);

  h->entitiesOccupying = NULL;
  return h;
}

void hex_destroy (HEX * h) {
  if (h->entitiesOccupying != NULL) {
    vector_destroy (h->entitiesOccupying);
  }
  xph_free (h);
}

bool valid_hex (const HEX * h) {
  if (h == NULL) {
    return FALSE;
  }
  if (h->p.z != 0.0f) {
    return FALSE;
  }
  if (h->k < 0 || h->k >= 6) {
    return FALSE;
  } else if (h->r == 0 && (h->k != 0 || h->i != 0)) {
    return FALSE;
  } else if (h->r != 0 && h->i >= h->r) {
    return FALSE;
  }
  return TRUE;
}

VECTOR3 hex_position_from_coord (short r, short k, short i) {
  int
    l = (k + 1) % 6,
    m = (k + 2) % 6,
    n = (k + 3) % 6;
  VECTOR3
    p = vectorCreate (0.0, 0.0, 0.0),
    q;
  assert (k >= 0 && k < 6);
  if (r == 0) {
    assert (k == 0 && i == 0);
    return p;
  }
  assert (i < r);
  p = vectorCreate (hxo[k][0] + hxo[l][0], hxo[k][1] + hxo[l][1], 0.0);
  p = vectorMultiplyByScalar (&p, r);
  if (i == 0) {
    return p;
  }
  q = vectorCreate (hxo[m][0] + hxo[n][0], hxo[m][1] + hxo[n][1], 0.0);
  q = vectorMultiplyByScalar (&q, i);
  p = vectorAdd (&p, &q);
  return p;
}

void hex_slope_from_plane (HEX * h, enum hex_topbottom t, float z, float a, float b) {
  // the a offset is hxo[0], the b offset is hxo[1]. if we change the hex mapping (which we probably will) then we will need to update this code or else everything that depends on it working (most notably, tile collisions) will break
  VECTOR3
    av, bv;
  // if we end up storing the other values, calculate them here.
  if (t == HEX_TOP) {
    h->z = z * 15;
    h->a = a * 15;
    h->b = b * 15;
    av = vectorCreate (hxo[0][0], h->a - h->z, hxo[0][1]);
    bv = vectorCreate (hxo[1][0], h->b - h->z, hxo[1][1]);
    h->surfaceNormal = vectorCross (&bv, &av);
    h->surfaceNormal = vectorNormalize (&h->surfaceNormal);
  } else {
    h->dz = z * 15;
    h->da = a * 15;
    h->db = b * 15;
    av = vectorCreate (hxo[0][0], h->da - h->dz, hxo[0][1]);
    bv = vectorCreate (hxo[1][0], h->db - h->dz, hxo[1][1]);
    h->baseNormal = vectorCross (&av, &bv);
    h->baseNormal = vectorNormalize (&h->baseNormal);
  }
}

/* as you can see, it is a monumental hassle to find the neighboring tiles when you are using polar coordinates. Even worse: the directions these neighbors lie differs based on the location of the tile, which makes mapping the neighbor tiles to the direction you will find them in even more of a challenge. This is why we also use standard x,y coordinates.
 */
HEX ** hex_neighbors (const MAP * m, const HEX * h) {
  HEX ** n = xph_alloc (sizeof (HEX *) * 6, "hex_neighbors");
  short
    r = h->r,
    k = h->k,
    i = h->i;
  int
    hi, li,
    hic, lic;
  //printf ("n: %p. 0-5: %p-%p\n", n, &n[0], &n[5]);
  if (r == 0) {
    assert (k == 0 && i == 0);
    vector_at (n[0], m->tiles, hex_coord_linear_offset (1, 0, 0));
    vector_at (n[1], m->tiles, hex_coord_linear_offset (1, 1, 0));
    vector_at (n[2], m->tiles, hex_coord_linear_offset (1, 2, 0));
    vector_at (n[3], m->tiles, hex_coord_linear_offset (1, 3, 0));
    vector_at (n[4], m->tiles, hex_coord_linear_offset (1, 4, 0));
    vector_at (n[5], m->tiles, hex_coord_linear_offset (1, 5, 0));
    return n;
  }
  assert (i < r);
  assert (k >= 0 && k < 6);
  hi = (i + 1) % r;
  li = (i == 0 ? r - 1 : i - 1);
  hic = hi != i + 1;
  lic = li != i - 1;
  vector_at (n[0], m->tiles, hex_coord_linear_offset (r + 1, k, i));
  vector_at (n[1], m->tiles, hex_coord_linear_offset (r + 1, k, i + 1));
  vector_at (n[2], m->tiles, hex_coord_linear_offset (r, k + lic, li));
  vector_at (n[3], m->tiles, hex_coord_linear_offset (r, k + hic, hi));
  if (r == 1) {
    //printf ("r == 1\n");
    vector_at (n[4], m->tiles, hex_coord_linear_offset (0, 0, 0));
  } else if (i == r - 1) { // pretty sure (hic) would work here
    //printf ("i == r - 1\n");
    vector_at (n[4], m->tiles, hex_coord_linear_offset (r - 1, k, i - 1));
  } else {
    //printf ("{else}\n");
    vector_at (n[4], m->tiles, hex_coord_linear_offset (r - 1, k, i));
  }
  if (i == 0) {
    //printf ("i == 0\n");
    vector_at (n[5], m->tiles, hex_coord_linear_offset (r + 1, k, r));
  } else {
    //printf ("else: {%d %d %d}\n", r, k, i);
    // I have no clue what this means.
    vector_at (n[5], m->tiles, hex_coord_linear_offset (r - 1, k, (i + 1) % (r - 1)));
  }
  return n;
}

void hex_draw (const HEX * t) {
  // todo: a simple and common case of this is t->z == t->a == t->b, in which case all these values are t->z and we don't need to do any calculation. (and as usual, this is a choice between CPU and memory: if we store these values in the tile struct we will have four more floats in memory per tile but we won't have to re-calculate these values every frame.)
  float
    ra = t->z - t->a,
    rb = t->z - t->b,
    d = t->z + ra,
    e = t->z + rb,
    // I don't know why these two work, but they do.
    c = t->z + (ra + (t->z - e)),
    f = t->z + ((t->z - d) + rb),
    surfaces[6] = {t->a, t->b, c, d, e, f};
  int
    i = 0,
    j = 1;
  //printf ("height values for {%d %d %d}: %7.3f, %7.3f %7.3f %7.3f %7.3f %7.3f %7.3f\n", t->r, t->k, t->i, t->z, t->a, t->b, c, d, e, f);
  glColor3f (0.95, 0.9, 0.8);
  glBegin (GL_TRIANGLE_FAN);
  glVertex3f (t->p.x + hxo[0][0], t->p.z + t->a, t->p.y + hxo[0][1]);
  glVertex3f (t->p.x + hxo[5][0], t->p.z + f,    t->p.y + hxo[5][1]);
  glVertex3f (t->p.x + hxo[4][0], t->p.z + e,    t->p.y + hxo[4][1]);
  glVertex3f (t->p.x + hxo[3][0], t->p.z + d,    t->p.y + hxo[3][1]);
  glVertex3f (t->p.x + hxo[2][0], t->p.z + c,    t->p.y + hxo[2][1]);
  glVertex3f (t->p.x + hxo[1][0], t->p.z + t->b, t->p.y + hxo[1][1]);
  glVertex3f (t->p.x + hxo[0][0], t->p.z + t->a, t->p.y + hxo[0][1]);
  glEnd ();

  // draw the tile's surface normal
  glColor3f (0.0, 1.0, 0.0);
  glBegin (GL_LINES);
  glVertex3f (t->p.x, t->z, t->p.y);
  glVertex3f (t->p.x + t->surfaceNormal.x * 16, t->z + t->surfaceNormal.y * 16, t->p.y + t->surfaceNormal.z * 16);
  glEnd ();

  /* the constant problem with drawing hex tile edges:
   *  - we want to draw only edges that are visible, so we want to skip edges
   *    that, for example, are between two tiles that have the same height
   *  - we want to only draw as much of a edge as we need, for example, for a
   *    tile that is ten units tall but next to a tile which is nine units
   *    tall, we only want to draw the final one unit which is not overlapped
   *  - we want the edges to match up with both the surface/depth heights of
   *    this tile and the surface/depth heights of the neighboring tile
   *  - we want to draw each edge once and once only, even though each edge is
   *    between two tiles
   * the solutions to the various issues I've worked out are as follows:
   *  - a list of neighbored tiles is passed to the drawing function, sorted
   *    in clockwise/counterclockwise order, so that it is reasonably simple
   *    to iterate over them each in turn as we draw edges.
   *  - tiles draw downward edges only. if a neighboring tile is above it,
   *    their shared edge will be drawn by the taller neighboring tile.
   * remaining issues:
   *  - criss-crossing edges (ones that look like X when seen on edge). either
   *    we draw the edge twice or we do complex math to work out the cross
   *    point and draw a triangular wedge for each side.
   *  - short tiles next to tall tiles. If we have a tile that goes from 0 to
   *    10 with a neighboring tile that goes from 4 to 6, the tall tile is
   *    responsible for drawing two seperate polygons: one from 10 to 6, and
   *    another from 4 to 0.
   *  - general issues with tile "stacks"-- if there are multiple tiles on one
   *    coordinate, edge drawing is even further complicated.
   * you may note that the current drawing method does not use any of the
   * named solutions yet. With the polar coordinate system it is rather
   * difficult to get the neighbors in clockwise order of any tile except for
   * the origin. Translating tiles is also unexpectedly difficult. This is,
   * perhaps, a reason to switch coordinate representations.
   */
  while (i < 6) {
    switch (i) {
      case 0:
      case 1:
        glColor3f (0.8, 0.75, 0.6);
        break;
      case 2:
      case 5:
        glColor3f (0.7, 0.65, 0.5);
        break;
      default:
        glColor3f (0.6, 0.55, 0.4);
        break;
    }
    glBegin (GL_TRIANGLE_STRIP);
    glVertex3f (t->p.x + hxo[i][0], t->p.z + surfaces[i], t->p.y + hxo[i][1]);
    glVertex3f (t->p.x + hxo[j][0], t->p.z + surfaces[j], t->p.y + hxo[j][1]);
    glVertex3f (t->p.x + hxo[i][0], t->p.z - t->z, t->p.y + hxo[i][1]);
    glVertex3f (t->p.x + hxo[j][0], t->p.z - t->z, t->p.y + hxo[j][1]);
    glEnd ();
    i++;
    j = (j + 1) % 6;
  }
}

void hex_draw_sides (const HEX * t, const HEX ** n) {
}

void hex_draw_fake (short r, short k, short i) {
  VECTOR3 o = hex_position_from_coord (r, k, i);

  //printf ("drawing fake hex at {%d %d %d} \n", r, k, i);
  glColor3f (0.7, 0.7, 0.7);
  glBegin (GL_POLYGON);
  glVertex3f (o.x + hxo[0][0], 0.0f, o.y + hxo[0][1]);
  glVertex3f (o.x + hxo[5][0], 0.0f, o.y + hxo[5][1]);
  glVertex3f (o.x + hxo[4][0], 0.0f, o.y + hxo[4][1]);
  glVertex3f (o.x + hxo[3][0], 0.0f, o.y + hxo[3][1]);
  glVertex3f (o.x + hxo[2][0], 0.0f, o.y + hxo[2][1]);
  glVertex3f (o.x + hxo[1][0], 0.0f, o.y + hxo[1][1]);
  glVertex3f (o.x + hxo[0][0], 0.0f, o.y + hxo[0][1]);
  glEnd ();
}


MAP * map_create (int size) {
  MAP * m = xph_alloc (sizeof (MAP), "MAP");
  m->size = (size <= 0 ? 1 : size);
  m->tiles = vector_create (hex (m->size) + 1, sizeof (HEX *));
  m->edge = EDGE_WALL;
  return m;
}

void map_destroy (MAP * m) {
  int
    i = 0,
    s = hex (m->size);
  HEX * h = NULL;
  while (i < s) {
    vector_at (h, m->tiles, i++);
    if (h == NULL) {
      printf ("%s (%p): missing tile on offset %d\n", __FUNCTION__, m, i - 1);
      continue;
    }
    hex_destroy (h);
  }
  vector_destroy (m->tiles);
  xph_free (m);
}

void map_draw (const MAP * m) {
  int
    r = 1, k = 0, i = 0;
  HEX * h;
  //HEX ** n = NULL;
  vector_at (h , m->tiles, 0);
  if (valid_hex (h)) {
    hex_draw (h);
    //n = hex_neighbors (m, h);
    //hex_draw_sides (h, (const HEX **)n);
    //xph_free (n);
    //n = NULL;
  } else {
    hex_draw_fake (0, 0, 0);
  }
  while (r <= m->size) {
    k = 0;
    while (k < 6) {
      i = 0;
      while (i < r) {
        //printf ("%s: fetching hex #%d ({%d %d %d})\n", __FUNCTION__, hex (r - 1) + (k * r) + i, r, k, i);
        vector_at (h, m->tiles, hex_coord_linear_offset (r, k, i));
        if (valid_hex (h)) {
          hex_draw (h);
          //n = hex_neighbors (m, h);
          //hex_draw_sides (h, (const HEX **)n);
          //xph_free (n);
          //n = NULL;
        } else {
          hex_draw_fake (r, k, i);
        }
        i++;
      }
      k++;
    }
    r++;
  }
  if (m->edge == EDGE_WALL) {
    // draw extra tiles around the edge with a height of FLT_MAX
  }
}

HEX * map_hex_at_point (MAP * m, float x, float y) {
  int
    cx = x / 45.0,
    cy = (y - (cx * 26)) / 52.0,
    r, k, i;
  HEX * h = NULL;
  if (abs (cx) > m->size || abs (cy) > m->size) {
    // (tile past the edge of the map)
    return NULL;
  }
  xy_to_rki (cx, cy, &r, &k, &i);
  return vector_at (h, m->tiles, hex_coord_linear_offset (r, k, i));
}

Vector * map_adjacent_tiles (MAP * m, int x, int y) {
  Vector * v = vector_create (8, sizeof (HEX *));
  int i = 0;
  while (i < 6) {
    vector_push_back (v, map_hex_at_point (m, x + xy[i][0], y + xy[i][0]));
    i++;
  }
  return v;
}

/*
int hex (int n);
VECTOR3 hex_offset (int n, float dist);

int hex (int n) {
  return 3 * n * (n + 1) - 1;
}

VECTOR3 hex_offset (int n, float dist) {
  VECTOR3 v = vectorCreate (0.0, 0.0, 0.0);

  if (n == 0) {
    return v;
  }
  // magic happens here

  return v;
}

 *    PI / 2
 *      |
 *      |
 * PI -- -- 0, PI * 2
 *      |
 * ^    |
 * | PI * 1.5
 * y
 * /x->    x,y == cos,sin
 * DEGTORAG(x)		((x) / 180.0 * M_PI)
 * RADTODEG(x)		((x) / M_PI * 180.0)
 *

// sin (60 / 180.0 * M_PI)
#define HEXANGLE 0.8660254

simple cases:
1-6: 
// (1 = .866,.5; 2 = .866,-.5; 3 = .0,-1.0; 4 = -.866,-.5; 5 = -.866,.5; 6 = .0,1.0) with an arbitrary numbering; might want to alter so that the atan2 calcs or whatever match up with the angle offsets
other cases:

int
  mult = ceil (hxrt (n + 1)),
  base = hex (mult - 1);
float
  d = dist * mult;
  // there are six "corner" values for each "level" of the hexagon. m0 is the largest corner value that is <= n, and m1 is the one following that
int
  m0 = n - ((n - hex (base) + 1) % mult),
  m1 = m0 + mult;
 * SPECIAL CASE: n is close to the lowest point on the level, so the m0 calculation has a value that is actually in the next-smallest level when it should wrap around (i.e., the result is -1 % hex (base + 1)).
 * (the real question is does it matter-- we'll end up using the right magnitude either way, since that's what mult and d are for, and there's the right difference between m0 and m1 to do the usual interpolation step when "fixing" it would mean you'd have to do a special integration step for this case)
 * anyway the code to 'fix' it is as below but I don't know if you really need it.
if (m0 == base - 1) {
  m0 = hex (base + 1) - 1;
}
 *

if (n == m0) {
  // do some magic and return m0's coordinate (which probably involves finding the i in this statement: (hex (n) + n) + (i * n), which would then be the offset value to use [see above 'simple cases' values] multiplied by d)
} else if (n == m1) {
  // do some magic and return m1's coordinate
}
// do some magic and linearly interpolate between m0 and m1's coordinates. keep an eye out for the special case noted above.
*/