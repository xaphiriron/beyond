/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#include "comp_body.h"

#include "component_position.h"

static void drawBody (Entity e);

static void body_create (EntComponent comp, EntSpeech speech);
static void body_destroy (EntComponent comp, EntSpeech speech);

void body_define (EntComponent comp, EntSpeech speech)
{
	component_registerResponse ("body", "__create", body_create);
	component_registerResponse ("body", "__destroy", body_destroy);
}

static void body_create (EntComponent comp, EntSpeech speech)
{
	Body
		body = xph_alloc (sizeof (struct xph_body));
	body->height = 90.0;

	component_setData (comp, body);
}

static void body_destroy (EntComponent comp, EntSpeech speech)
{
	Body
		body = component_getData (comp);
	xph_free (body);

	component_clearData (comp);
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
	Body
		body = component_getData (entity_getAs (e, "body"));
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
	SUBHEX
		baryPlatter[3];
	float
		baryWeight[3];
	VECTOR3
		baryPoint[3];
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
		glVertex3f (render.x + c, render.y, render.z + s);
		glVertex3f (render.x + c, render.y + body->height, render.z + s);
		i++;
	}
	glColor4ub (0x99, 0x55, 0x00, 0xff);
	glVertex3f
	(
		render.x + cos (facing) * radius,
		render.y,
		render.z + sin (facing) * radius
	);
	glVertex3f
	(
		render.x + cos (facing) * radius,
		render.y + body->height,
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
		glVertex3f (render.x + c, render.y, render.z + s);
		i++;
	}
	glVertex3f
	(
		render.x + cos (facing) * radius,
		render.y,
		render.z + sin (facing) * radius
	);
	glEnd ();
	i = 6;
	glBegin (GL_TRIANGLE_FAN);
	glVertex3f (render.x + cos (facing) * radius, render.y + body->height, render.z + sin (facing) * radius);
	while (i > 0)
	{
		c = cos ((M_PI * 2 / 6) * i + facing) * radius;
		s = sin ((M_PI * 2 / 6) * i + facing) * radius;
		glVertex3f (render.x + c, render.y + body->height, render.z + s);
		i--;
	}
	glEnd ();

	glBegin (GL_LINES);
	glColor4ub (0x00, 0x99, 0xff, 0xff);
	glVertex3f
	(
		render.x - view->side.x * 52,
		body->height + (render.y - view->side.y * 52),
		render.z - view->side.z * 52
	);
	glVertex3f
	(
		render.x + view->side.x * 52,
		body->height + (render.y + view->side.y * 52),
		render.z + view->side.z * 52
	);
	glColor4ub (0x99, 0x00, 0xff, 0xff);
	glVertex3f
	(
		render.x - view->front.x * 52,
		body->height + (render.y - view->front.y * 52),
		render.z - view->front.z * 52
	);
	glVertex3f
	(
		render.x + view->front.x * 52,
		body->height + (render.y + view->front.y * 52),
		render.z + view->front.z * 52
	);
	glColor4ub (0x99, 0xff, 0x00, 0xff);
	glVertex3f
	(
		render.x - view->up.x * 52,
		body->height + (render.y - view->up.y * 52),
		render.z - view->up.z * 52
	);
	glVertex3f
	(
		render.x + view->up.x * 52,
		body->height + (render.y + view->up.y * 52),
		render.z + view->up.z * 52
	);
	glEnd ();


	position_baryPoints (e, baryPlatter, baryWeight);
	baryPoint[0] = renderOriginDistance (baryPlatter[0]);
	baryPoint[1] = renderOriginDistance (baryPlatter[1]);
	baryPoint[2] = renderOriginDistance (baryPlatter[2]);

	glBegin (GL_LINES);
	glColor4f (1.0, 0.0, 0.0, baryWeight[0]);
	glVertex3f (render.x, render.y + body->height, render.z);
	glVertex3f (baryPoint[0].x, render.y + body->height, baryPoint[0].z);

	glColor4f (1.0, 0.0, 0.0, baryWeight[1]);
	glVertex3f (render.x, render.y + body->height, render.z);
	glVertex3f (baryPoint[1].x, render.y + body->height, baryPoint[1].z);

	glColor4f (1.0, 0.0, 0.0, baryWeight[2]);
	glVertex3f (render.x, render.y + body->height, render.z);
	glVertex3f (baryPoint[2].x, render.y + body->height, baryPoint[2].z);
	glEnd ();


};

float body_height (Entity e)
{
	Body
		body = component_getData (entity_getAs (e, "body"));
	if (!body)
		return 0.0;
	return body->height;
	
}