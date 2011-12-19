#ifndef VIDEO_H
#define VIDEO_H

#include <assert.h>
#include <stdbool.h>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>

void videoInit ();
void videoDestroy ();

void videoPrerender ();
void videoPostrender ();


float video_getZnear ();
float video_getXResolution ();
float video_getYResolution ();
bool video_setScaling (float scale);
bool video_setDimensions (signed int width, signed int height);
bool video_getDimensions (unsigned int * width, unsigned int * height);

/*
 * 0,0 pixel coordinate is assumed to be at the top left of the display and
 * the opengl viewmatrix is assumed to be the identity (this inverts the y
 * axis relative to opengl)
 */
inline float video_pixelYMap (int y);
inline float video_pixelXMap (int x);

/* the difference between *Offset and *Map is that an X,Y coordinate constructed with *Map has the origin at the top left corner of the screen, while *Offset has it at the centre of the screen. Due to the way OpenGL works, it's often useful to construct a coordinate box by getting the video_pixelXMap (x), video_pixelYMap (y) for the top-left corner with *Map and then video_pixelXOffset (width), video_pixelOffsetY (height) and constructing the rest of the coordinates as {x, y}, {x + width, y}, {x + width, y + height}, {x, y + height}
 * - xph 2011 06 15 (even though this code was written long before)
 */
inline float video_pixelXOffset (signed int x);
inline float video_pixelYOffset (signed int y);

void video_orthoOff ();
void video_orthoOn ();
void video_wireframeSwitch ();

#endif /* VIDEO_H */