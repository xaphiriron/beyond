#include "comp_body.h"

#include "component_position.h"

static void drawBody (Entity e);

void body_define (EntComponent comp, EntSpeech speech)
{
}

void bodyRender_system (Dynarr entities)
{
	Entity
		e;
	int
		i = 0;
	while ((e = *(Entity *)dynarr_at (entities, i++)))
	{
		drawBody (e);
	}
}

static void drawBody (Entity e)
{
	VECTOR3
		render;
	const AXES
		* view;
	float
		c, s,
		facing,
		radius = 52 / 2;
	int
		i = 0;
	render = position_renderCoords (e);
	view = position_getViewAxes (e);
	facing = position_getHeading (e);

	glBindTexture (GL_TEXTURE_2D, 0);
	glBegin (GL_TRIANGLE_STRIP);
	while (i < 6)
	{
		c = cos ((M_PI * 2 / 6) * i + facing) * radius;
		s = sin ((M_PI * 2 / 6) * i + facing) * radius;
		if (i % 2)
			glColor4ub (0xff, 0x99, 0x00, 0xff);
		else
			glColor4ub (0x99, 0x55, 0x00, 0xff);
		glVertex3f (render.x + c, render.y - 90, render.z + s);
		glVertex3f (render.x + c, render.y, render.z + s);
		i++;
	}
	glColor4ub (0x99, 0x55, 0x00, 0xff);
	glVertex3f
	(
		render.x + cos (facing) * radius,
		render.y - 90,
		render.z + sin (facing) * radius
	);
	glVertex3f
	(
		render.x + cos (facing) * radius,
		render.y,
		render.z + sin (facing) * radius
	);
	glEnd ();
	i = 0;
	glBegin (GL_TRIANGLE_FAN);
	glColor4ub (0xff, 0x99, 0x00, 0xff);
	while (i < 6)
	{
		c = cos ((M_PI * 2 / 6) * i + facing) * radius;
		s = sin ((M_PI * 2 / 6) * i + facing) * radius;
		glVertex3f (render.x + c, render.y - 90, render.z + s);
		i++;
	}
	glVertex3f
	(
		render.x + cos (facing) * radius,
		render.y - 90,
		render.z + sin (facing) * radius
	);
	glEnd ();
	i = 6;
	glBegin (GL_TRIANGLE_FAN);
	glVertex3f (render.x + cos (facing) * radius, render.y, render.z + sin (facing) * radius);
	while (i > 0)
	{
		c = cos ((M_PI * 2 / 6) * i + facing) * radius;
		s = sin ((M_PI * 2 / 6) * i + facing) * radius;
		glVertex3f (render.x + c, render.y, render.z + s);
		i--;
	}
	glEnd ();

	glBegin (GL_LINES);
	glColor4ub (0x00, 0x99, 0xff, 0xff);
	glVertex3f
	(
		render.x - view->side.x * 52,
		render.y - view->side.y * 52,
		render.z - view->side.z * 52
	);
	glVertex3f
	(
		render.x + view->side.x * 52,
		render.y + view->side.y * 52,
		render.z + view->side.z * 52
	);
	glColor4ub (0x99, 0x00, 0xff, 0xff);
	glVertex3f
	(
		render.x - view->front.x * 52,
		render.y - view->front.y * 52,
		render.z - view->front.z * 52
	);
	glVertex3f
	(
		render.x + view->front.x * 52,
		render.y + view->front.y * 52,
		render.z + view->front.z * 52
	);
	glColor4ub (0x99, 0xff, 0x00, 0xff);
	glVertex3f
	(
		render.x - view->up.x * 52,
		render.y - view->up.y * 52,
		render.z - view->up.z * 52
	);
	glVertex3f
	(
		render.x + view->up.x * 52,
		render.y + view->up.y * 52,
		render.z + view->up.z * 52
	);
	glEnd ();
};