#ifndef XPH_TEXTURE_H
#define XPH_TEXTURE_H

#include <SDL/SDL_opengl.h>
#include <GL/glpng.h>

struct texture {
  GLuint id;
  char * path;
  pngInfo * info;
  GLint
    wrap,
    minfilter,
    magfilter;
  int
    mipmap,
    trans,
    references;
};

void texturesDestroy ();

struct texture * textureLoad (const char * relPath, int mipmap, int trans, GLint wrap, GLint minfilter, GLint magfilter);
void textureUnload (struct texture * t);

#endif /* XPH_TEXTURE_H */