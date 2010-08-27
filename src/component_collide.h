#ifndef XPH_COMPONENT_COLLIDE_H
#define XPH_COMPONENT_COLLIDE_H

#include "vector.h"
#include "hex.h"

#include "object.h"
#include "entity.h"

#include "component_position.h"

enum collide_types {
  COLLIDE_SPHERE,
  COLLIDE_CYLINDER_ROUND,	// do a sphere distance test against the ends
  COLLIDE_CYLINDER_FLAT,	// do a plane/circle distance test against the ends

  // ...

  COLLIDE_BOX,			// not supported
  COLLIDE_TETRAHEDRON,		// not supported

  COLLIDE_MODEL			// lol so not supported
};

struct sphere_collide {
  enum collide_types type;
  float radius;
};

struct cylinder_collide {
  enum collide_types type;
  float
    halflength,		// half length of distance along position up orientation axis.
    radius;
};

struct box_collide {
  enum collide_types type;
  // BOUNDED BY THE PLANES DEFINED BY VECTORS IN THE ENTITY'S POSITION ORIENTATION AXES
  float
    depth,	// forward-size
    bredth,	// side-size
    height;	// up-size
};

// should we also store "nearby things we could possibly collide with" in the collide data?
typedef struct {
  union {
    enum collide_types type;
    struct sphere_collide sph;
    struct cylinder_collide cyl;
    struct box_collide box;
  } i;
  bool onStableGround;
} collide_data;

// a is always set, b or h set depending on type
typedef struct {
  Entity
    * a,
    * b;
  HEX * h;
  enum ent_or_hex {
    COLLIDE_HEX,
    COLLIDE_ENT
  } type;
  VECTOR3 pointofcontact;
  // ??? WHAT ELSE?
} collide_intersection;


struct check {
  unsigned int a, b;
};

void mark_checked (const Entity * e, const Entity * f);
bool marked_checked (const Entity * e, const Entity * f);
void clear_checked ();
void destroy_checked ();
int check_sort (const void * a, const void * b);
int check_search (const void * k, const void * d);


collide_intersection * collide_entity (Entity * e, Entity * f);
collide_intersection * collide_hex (Entity * e, HEX * hex);

collide_intersection * collide_spheres (float jx, float jy, float jz, float jr, float kx, float ky, float kz, float kr);
/* data required to calculate sphere intersections:	(4)
 *  base x,y,z (pdata->pos)					(3 floats)
 *  radius (cdata->sph.radius)					(1 float)
 *
 * data required to calculate capsule intersections:	(8)
 *  base x,y,z (pdata->pos)					(3 floats)
 *  line x,y,z (pdata->orient.up)				(3 floats)
 *  segment length (cdata->cyl->halflength)			(1 float)
 *  distance/radius from line (cdata->cyl->radius)		(1 float)
 *
 * data required to calculate tetrahedron intersections: (12)
 *  base x,y,z			(3)
 *  3 vertices/vector edges	(9)
 *
 * data required to calculate box intersections:	(15)
 *  base x,y,z (pdata->pos)					(3 floats)
 *  all orientation axes (pdata->orient)			(9 floats)
 *  dimension of box sides (cdata->box.depth/bredth/height)	(3 floats)
 */

Vector * collide_footprint (Entity * e);
void collide_update ();
void collide_response (collide_intersection * x);

void setCollideType (Entity * e, enum collide_types type);
void setCollideRadius (Entity * e, float r);


int component_collide (Object * obj, objMsg msg, void * a, void * b);

#endif /* XPH_COMPONENT_COLLIDE_H */