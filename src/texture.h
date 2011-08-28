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

void textureActiveStencil (TEXTURE t, unsigned char r, unsigned char g, unsigned char b);

void textureFloodFill (TEXTURE t, VECTOR3 start);

void texturePressPixel (TEXTURE t, VECTOR3 px);
void texturePressLine (TEXTURE t, VECTOR3 start, VECTOR3 end);
void texturePressHex (TEXTURE t, VECTOR3 centre, unsigned int size, float angle);

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