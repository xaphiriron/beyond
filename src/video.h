#ifndef VIDEO_H
#define VIDEO_H

#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include "assert.h"
#include "bool.h"
#include "object.h"

extern Object * VideoObject;

typedef struct video {
  unsigned int
    height,
    width;
  bool
    orthographic,
    doublebuffer;
  float resolution;
  char
    * title,
    * icon;
  float
    near,
    far;

  unsigned int SDLmode;
  SDL_Surface * screen;
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
float video_getXResolution ();
float video_getYResolution ();

int video_handler (Object * e, objMsg msg, void * a, void * b);

#endif /* VIDEO_H */