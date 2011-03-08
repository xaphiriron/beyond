#ifndef XPH_GROUND_DRAW_H
#define XPH_GROUND_DRAW_H

#include "vector.h"

#include "object.h"
#include "entity.h"

typedef struct ground_origin_label * CameraGroundLabel;
typedef struct camera_cache * CameraCache;

#include "component_camera.h"
#include "component_ground.h"
#include "hex_draw.h"

struct camera_cache {
  Entity origin;	// which entity the cache has been constructed around
  Dynarr cache;	// has hex (extent) indices; contains CameraGroundLabels; values may be NULL (since a ground is not obligated to be connected in all directions)
	unsigned int
		desiredExtent,
		nextR, nextK, nextI;
	bool
		done;
};

extern CameraCache OriginCache;

CameraCache cameraCache_create ();
void cameraCache_destroy (CameraCache cache);

void cameraCache_setExtent (unsigned int size);
void cameraCache_update (const TIMER * t);

void cameraCache_setGroundEntityAsOrigin (Entity g);
CameraGroundLabel cameraCache_getOriginLabel ();


CameraGroundLabel ground_createLabel (Entity origin, Entity this, int x, int y);
void ground_destroyLabel (CameraGroundLabel l);

VECTOR3 label_distanceFromOrigin (int size, short x, short y);

bool label_getXY (const CameraGroundLabel l, int * xp, int * yp);
Entity label_getGroundReference (const CameraGroundLabel l);
Entity label_getOriginReference (const CameraGroundLabel l);
VECTOR3 label_getOriginOffset (const CameraGroundLabel l);
int label_getCoordinateDistanceFromOrigin (const CameraGroundLabel l);

void label_setMatrix (const CameraGroundLabel l);
void label_unsetMatrix (const CameraGroundLabel l);

void ground_draw (Entity g, Entity camera, CameraGroundLabel l);
void ground_draw_fill (Entity camera);

#endif /* XPH_GROUND_DRAW_H */