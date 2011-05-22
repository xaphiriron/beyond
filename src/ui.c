#include "ui.h"

#include "xph_memory.h"
#include "xph_log.h"
#include "video.h"

#include "entity.h"
#include "component_input.h"
#include "worldgen.h"

/*
static VECTOR3 uiTransformWorldToUI (const WORLDHEX whx);
static float
	HexAspect = 0.0;
static VECTOR3
	RedPole,
	GreenPole,
	BluePole;
*/

struct uiPanelWorldMap
{
	enum uiPanelTypes
		type;
};

union uiPanels
{
	enum uiPanelTypes
		type;
	struct uiPanelWorldMap
		worldmap;
};

UIPANEL uiCreatePanel (enum uiPanelTypes type, ...)
{
	UIPANEL
		p = xph_alloc (sizeof (union uiPanels));
	p->type = type;
	switch (type)
	{
		case UI_WORLDMAP:
			p->worldmap.type = type;
			break;
		default:
			ERROR ("Invalid panel type (%d); cannot create panel", type);
			return NULL;
	}
	return p;
}

void uiDestroyPanel (UIPANEL p)
{
	xph_free (p);
}

enum uiPanelTypes uiGetPanelType (const UIPANEL p)
{
	if (p == NULL)
		return UI_NONE;
	return p->type;
}

void uiDrawPanel (const UIPANEL p)
{
	switch (p->type)
	{
		case UI_WORLDMAP:
			uiDrawWorldmap (p);
			break;
		case UI_NONE:
		default:
			return;
	}
}

void uiDrawWorldmap (const UIPANEL p)
{
	float
		near = video_getZnear (),
		leftEdge, rightEdge, topEdge, bottomEdge/*,
		maxMagnitude*/;
	unsigned int
		width, height;
/*
	WORLDHEX
		position;
	Entity
		player;

	int
		i, j, vertices, e;

	float
		pixel8X,
		pixel8Y;

	VECTOR3
		maxCorners[6],
		uiPositionVector,
		maximal,
		* savedVertices;
	Dynarr
		edges;

*/
	if (p->type != UI_WORLDMAP)
		return;
	video_getDimensions (&width, &height);
	leftEdge = video_pixelXMap (20);
	rightEdge = video_pixelXMap (width - 20);
	topEdge = video_pixelYMap (20);
	bottomEdge = video_pixelYMap (height - 20);

/*
	pixel8X = video_pixelXOffset (8);
	pixel8Y = video_pixelYOffset (8);

	if (HexAspect == 0.0)
	{
		HexAspect = width < height
			? video_pixelXOffset ((width - 40) * .25)
			: video_pixelYOffset ((height - 40) * .28571428);
		RedPole = vectorCreate
		(
			video_pixelXMap (20) + HexAspect * 3,
			video_pixelYMap (20) - HexAspect,
			0
		);
		GreenPole = vectorCreate
		(
			video_pixelXMap (20) + HexAspect * 3.8571,
			video_pixelYMap (20) - HexAspect * 2.5,
			0
		);
		BluePole = vectorCreate
		(
			video_pixelXMap (20) + HexAspect * 2.1428,
			video_pixelYMap (20) - HexAspect * 2.5,
			0
		);
	}

	maximal = whxMaximalDistance ();
	maxMagnitude = vectorMagnitude (&maximal);

	whxGetPoleCorners (maxCorners);
	maxCorners[0] = vectorDivideByScalar (&maxCorners[0], maxMagnitude);
	maxCorners[1] = vectorDivideByScalar (&maxCorners[1], maxMagnitude);
	maxCorners[2] = vectorDivideByScalar (&maxCorners[2], maxMagnitude);
	maxCorners[3] = vectorDivideByScalar (&maxCorners[3], maxMagnitude);
	maxCorners[4] = vectorDivideByScalar (&maxCorners[4], maxMagnitude);
	maxCorners[5] = vectorDivideByScalar (&maxCorners[5], maxMagnitude);
	maxCorners[0] = vectorMultiplyByScalar (&maxCorners[0], HexAspect);
	maxCorners[1] = vectorMultiplyByScalar (&maxCorners[1], HexAspect);
	maxCorners[2] = vectorMultiplyByScalar (&maxCorners[2], HexAspect);
	maxCorners[3] = vectorMultiplyByScalar (&maxCorners[3], HexAspect);
	maxCorners[4] = vectorMultiplyByScalar (&maxCorners[4], HexAspect);
	maxCorners[5] = vectorMultiplyByScalar (&maxCorners[5], HexAspect);
*/

	glColor3f (0.8, 0.8, 0.8);
	glBegin (GL_QUADS);
	glVertex3f (leftEdge, topEdge, near);
	glVertex3f (leftEdge, bottomEdge, near);
	glVertex3f (rightEdge, bottomEdge, near);
	glVertex3f (rightEdge, topEdge, near);
	glEnd ();

/*
	glColor3f (0.8, 0.3, 0.2);
	glBegin (GL_TRIANGLE_FAN);
	glVertex3f (RedPole.x + maxCorners[0].z, RedPole.y + maxCorners[0].x, near);
	glVertex3f (RedPole.x + maxCorners[5].z, RedPole.y + maxCorners[5].x, near);
	glVertex3f (RedPole.x + maxCorners[4].z, RedPole.y + maxCorners[4].x, near);
	glVertex3f (RedPole.x + maxCorners[3].z, RedPole.y + maxCorners[3].x, near);
	glVertex3f (RedPole.x + maxCorners[2].z, RedPole.y + maxCorners[2].x, near);
	glVertex3f (RedPole.x + maxCorners[1].z, RedPole.y + maxCorners[1].x, near);
	glEnd ();

	glColor3f (0.3, 0.8, 0.2);
	glBegin (GL_TRIANGLE_FAN);
	glVertex3f (GreenPole.x + maxCorners[2].z, GreenPole.y + maxCorners[2].x, near);
	glVertex3f (GreenPole.x + maxCorners[1].z, GreenPole.y + maxCorners[1].x, near);
	glVertex3f (GreenPole.x + maxCorners[0].z, GreenPole.y + maxCorners[0].x, near);
	glVertex3f (GreenPole.x + maxCorners[5].z, GreenPole.y + maxCorners[5].x, near);
	glVertex3f (GreenPole.x + maxCorners[4].z, GreenPole.y + maxCorners[4].x, near);
	glVertex3f (GreenPole.x + maxCorners[3].z, GreenPole.y + maxCorners[3].x, near);
	glEnd ();

	glColor3f (0.2, 0.3, 0.8);
	glBegin (GL_TRIANGLE_FAN);
	glVertex3f (BluePole.x + maxCorners[4].z, BluePole.y + maxCorners[4].x, near);
	glVertex3f (BluePole.x + maxCorners[3].z, BluePole.y + maxCorners[3].x, near);
	glVertex3f (BluePole.x + maxCorners[2].z, BluePole.y + maxCorners[2].x, near);
	glVertex3f (BluePole.x + maxCorners[1].z, BluePole.y + maxCorners[1].x, near);
	glVertex3f (BluePole.x + maxCorners[0].z, BluePole.y + maxCorners[0].x, near);
	glVertex3f (BluePole.x + maxCorners[5].z, BluePole.y + maxCorners[5].x, near);
	glEnd ();

	player = input_getPlayerEntity ();
	//printf ("&position: %p\n", &position);
	entity_message (player, "getWorldPosition", &position);
	uiPositionVector = uiTransformWorldToUI (position);

	glColor3f (1.0, 1.0, 1.0);
	glBegin (GL_TRIANGLE_FAN);
	glVertex3f (
		uiPositionVector.x + pixel8X,
		uiPositionVector.y + pixel8Y,
		near
	);
	glVertex3f (
		uiPositionVector.x - pixel8X,
		uiPositionVector.y + pixel8Y,
		near
	);
	glVertex3f (
		uiPositionVector.x - pixel8X,
		uiPositionVector.y - pixel8Y,
		near
	);
	glVertex3f (
		uiPositionVector.x + pixel8X,
		uiPositionVector.y - pixel8Y,
		near
	);
	glEnd ();
	worldhexDestroy (position);

	i = 0;
	vertices = graphVertexCount (worldgenWorldGraph ());
	savedVertices = xph_alloc (sizeof (VECTOR3) * vertices);
	glColor3f (0.0, 0.0, 0.0);
	while (i < vertices)
	{
		position = vertexPosition (graphGetVertex (worldgenWorldGraph (), i));
		uiPositionVector = uiTransformWorldToUI (position);
		edges = vertexEdges (graphGetVertex (worldgenWorldGraph (), i));
		j = 0;
		while ((e = *(size_t *)dynarr_at (edges, j)) < i)
		{
			glBegin (GL_LINES);
			glVertex3f (
				uiPositionVector.x,
				uiPositionVector.y,
				near
			);
			glVertex3f (
				savedVertices[e].x,
				savedVertices[e].y,
				near
			);
			glEnd ();
			j++;
		}
		//DEBUG ("got %s, which translates to %f,%f,%f", whxPrint (position), uiPositionVector.x, uiPositionVector.y, uiPositionVector.z);
		glBegin (GL_TRIANGLE_FAN);
		glVertex3f (
			uiPositionVector.x + pixel8X,
			uiPositionVector.y + pixel8Y,
			near
		);
		glVertex3f (
			uiPositionVector.x - pixel8X,
			uiPositionVector.y + pixel8Y,
			near
		);
		glVertex3f (
			uiPositionVector.x - pixel8X,
			uiPositionVector.y - pixel8Y,
			near
		);
		glVertex3f (
			uiPositionVector.x + pixel8X,
			uiPositionVector.y - pixel8Y,
			near
		);
		savedVertices[i] = uiPositionVector;
		glEnd ();
		i++;
	}
	xph_free (savedVertices);
	
*/
}
/*
static VECTOR3 uiTransformWorldToUI (const WORLDHEX whx)
{
	static VECTOR3
		maximal;
	static float
		maxMagnitude = 0.0;
	VECTOR3
		distanceFromPole,
		* relativePole,
		uiPosition;
	unsigned char
		pole;
	if (maxMagnitude == 0.0)
	{
		DEBUG ("Initializing maximal/max magnitude", NULL);
		maximal = whxMaximalDistance ();
		maxMagnitude = vectorMagnitude (&maximal);
	}

	distanceFromPole = whxPoleDistance (whx);
	pole = whxPole (whx);
	if (pole == 'r')
		relativePole = &RedPole;
	else if (pole == 'g')
		relativePole = &GreenPole;
	else if (pole == 'b')
		relativePole = &BluePole;
	else
	{
		relativePole = &RedPole;
		ERROR ("%s, which was going to be used on the map, isn't placed or has an invalid position.", whxPrint (whx));
	}
	distanceFromPole = vectorDivideByScalar (&distanceFromPole, maxMagnitude);
	distanceFromPole = vectorMultiplyByScalar (&distanceFromPole, HexAspect);
	uiPosition = vectorCreate
	(
		relativePole->x + distanceFromPole.z,
		relativePole->y + distanceFromPole.x,
		0
	);

	return uiPosition;
}
*/