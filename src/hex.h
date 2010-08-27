#ifndef XPH_HEX_H
#define XPH_HEX_H

#include <float.h>
#include <SDL/SDL_opengl.h>
#include "vector.h"
#include "xph_memory.h"
#include "cpv.h"

/* todo: weigh the benefits and costs of making this a component, for various
 * values of "this"
 */

typedef struct tile {
  short
    r, k, i;		// polar coordinates from origin; used only to make a linear list from a hexagonal playfield
  int
    x, y;		// use these coordinates instead.
  VECTOR3
    p,
    surfaceNormal,
    baseNormal;
  float
    z, a, b,		// the plane of the surface of the hex
    dz, da, db;		// the plane of the bottom of the hex

  Vector * entitiesOccupying;
} HEX;


typedef struct field {
  Vector * tiles;
  int size;
  enum map_border {
    EDGE_WALL,
    EDGE_VOID
  } edge;
} MAP;

enum hex_topbottom {
  HEX_TOP,
  HEX_BOTTOM
};

int hex (int n);
int hex_coord_linear_offset (short r, short k, short i);
void rki_to_xy (int r, int k, int i, int * xp, int * yp);
void xy_to_rki (int x, int y, int * rp, int * kp, int * ip);

bool hex_point_inside (const VECTOR3 * v);

HEX * hex_create (short r, short k, short i, float h);
void hex_destroy (HEX * h);

bool valid_hex (const HEX * h);
VECTOR3 hex_position_from_coord (short r, short k, short i);
void hex_slope_from_plane (HEX * h, enum hex_topbottom t, float z, float a, float b);
HEX ** hex_neighbors (const MAP * m, const HEX * h);

void hex_draw (const HEX * h);
void hex_draw_sides (const HEX * h, const HEX ** n);

void hex_draw_fake (short r, short k, short i);


MAP * map_create (int s);
void map_destroy (MAP * m);

void map_draw (const MAP * m);

HEX * map_hex_at_point (MAP * m, float x, float y);
Vector * map_adjacent_tiles (MAP * m, int x, int y);

#endif /* XPH_HEX_H */