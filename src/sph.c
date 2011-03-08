#include "sph.h"

SPH3 sph_axis ()
{
	SPH3
		v;
	v.r = 1.0;
	v.theta = 0.0;
	v.phi = 0.0;
	return v;
}

SPH3 sph_add (SPH3 a, SPH3 b)
{
	SPH3
		r;
	r.r = a.r + b.r;
	r.theta = a.theta + b.theta;
	if (r.theta > M_PI)
		r.theta = M_PI - (r.theta - M_PI);
	r.phi = a.phi + b.phi;
	if (r.phi < -M_PI)
		r.phi = M_PI - (-M_PI - r.phi);
	else if (r.phi > M_PI)
		r.phi = -M_PI - (M_PI - r.phi);
	return r;
}

VECTOR3 sph2vec (SPH3 s)
{
	VECTOR3
		v;
	v.x = s.r * sin (s.theta) * cos (s.phi);
	v.y = s.r * sin (s.theta) * sin (s.phi);
	v.z = s.r * cos (s.theta);
	return v;
}

SPH3 vec2sph (VECTOR3 v)
{
	SPH3
		s;
	s.r = 1.0;
	s.theta = 0.0;
	s.phi = 0.0;
	return s;
}
