#include "shapes.h"

#define		CUTOUT_MAX_CORNERS		8
#define		CUTOUT_MAX_PIECES		16

struct point
{
	short
		x, y;
};

struct cutout_piece
{
	struct point
		corners[CUTOUT_MAX_CORNERS];
	unsigned char
		count;
};

struct cutout
{
	struct cutout_piece
		pieces[CUTOUT_MAX_PIECES];
	unsigned char
		count;
};

struct regpoly
{
	unsigned char
		ir,
		or,
		pts;
};

struct sphere
{
	unsigned char
		radius;
};

struct cube
{
	unsigned char
		edgelen;
};

struct obj_shape
{
	VECTOR3
		normal,
		x, y;
	unsigned short
		scale;
	unsigned char
		rgba[4];
	enum shape_type
	{
		SHAPE_NONE = 0,
		SHAPE_CUTOUT,
		SHAPE_REGPOLY,
		SHAPE_SPHERE,
		SHAPE_CUBE,
	} type;
	union shape_data
	{
		struct cutout cutout;
		struct regpoly regpoly;
		struct sphere sphere;
		struct cube cube;
	} data;
};

static void shape_gen_axes (const VECTOR3 * n, VECTOR3 * x, VECTOR3 * y);

static void shape_int_draw_circle (const VECTOR3 * render, unsigned short r, unsigned char pts, const VECTOR3 * x, const VECTOR3 * y);
static void shape_int_draw_disc (const VECTOR3 * render, unsigned short ir, unsigned short or, unsigned char pts, const VECTOR3 * x, const VECTOR3 * y);
static void shape_int_draw_cube (const VECTOR3 * render, unsigned short size, const VECTOR3 * up, const VECTOR3 * side, const VECTOR3 * front);
static void shape_int_draw_cutout (const VECTOR3 * render, const struct cutout * c, unsigned short size, const VECTOR3 * x, const VECTOR3 * y);


void shape_setColor (SHAPE s, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	assert (s != NULL);
	s->rgba[0] = r;
	s->rgba[1] = g;
	s->rgba[2] = b;
	s->rgba[3] = a;
}

void shape_setScale (SHAPE s, unsigned short sc)
{
	assert (s != NULL);
	s->scale = sc;
}

void shape_setNormal (SHAPE s, const VECTOR3 * n)
{
	assert (s != NULL);
	assert (n != NULL);
	if (vector_cmp (&s->normal, n))
		return;
	s->normal = *n;
	shape_gen_axes (&s->normal, &s->x, &s->y);
}


SHAPE shape_makeBlank ()
{
	SHAPE
		s = xph_alloc (sizeof (struct obj_shape));
	memset (s->rgba, 0xff, 4);
	s->type = SHAPE_NONE;
	s->scale = 128;
	s->normal = vectorCreate (0, 1, 0);
	shape_gen_axes (&s->normal, &s->x, &s->y);
	return s;
}

SHAPE shape_makeFilledPoly (unsigned char sides)
{
	SHAPE
		s = shape_makeBlank ();
	s->type = SHAPE_REGPOLY;
	s->data.regpoly.ir = 0;
	s->data.regpoly.or = 63;
	s->data.regpoly.pts = sides;
	return s;
}

SHAPE shape_makeHollowPoly (unsigned char sides, unsigned char thickness)
{
	SHAPE
		s = shape_makeBlank ();
	s->type = SHAPE_REGPOLY;
	s->data.regpoly.ir = 63 - (thickness >> 2);
	s->data.regpoly.or = 63;
	s->data.regpoly.pts = sides;
	return s;
}

SHAPE shape_makeCube (unsigned char size)
{
	SHAPE
		s = shape_makeBlank ();
	s->type = SHAPE_CUBE;
	s->data.cube.edgelen = size;
	return s;
}

SHAPE shape_makeCross ()
{
	SHAPE
		s = shape_makeBlank ();
	s->type = SHAPE_CUTOUT;
	s->data.cutout.count = 3;
	s->data.cutout.pieces[0].count = 4;

	s->data.cutout.pieces[0].corners[0].x = -3;
	s->data.cutout.pieces[0].corners[0].y = 20;
	s->data.cutout.pieces[0].corners[1].x = -3;
	s->data.cutout.pieces[0].corners[1].y = -20;
	s->data.cutout.pieces[0].corners[2].x = 3;
	s->data.cutout.pieces[0].corners[2].y = -20;
	s->data.cutout.pieces[0].corners[3].x = 3;
	s->data.cutout.pieces[0].corners[3].y = 20;

	s->data.cutout.pieces[1].count = 4;
	s->data.cutout.pieces[1].corners[0].x = 5;
	s->data.cutout.pieces[1].corners[0].y = 3;
	s->data.cutout.pieces[1].corners[1].x = 5;
	s->data.cutout.pieces[1].corners[1].y = -3;
	s->data.cutout.pieces[1].corners[2].x = 20;
	s->data.cutout.pieces[1].corners[2].y = -3;
	s->data.cutout.pieces[1].corners[3].x = 20;
	s->data.cutout.pieces[1].corners[3].y = 3;

	s->data.cutout.pieces[2].count = 4;
	s->data.cutout.pieces[2].corners[0].x = -20;
	s->data.cutout.pieces[2].corners[0].y = 3;
	s->data.cutout.pieces[2].corners[1].x = -20;
	s->data.cutout.pieces[2].corners[1].y = -3;
	s->data.cutout.pieces[2].corners[2].x = -5;
	s->data.cutout.pieces[2].corners[2].y = -3;
	s->data.cutout.pieces[2].corners[3].x = -5;
	s->data.cutout.pieces[2].corners[3].y = 3;
	return s;
}

void shape_destroy (SHAPE s)
{
	xph_free (s);
}



void shape_draw (const SHAPE s, const VECTOR3 * offset)
{
	assert (s != NULL);
	glColor4ub (s->rgba[0], s->rgba[1], s->rgba[2], s->rgba[3]);
	glBindTexture (GL_TEXTURE_2D, 0);
	switch (s->type)
	{
		case SHAPE_CUTOUT:
			shape_int_draw_cutout (offset, &s->data.cutout, s->scale, &s->x, &s->y);
			break;
		case SHAPE_REGPOLY:
			if (s->data.regpoly.ir == 0)
				shape_int_draw_circle (offset, s->data.regpoly.or * s->scale, s->data.regpoly.pts, &s->x, &s->y);
			else
				shape_int_draw_disc (offset, s->data.regpoly.ir * s->scale, s->data.regpoly.or * s->scale, s->data.regpoly.pts, &s->x, &s->y);
			break;
		case SHAPE_SPHERE:
			// the idea for spheres is to billboard a circle towards the camera, and maybe ideally do some depth buffer fiddling to make it clip correctly. if that's even possible.
			break;
		case SHAPE_CUBE:
				shape_int_draw_cube (offset, s->data.cube.edgelen * s->scale, &s->normal, &s->x, &s->y);
			break;
		case SHAPE_NONE:
		default:
			break;
	}
}




/***
 * V. BASIC INTERNAL SHAPE FUNCTIONS
 */

static void shape_gen_axes (const VECTOR3 * n, VECTOR3 * x, VECTOR3 * y)
{
	VECTOR3
		m = vectorCreate (1, 0, 0),
		o = vectorCreate (-1, 0, 0);
	if (x == NULL || y == NULL)
		return;
	if (vector_cmp (n, &m))
	{
		*x = vectorCreate (0, 1, 0);
		*y = vectorCreate (0, 0, 1);
	}
	else if (vector_cmp (n, &o))
	{
		*x = vectorCreate (0, 0, -1);
		*y = vectorCreate (0, -1, 0);
	}
	else
	{
		*x = vectorCross (&m, n);
		*y = vectorCross (n, x);
		*x = vectorNormalize (x);
		*y = vectorNormalize (y);
	}
}


static void shape_int_draw_circle (const VECTOR3 * render, unsigned short r, unsigned char pts, const VECTOR3 * x, const VECTOR3 * y)
{
	float
		i = 0,
		pi2 = M_PI * 2,
		step = pi2 / pts,
		c, s,
		nr;
	glBegin (GL_TRIANGLE_FAN);
	while (i < pi2)
	{
		nr = r / 255.;
		c = cos (i) * nr;
		s = sin (i) * nr;
		glVertex3f
		(
			render->x + c * x->x + s * y->x,
			render->y + c * x->y + s * y->y,
			render->z + c * x->z + s * y->z
		);
		i += step;
	}
	glEnd ();
}

static void shape_int_draw_disc (const VECTOR3 * render, unsigned short ir, unsigned short or, unsigned char pts, const VECTOR3 * x, const VECTOR3 * y)
{
	float
		i = 0,
		pi2 = M_PI * 2,
		step = pi2 / pts,
		c, s,
		nir, nor;
	glBegin (GL_TRIANGLE_STRIP);
	while (i < pi2)
	{
		nir = ir / 255.;
		nor = or / 255.;
		c = cos (i) * nir;
		s = sin (i) * nir;
		glVertex3f
		(
			render->x + c * x->x + s * y->x,
			render->y + c * x->y + s * y->y,
			render->z + c * x->z + s * y->z
		);
		c = cos (i) * nor;
		s = sin (i) * nor;
		glVertex3f
		(
			render->x + c * x->x + s * y->x,
			render->y + c * x->y + s * y->y,
			render->z + c * x->z + s * y->z
		);
		i += step;
	}
	glEnd ();
}

static void shape_int_draw_cube (const VECTOR3 * render, unsigned short size, const VECTOR3 * up, const VECTOR3 * side, const VECTOR3 * front)
{
	float
		ns = size / 511.;
	// i am so sure there's a simpler way to draw cubes.
	glBegin (GL_QUADS);
	// up face
	glVertex3f
	(
		render->x + (front->x + -side->x + up->x) * ns,
		render->y + (front->y + -side->y + up->y) * ns,
		render->z + (front->z + -side->z + up->z) * ns
	);
	glVertex3f
	(
		render->x + (-front->x + -side->x + up->x) * ns,
		render->y + (-front->y + -side->y + up->y) * ns,
		render->z + (-front->z + -side->z + up->z) * ns
	);
	glVertex3f
	(
		render->x + (-front->x + side->x + up->x) * ns,
		render->y + (-front->y + side->y + up->y) * ns,
		render->z + (-front->z + side->z + up->z) * ns
	);
	glVertex3f
	(
		render->x + (front->x + side->x + up->x) * ns,
		render->y + (front->y + side->y + up->y) * ns,
		render->z + (front->z + side->z + up->z) * ns
	);

	// anti-up face
	glVertex3f
	(
		render->x + (front->x + -side->x + -up->x) * ns,
		render->y + (front->y + -side->y + -up->y) * ns,
		render->z + (front->z + -side->z + -up->z) * ns
	);
	glVertex3f
	(
		render->x + (front->x + side->x + -up->x) * ns,
		render->y + (front->y + side->y + -up->y) * ns,
		render->z + (front->z + side->z + -up->z) * ns
	);
	glVertex3f
	(
		render->x + (-front->x + side->x + -up->x) * ns,
		render->y + (-front->y + side->y + -up->y) * ns,
		render->z + (-front->z + side->z + -up->z) * ns
	);
	glVertex3f
	(
		render->x + (-front->x + -side->x + -up->x) * ns,
		render->y + (-front->y + -side->y + -up->y) * ns,
		render->z + (-front->z + -side->z + -up->z) * ns
	);

	// side face
	glVertex3f
	(
		render->x + (front->x + side->x + up->x) * ns,
		render->y + (front->y + side->y + up->y) * ns,
		render->z + (front->z + side->z + up->z) * ns
	);
	glVertex3f
	(
		render->x + (-front->x + side->x + up->x) * ns,
		render->y + (-front->y + side->y + up->y) * ns,
		render->z + (-front->z + side->z + up->z) * ns
	);
	glVertex3f
	(
		render->x + (-front->x + side->x + -up->x) * ns,
		render->y + (-front->y + side->y + -up->y) * ns,
		render->z + (-front->z + side->z + -up->z) * ns
	);
	glVertex3f
	(
		render->x + (front->x + side->x + -up->x) * ns,
		render->y + (front->y + side->y + -up->y) * ns,
		render->z + (front->z + side->z + -up->z) * ns
	);

	// anti-side face
	glVertex3f
	(
		render->x + (front->x + -side->x + up->x) * ns,
		render->y + (front->y + -side->y + up->y) * ns,
		render->z + (front->z + -side->z + up->z) * ns
	);
	glVertex3f
	(
		render->x + (front->x + -side->x + -up->x) * ns,
		render->y + (front->y + -side->y + -up->y) * ns,
		render->z + (front->z + -side->z + -up->z) * ns
	);
	glVertex3f
	(
		render->x + (-front->x + -side->x + -up->x) * ns,
		render->y + (-front->y + -side->y + -up->y) * ns,
		render->z + (-front->z + -side->z + -up->z) * ns
	);
	glVertex3f
	(
		render->x + (-front->x + -side->x + up->x) * ns,
		render->y + (-front->y + -side->y + up->y) * ns,
		render->z + (-front->z + -side->z + up->z) * ns
	);


	// front face
	glVertex3f
	(
		render->x + (front->x + side->x + up->x) * ns,
		render->y + (front->y + side->y + up->y) * ns,
		render->z + (front->z + side->z + up->z) * ns
	);
	glVertex3f
	(
		render->x + (front->x + side->x + -up->x) * ns,
		render->y + (front->y + side->y + -up->y) * ns,
		render->z + (front->z + side->z + -up->z) * ns
	);
	glVertex3f
	(
		render->x + (front->x + -side->x + -up->x) * ns,
		render->y + (front->y + -side->y + -up->y) * ns,
		render->z + (front->z + -side->z + -up->z) * ns
	);
	glVertex3f
	(
		render->x + (front->x + -side->x + up->x) * ns,
		render->y + (front->y + -side->y + up->y) * ns,
		render->z + (front->z + -side->z + up->z) * ns
	);

	// anti-front face
	glVertex3f
	(
		render->x + (-front->x + side->x + up->x) * ns,
		render->y + (-front->y + side->y + up->y) * ns,
		render->z + (-front->z + side->z + up->z) * ns
	);
	glVertex3f
	(
		render->x + (-front->x + -side->x + up->x) * ns,
		render->y + (-front->y + -side->y + up->y) * ns,
		render->z + (-front->z + -side->z + up->z) * ns
	);
	glVertex3f
	(
		render->x + (-front->x + -side->x + -up->x) * ns,
		render->y + (-front->y + -side->y + -up->y) * ns,
		render->z + (-front->z + -side->z + -up->z) * ns
	);
	glVertex3f
	(
		render->x + (-front->x + side->x + -up->x) * ns,
		render->y + (-front->y + side->y + -up->y) * ns,
		render->z + (-front->z + side->z + -up->z) * ns
	);

	glEnd ();
}

static void shape_int_draw_cutout (const VECTOR3 * render, const struct cutout * c, unsigned short size, const VECTOR3 * x, const VECTOR3 * y)
{
	int
		i,
		j;
	float
		n = size / 255.;
	const struct point
		* p;
	while (i < c->count)
	{
		j = 0;
		glBegin (GL_POLYGON);
		while (j < c->pieces[i].count)
		{
			p = &c->pieces[i].corners[j];
			glVertex3f
			(
				render->x + (p->x * x->x + p->y * y->x) * n,
				render->y + (p->x * x->y + p->y * y->y) * n,
				render->z + (p->x * x->z + p->y * y->z) * n
			);
			j++;
		}
		glEnd ();
		i++;
	}
}
