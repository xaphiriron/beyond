#include "component_integrate.h"

void addExtraVelocity (struct integrate_data * idata, VECTOR3 * vel) {
  idata->extra_velocity = vectorAdd (&idata->extra_velocity, vel);
}

void applyForce (struct integrate_data * idata, VECTOR3 * force, float delta) {
  VECTOR3 f = vectorMultiplyByScalar (force, 1 / idata->mass);
  vectorMultiplyByScalar (&f, 1 / delta);
  idata->tar_acceleration = vectorAdd (&idata->acceleration, &f);
}

void applyGravity (struct integrate_data * idata, float delta) {
  VECTOR3 g = vectorCreate (0.0, 9.8, 0.0);
  //g = vectorMultiplyByScalar (&g, 1 / idata->mass);
  g = vectorMultiplyByScalar (&g, delta);
  //printf ("gravity: %7.5f, %7.5f, %7.5f @ %f seconds\n", g.x, g.y, g.z, delta);
  idata->tar_acceleration = vectorAdd (&idata->acceleration, &g);
}

void commitIntegration (Entity * e, float delta) {
  Component
    * p = entity_getAs (e, "position"),
    * i = entity_getAs (e, "integrate");
  struct position_data * pdata = NULL;
  struct integrate_data * idata = NULL;
  if (p == NULL || i == NULL) {
    return;
  }
  pdata = p->comp_data;
  idata = i->comp_data;

  //printf ("%s: entity #%d\n", __FUNCTION__, e->guid);
/*
  printf ("%s (%p):\n", __FUNCTION__, e);
  printf (" position-- new: %f, %f, %f; old: %f, %f, %f\n", idata->tar_pos.x, idata->tar_pos.y, idata->tar_pos.z, pdata->pos.x, pdata->pos.y, pdata->pos.z);
 */
  printf (
    " velocity:\n  new:  %5.2f, %5.2f, %5.2f; \n  old:  %5.2f, %5.2f, %5.2f\n  tar:  %5.2f, %5.2f, %5.2f\n  temp: %5.2f, %5.2f, %5.2f\n",
    idata->tar_velocity.x + idata->extra_velocity.x,
    idata->tar_velocity.y + idata->extra_velocity.y,
    idata->tar_velocity.z + idata->extra_velocity.z,
    idata->velocity.x, idata->velocity.y, idata->velocity.z,
    idata->tar_velocity.x, idata->tar_velocity.y, idata->tar_velocity.z,
    idata->extra_velocity.x, idata->extra_velocity.y, idata->extra_velocity.z
  );
/*
  printf (" force-- new: %f, %f, %f; old: %f, %f, %f\n", idata->tar_acceleration.x, idata->tar_acceleration.y, idata->tar_acceleration.z, idata->acceleration.x, idata->acceleration.y, idata->acceleration.z);
 //*/

  setPosition (e, idata->tar_pos);
  idata->velocity = idata->tar_velocity;
  idata->acceleration = idata->tar_acceleration;

  idata->tar_velocity = idata->tar_acceleration = idata->extra_velocity = vectorCreate (0.0, 0.0, 0.0);
  idata->tar_pos = pdata->pos;
}

void integrate (Entity * e, float delta) {
  Component
    * p = entity_getAs (e, "position"),
    * c = entity_getAs (e, "collide"),
    * i = entity_getAs (e, "integrate");
  struct position_data * pdata = NULL;
  struct integrate_data * idata = NULL;
  collide_data * cdata = NULL;
  VECTOR3
    v,
    w;
  if (p == NULL || i == NULL) {
    return;
  }
  if (c != NULL) {
    cdata = c->comp_data;
  }
  pdata = p->comp_data;
  idata = i->comp_data;
  //printf ("%s: entity #%d\n", __FUNCTION__, e->guid);

  if (cdata == NULL || cdata->onStableGround == FALSE) {
    // idk how to update acceleration, except by applying gravity.
    applyGravity (idata, delta);
  }

  v = vectorMultiplyByScalar (&idata->tar_acceleration, delta);
  v = vectorAdd (&idata->velocity, &v);
  idata->tar_velocity = v;

  v = vectorMultiplyByScalar (&idata->tar_velocity, delta);
  w = vectorMultiplyByScalar (&idata->extra_velocity, delta);
  v = vectorAdd (&v, &w);
  idata->tar_pos = vectorAdd (&pdata->pos, &v);

}


/* this depends on position, obviously, but in the end it will probably end up
 * depending on colliding, too.
 */
int component_integrate (Object * obj, objMsg msg, void * a, void * b) {
  const PHYSICS * physics = obj_getClassData (PhysicsObject, "physics");
  Entity * e = NULL;
  Vector * v = NULL;
  int i = 0;
  struct integrate_data ** cd = NULL;
  switch (msg) {
    case OM_CLSNAME:
      strncpy (a, "integrate", 32);
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
    case OM_SHUTDOWN:
    case OM_DESTROY:
      obj_destroy (obj);
      return EXIT_SUCCESS;

    case OM_COMPONENT_INIT_DATA:
      cd = a;
      *cd = xph_alloc (sizeof (struct integrate_data), "struct integrate_data");
      (*cd)->velocity = vectorCreate (0.0, 0.0, 0.0);
      (*cd)->acceleration = vectorCreate (0.0, 0.0, 0.0);
      (*cd)->mass = 1.0;
      return EXIT_SUCCESS;

    case OM_COMPONENT_DESTROY_DATA:
      cd = a;
      xph_free (*cd);
      *cd = NULL;
      return EXIT_SUCCESS;

    case OM_UPDATE:
      v = entity_getEntitiesWithComponent (2, "position", "integrate");
      //printf ("%s: iterating over %d entit%s that ha%s position/integrate components\n", __FUNCTION__, vector_size (v), (vector_size (v) == 1 ? "y" : "ies"), (vector_size (v) == 1 ? "s" : "ve"));
      i = 0;
      while (i < vector_size (v)) {
        vector_at (e, v, i++);
        integrate (e, physics->timestep);
      }
      vector_destroy (v);
      return EXIT_SUCCESS;

    case OM_POSTUPDATE:
      v = entity_getEntitiesWithComponent (2, "position", "integrate");
      i = 0;
      while (i < vector_size (v)) {
        vector_at (e, v, i++);
        commitIntegration (e, physics->timestep);
      }
      return EXIT_SUCCESS;


    default:
      return obj_pass ();
  }
  return EXIT_FAILURE;
}
