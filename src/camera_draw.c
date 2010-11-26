#include "camera_draw.h"

void camera_drawCursor (const Entity e)
{
	//cameraComponent
	//	cdata = component_getData (entity_getAs (e, "camera"));
	//Texture * t = camera_getCursorTexture (cdata);
	unsigned int
		height = 0,
		width = 0,
		centerHeight,
		centerWidth,
		halfTexHeight,
		halfTexWidth;
	float
		zNear,
		top, bottom,
		left, right;
/*
	if (t == NULL)
		return;
*/
	if (!video_getDimensions (&width, &height))
		return;
	zNear = video_getZnear ();
	centerHeight = height / 2;
	centerWidth = width / 2;
	halfTexHeight = 4;//texture_pxHeight (t) / 2;
	halfTexWidth = 4;//texture_pxWidth (t) / 2;
	top = video_pixelYMap (centerHeight + halfTexHeight);
	bottom = video_pixelYMap (centerHeight - halfTexHeight);
	left = video_pixelXMap (centerWidth - halfTexWidth);
	right = video_pixelXMap (centerWidth + halfTexWidth);
	//printf ("top: %7.2f; bottom: %7.2f; left: %7.2f; right: %7.2f\n", top, bottom, left, right);
	glPushMatrix ();
	glLoadIdentity ();
	glColor3f (1.0, 1.0, 1.0);
	//glBindTexture (GL_TEXTURE_2D, texture_glID (t));
	glBegin (GL_TRIANGLE_STRIP);
	glVertex3f (top, left, zNear);
	glVertex3f (bottom, left, zNear);
	glVertex3f (top, right, zNear);
	glVertex3f (bottom, right, zNear);
	glEnd ();
	glPopMatrix ();
}
