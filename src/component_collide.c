#include "component_collide.h"

static Vector * Checked = NULL;

void mark_checked (const Entity * e, const Entity * f) {
  struct check * c = NULL;
  if (Checked == NULL) {
    Checked = vector_create (8, sizeof (struct check *));
  }
  if (marked_checked (e, f)) {
    return;
  }
  c = xph_alloc (sizeof (struct check), "struct check");
  if (e->guid < f->guid) {
    c->a = e->guid;
    c->b = f->guid;
  } else {
    c->b = e->guid;
    c->a = f->guid;
  }
  vector_push_back (Checked, c);
  vector_sort (Checked, check_sort);
}

bool marked_checked (const Entity * e, const Entity * f) {
  struct check c;
  if (Checked == NULL) {
    return FALSE;
  }
  if (e->guid < f->guid) {
    c.a = e->guid;
    c.b = f->guid;
  } else {
    c.b = e->guid;
    c.a = f->guid;
  }
  return
    (vector_search (Checked, &c, check_search) != NULL)
      ? TRUE
      : FALSE;
}

void clear_checked () {
  struct check * c = NULL;
  if (Checked != NULL) {
    while (vector_size (Checked) > 0) {
      vector_pop_back (c, Checked);
      xph_free (c);
    }
  }
}

void destroy_checked () {
  clear_checked ();
  if (Checked != NULL) {
    vector_destroy (Checked);
    Checked = NULL;
  }
}

int check_sort (const void * t, const void * u) {
  int q = (*(struct check **)t)->a - (*(struct check **)u)->a;
  return
    (q == 0)
      ? (*(struct check **)t)->b - (*(struct check **)u)->b
      : q;
}

int check_search (const void * k, const void * d) {
  int q = ((struct check *)k)->a - (*(struct check **)d)->a;
  return
    (q == 0)
      ? ((struct check *)k)->b - (*(struct check **)d)->b
      : q;
}



// if we want to make holes in the world or the edges of the world act like solid tiles with infinite height and depth, we're going to have to pass something different in, since a null hex pointer doesn't give any information about the direction or location of the missing hex.
// also, collide_intersection stores two entity pointers, and it's assumed that they're both objects which will rebound. hexes are not entities, and they are fixed immutably in place (at least with regard to physics interactions), so that also needs to be altered.
collide_intersection * collide_hex (Entity * e, HEX * hex) {
  Component
    * c = NULL,
    * p = NULL;
  struct position_data
    * pdata = NULL;
  collide_data
    * cdata = NULL;
  collide_intersection
    * x = NULL;
  VECTOR3
    distanceFromTileCenter,
    intersection,
    relIntersection;
  float
    angleFromTileCenter = 0.0,
    planeDistance = 0.0;
  int side = 0;
  if (e == NULL || hex == NULL) {
    return NULL;
  }
  p = entity_getAs (e, "position");
  c = entity_getAs (e, "collide");
  if (p == NULL || c == NULL) {
    return NULL;
  }
  pdata = p->comp_data;
  cdata = c->comp_data;
  switch (cdata->i.type) {
    case COLLIDE_SPHERE:
      // there are four possibilities:
      //  there is no collision, collision with tile surface, collision with tile bottom, collision with tile sides.
      // Given the shape of the tiles, it's impossible for the sphere to collide with both the top and bottom of a tile (unless it's inside the tile, which is what we're trying to avoid in the first place), or collide with more than one of the six side quads. I think to be sure of a side intersection or lack thereof two sides have to be tested. Which side(s) to be tested can be determined by an atan2() call, passing the distance from the tile center. I have no clue how to do the actual side tests, even though they're flat quads aligned with the y axis. the tile bottom test is the exact same thing as the tile surface test, with I think a few signs inverted since the normal will be pointing down.

      distanceFromTileCenter = vectorSubtract (&pdata->pos, &hex->p);
      distanceFromTileCenter.y = pdata->pos.y - hex->z;
      planeDistance = vectorDot (&distanceFromTileCenter, &hex->surfaceNormal);
      //printf ("distance between plane %5.3f, %5.3f, %5.3f [at origin] and sphere center %5.3f, %5.3f, %5.3f: %f\n", hex->surfaceNormal.x, hex->surfaceNormal.y, hex->surfaceNormal.z, distanceFromTileCenter.x, distanceFromTileCenter.y, distanceFromTileCenter.z, planeDistance);
      if (planeDistance >= -cdata->i.sph.radius && planeDistance <= cdata->i.sph.radius) {
        intersection = vectorMultiplyByScalar (&hex->surfaceNormal, -planeDistance);
        intersection = vectorAdd (&pdata->pos, &intersection);
        relIntersection = vectorSubtract (&hex->p, &intersection);
        // do a 2d test to see if relIntersection is inside the hxo values
        if (hex_point_inside (&relIntersection) == TRUE) {
          //printf ("OH MY GOD WE HAVE A TILE INTERSECTION! :DDDD\n");
          x = xph_alloc (sizeof (collide_intersection), "collide_intersection");
          x->type = COLLIDE_HEX;
          x->a = e;
          x->b = NULL;
          x->h = hex;
          x->pointofcontact = intersection;
          return x;
        }
      }
      angleFromTileCenter = atan2 (distanceFromTileCenter.x, distanceFromTileCenter.z);
      side = floor ((angleFromTileCenter + M_PI) / (M_PI / 3.0));
      //we want side to be a value [0..5] inclusive, but iirc from phantomnation this equation will actually fail. also, this being the correct side value depends on the sides being in the right order
      // to actually check if the sphere is hitting the side, we need to... do something. not just a check vs. the tile center, or the tile slope.
      // now do all that plane math with the base normal too :(
      if (hex->dz == -FLT_MAX) {
        // no collision possible at center of the world
        return NULL;
      }
      break;
    default:
      break;
  }
  return NULL;
}

collide_intersection * collide_entity (Entity * e, Entity * f) {
  Component
    * c = NULL,
    * d = NULL,
    * p = NULL,
    * q = NULL;
  struct position_data
    * pdata = NULL,
    * qdata = NULL;
  collide_data
    * cdata = NULL,
    * ddata = NULL;
  collide_intersection * x = NULL;
  if (e == NULL || f == NULL) {
    return NULL;
  }
  p = entity_getAs (e, "position");
  q = entity_getAs (f, "position");
  c = entity_getAs (e, "collide");
  d = entity_getAs (f, "collide");
  if (p == NULL || q == NULL || c == NULL || d == NULL) {
    return NULL;
  }
  pdata = p->comp_data;
  qdata = q->comp_data;
  cdata = c->comp_data;
  ddata = d->comp_data;
  switch (cdata->i.type) {
    case COLLIDE_SPHERE:
      switch (ddata->i.type) {
        case COLLIDE_SPHERE:
          x = collide_spheres (
            pdata->pos.x, pdata->pos.y, pdata->pos.z, cdata->i.sph.radius,
            qdata->pos.x, qdata->pos.y, qdata->pos.z, ddata->i.sph.radius
          );
          if (x != NULL) {
            x->a = e;
            x->b = f;
          }
          return x;
        default:
          break;
      }
      break;
    default:
      break;
  }
  return NULL;
}



collide_intersection * collide_spheres (float lx, float ly, float lz, float lr, float kx, float ky, float kz, float kr) {
  collide_intersection * x = NULL;
  float
    xlk = lx - kx,
    ylk = ly - ly,
    zlk = lz - lz,
    rksq = kr * kr,
    rlsq = lr * lr,
    distsq = xlk * xlk + ylk * ylk + zlk * zlk,
    delrsq, sumrsq, root, dstinv, scl;
  if (fcmp (distsq, 0.0) == TRUE) {
    // these two spheres overlap completely.
    return NULL;
  }
  delrsq = rlsq - rksq;
  sumrsq = rksq + rlsq;
  root = distsq - sumrsq;
  if (root > 0.01) {
    return NULL;
  }
  x = xph_alloc (sizeof (collide_intersection), "collide_intersection");
  dstinv = 0.5 / distsq;
  scl = 0.5 - delrsq * dstinv;
  x->pointofcontact.x = xlk * scl + kx;
  x->pointofcontact.y = ylk * scl + ky;
  x->pointofcontact.z = zlk * scl + kz;
  if (root < -0.01) {
    // interpenetration going on. WHO CARES LETS JUST IGNORE IT
  }
  return x;
}

Vector * collide_footprint (Entity * e) {
  WORLD * w = NULL;
  Component * p = NULL;
  struct position_data * pdata = NULL;
  HEX * h = NULL;
  Vector * v = NULL;
  p = entity_getAs (e, "position");
  if (p == NULL) {
    fprintf (stderr, "%s: colliding entity #%d doesn't have a position\n", __FUNCTION__, e->guid);
    return NULL;
  }
  pdata = p->comp_data;
  h = pdata->tileOccupying;
  if (h == NULL) {
    fprintf (stderr, "%s: colliding entity #%d is outside the world\n", __FUNCTION__, e->guid);
    return NULL;
  }
  if (WorldObject == NULL) {
    fprintf (stderr, "%s: World doesn't exist.\n", __FUNCTION__);
    return NULL;
  }
  w = obj_getClassData (WorldObject, "world");
  v = map_adjacent_tiles (w->map, h->x, h->y);
  vector_push_back (v, h);
  return v;
}

void collide_update () {
  Vector
    * colliders = NULL,
    * collisions = vector_create (2, sizeof (collide_intersection *)),
    * tiles = NULL;
  collide_intersection * x = NULL;
  int
    i = 0,
    j = 0,
    k = 0;
  Entity
    * e = NULL,
    * f = NULL;
  HEX * t = NULL;
  colliders = entity_getEntitiesWithComponent (3, "position", "integrate", "collide");

  clear_checked ();
  while (vector_at (e, colliders, i++) != NULL) {
    tiles = collide_footprint (e);
    if (tiles == NULL) {
      fprintf (stderr, "%s: Entity %p located outside the world.\n", __FUNCTION__, e);
      continue;
    }
    j = 0;
    while (j < vector_size (tiles)) {
      vector_at (t, tiles, j++);
      if (t == NULL) {
        continue;
      }
      x = collide_hex (e, t);
      if (x != NULL) {
        vector_push_back (collisions, x);
      }
      if (t->entitiesOccupying == NULL) {
        continue;
      }
      k = vector_size (t->entitiesOccupying);
      while (--k > 0) {
        vector_at (f, t->entitiesOccupying, k);
        if (!marked_checked (e, f)) {
         x = collide_entity (e, f);
          if (x != NULL) {
            vector_push_back (collisions, x);
          }
          mark_checked (e, f);
        }
      }
    }
  }
  while (vector_size (collisions) > 0) {
    vector_pop_back (x, collisions);
    collide_response (x);
    xph_free (x);
  }
  vector_destroy (collisions);
  clear_checked ();
}

void collide_response (collide_intersection * x) {
  Component
    * p = NULL,
    * i = NULL,
    * c = NULL;
  struct position_data * pdata;
  struct integrate_data * idata;
  collide_data * cdata;
  if (x == NULL) {
    return;
  }
  if (x->type == COLLIDE_HEX) {
    // to "bounce" what we'd do is take x->pointofintersection - a->pos * a->{bounciness} and set that as a's new velocity. but we're not going to do that. We're going
    //printf ("LETS DO THIS:\n");
    p = entity_getAs (x->a, "position");
    i = entity_getAs (x->a, "integrate");
    c = entity_getAs (x->a, "collide");
    if (c == NULL) {
      return;
    }
    pdata = p->comp_data;
    cdata = c->comp_data;
    cdata->onStableGround = TRUE;
    if (i != NULL) {
      //printf ("zeroing velocity and acceleration\n");
      idata = i->comp_data;
      idata->velocity = vectorCreate (0, 0, 0);
      idata->acceleration = vectorCreate (0, 0, 0);
      idata->tar_velocity = idata->tar_acceleration = vectorCreate (0.0, 0.0, 0.0);
    }
  }
}

  // note the complete lack of any checking for validity or prior shape.
void setCollideType (Entity * e, enum collide_types type) {
  Component * c = NULL;
  collide_data * cdata = NULL;
  if (e == NULL) {
    return;
  }
  c = entity_getAs (e, "collide");
  if (c == NULL) {
    return;
  }
  cdata = c->comp_data;
  cdata->i.type = type;
}

void setCollideRadius (Entity * e, float r) {
  Component * c = NULL;
  collide_data * cdata = NULL;
  if (e == NULL || (c = entity_getAs (e, "collide")) == NULL) {
    return;
  }
  cdata = c->comp_data;
  switch (cdata->i.type) {
    case COLLIDE_SPHERE:
      cdata->i.sph.radius = r;
      break;
    case COLLIDE_CYLINDER_ROUND:
    case COLLIDE_CYLINDER_FLAT:
      cdata->i.cyl.radius = r;
      break;
    default:
      break;
  }
}

int component_collide (Object * obj, objMsg msg, void * a, void * b) {
  collide_data ** cd = NULL;
  switch (msg) {
    case OM_CLSNAME:
      strncpy (a, "collide", 32);
      return EXIT_SUCCESS;
    case OM_CLSINIT:
    case OM_CLSFREE:
    case OM_CLSVARS:
    case OM_CREATE:
      return EXIT_FAILURE;
    default:
      break;
  }
  switch (msg) {
    case OM_COMPONENT_INIT_DATA:
      cd = a;
      *cd = xph_alloc (sizeof (collide_data), "collide_data");
      (*cd)->i.type = COLLIDE_SPHERE;
      (*cd)->i.sph.radius = 1.0;
      return EXIT_SUCCESS;

    case OM_COMPONENT_DESTROY_DATA:
      cd = a;
      xph_free (*cd);
      *cd = NULL;
      return EXIT_SUCCESS;

    case OM_UPDATE:
      collide_update ();
      return EXIT_SUCCESS;

    case OM_POSTUPDATE:
      return EXIT_SUCCESS;

    default:
      return obj_pass ();
  }
  return EXIT_FAILURE;
}