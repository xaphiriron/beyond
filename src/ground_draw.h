#ifndef XPH_GROUND_DRAW_H
#define XPH_GROUND_DRAW_H

#include "vector.h"

#include "object.h"
#include "entity.h"

#include "component_ground.h"

typedef struct ground_origin_label * CameraGroundLabel;
typedef struct camera_cache * CameraCache;

#include "hex_draw.h"

struct camera_cache {
  Entity origin;	// which entity the cache has been constructed around
  Dynarr cache;	// has hex (extent) indices; contains CameraGroundLabels; values may be NULL (since a ground is not obligated to be connected in all directions)
  unsigned short extent; // how many steps outward the cache has been filled to
};

extern CameraCache OriginCache;

CameraCache cameraCache_create ();
void cameraCache_destroy (CameraCache cache);

void cameraCache_extend (int size);

void cameraCache_setGroundEntityAsOrigin (Entity g);
CameraGroundLabel cameraCache_getOriginLabel ();


CameraGroundLabel ground_createLabel (Entity origin, Entity this, int x, int y, int rot);
void ground_destroyLabel (CameraGroundLabel l);

VECTOR3 label_distanceFromOrigin (int size, short x, short y);

bool label_getXY (const CameraGroundLabel l, int * xp, int * yp);
Entity label_getGroundReference (const CameraGroundLabel l);
Entity label_getOriginReference (const CameraGroundLabel l);
VECTOR3 label_getOriginOffset (const CameraGroundLabel l);
int ground_getLabelRotation (const CameraGroundLabel l);
int label_getCoordinateDistanceFromOrigin (const CameraGroundLabel l);

// ultimately this will require a camera entity reference to draw, since
// ultimately i will be doing my own depth-sorting instead of relying on
// opengl to do it for me. right now, however, (and pending finalization
// of the camera code) it's all automatic and inefficient. - xax 10-2010
void ground_draw (Entity g, Entity camera, CameraGroundLabel l);
void ground_draw_fill (Entity origin, Entity camera, int stepsOutward);

#endif /* XPH_GROUND_DRAW_H */