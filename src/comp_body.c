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
	float
		c, s,
		radius = 52 / 2;
	int
		i = 0;
	render = position_renderCoords (e);

	glBindTexture (GL_TEXTURE_2D, 0);
	glBegin (GL_TRIANGLE_STRIP);
	while (i < 6)
	{
		c = cos ((M_PI * 2 / 6) * i) * radius;
		s = sin ((M_PI * 2 / 6) * i) * radius;
		if (i % 2)
			glColor4ub (0xff, 0x99, 0x00, 0xff);
		else
			glColor4ub (0x99, 0x55, 0x00, 0xff);
		glVertex3f (render.x + c, render.y - 90, render.z + s);
		glVertex3f (render.x + c, render.y, render.z + s);
		i++;
	}
	glColor4ub (0x99, 0x55, 0x00, 0xff);
	glVertex3f (render.x + radius, render.y - 90, render.z);
	glVertex3f (render.x + radius, render.y, render.z);
	glEnd ();
	i = 0;
	glBegin (GL_TRIANGLE_FAN);
	glColor4ub (0xff, 0x99, 0x00, 0xff);
	while (i < 6)
	{
		c = cos ((M_PI * 2 / 6) * i) * radius;
		s = sin ((M_PI * 2 / 6) * i) * radius;
		glVertex3f (render.x + c, render.y - 90, render.z + s);
		i++;
	}
	glVertex3f (render.x + radius, render.y - 90, render.z);
	glEnd ();
	i = 6;
	glBegin (GL_TRIANGLE_FAN);
	glVertex3f (render.x + radius, render.y, render.z);
	while (i > 0)
	{
		c = cos ((M_PI * 2 / 6) * i) * radius;
		s = sin ((M_PI * 2 / 6) * i) * radius;
		glVertex3f (render.x + c, render.y, render.z + s);
		i--;
	}
	glEnd ();
};