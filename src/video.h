#ifndef VIDEO_H
#define VIDEO_H

#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include "assert.h"
#include "bool.h"
#include "object.h"

extern Object * VideoObject;

typedef struct video {
  float
    near,
    far,
    resolution;
  char
    * title,
    * icon;
  unsigned int
    height,
    width;
  bool
    orthographic,
    doublebuffer;

  unsigned int SDLmode;
  SDL_Surface * screen;

	bool
		renderWireframe;

} VIDEO;

VIDEO * video_create ();
void video_destroy (VIDEO * v);

bool video_loadConfigSettings (VIDEO * v, char * configPath);
void video_loadDefaultSettings (VIDEO * v);

void video_enableSDLmodules ();
void video_enableGLmodules ();

bool video_initialize (VIDEO *);
void video_regenerateDisplay (VIDEO *);

bool video_setResolution (VIDEO * v, float x, float y);
bool video_setScaling (VIDEO * v, float scale);
float video_getZnear ();
float video_getXResolution ();
float video_getYResolution ();
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

int video_handler (Object * e, objMsg msg, void * a, void * b);

#endif /* VIDEO_H */