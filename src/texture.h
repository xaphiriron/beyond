#ifndef XPH_TEXTURE_H
#define XPH_TEXTURE_H

#include <GL/gl.h>

#include "bool.h"
#include "vector.h"

typedef struct xph_texture * TEXTURE;

/***
 * TEXTURE PAINTING
 */

void textureSetBackgroundColor (unsigned char r, unsigned char g, unsigned char b, unsigned char a);
void textureSetColor (unsigned char r, unsigned char g, unsigned char b, unsigned char a);

unsigned char * textureColorAt (TEXTURE t, VECTOR3 px);

void textureActiveStencil (TEXTURE t, unsigned char r, unsigned char g, unsigned char b);

void textureFloodFill (TEXTURE t, signed int startX, signed int startY);

void textureDrawPixel (TEXTURE t, VECTOR3 px);
void textureDrawLine (TEXTURE t, VECTOR3 start, VECTOR3 end);
void textureDrawHex (TEXTURE t, VECTOR3 centre, unsigned int size, float angle);

/* copy rect of pixels from raw->data from rawX,rawY to rawX+width,rawY+height and paste them into t from x,y to x+width,y+height */
void textureCopyChunkFromRaw (TEXTURE t, unsigned int x, unsigned int y, unsigned int width, unsigned int height, const TEXTURE raw, unsigned int rawX, unsigned int rawY);

/***
 *
 */

bool textureOOB (const TEXTURE t, VECTOR3 coord);
GLuint textureName (const TEXTURE t);

TEXTURE textureGenBlank (unsigned int width, unsigned int height, unsigned char mode);
TEXTURE textureGenFromImage (const char * path);
void textureDestroy (TEXTURE t);

GLuint textureBind (TEXTURE t);

#endif /* XPH_TEXTURE_H */