#ifndef XPH_SPH_H
#define XPH_SPH_H

#include <math.h>
#include <vector.h>

typedef struct polarRotation
{
	float
		r,
		theta,
		phi;
} SPH3;

SPH3 sph_axis ();
SPH3 sph_add (SPH3 a, SPH3 b);

VECTOR3 sph2vec (SPH3 s);
SPH3 vec2sph (VECTOR3);

#endif /* XPH_SPH_H */
