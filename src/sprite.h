#ifndef XPH_SPRITE_H
#define XPH_SPRITE_H

#include <stdarg.h>

#include "bool.h"
#include "texture.h"

typedef struct spriteSheet * SPRITESHEET;
typedef struct sprite * SPRITE;

enum sheetFormat {
    SHEET_CONSTANT,
    SHEET_MULTISIZE
};

SPRITE spriteCreate ();
void spriteDestroy (SPRITE sprite);


bool spriteGetFromSheetOffset (const SPRITESHEET sheet, SPRITE sprite, int offset);
bool spriteGetFromSheetCoordinate (const SPRITESHEET sheet, SPRITE sprite, int column, int row);

enum xflip {
  FLIP_X = TRUE,
  NOFLIP_X = FALSE
};

enum yflip {
  FLIP_Y = TRUE,
  NOFLIP_Y = FALSE
};

bool spriteDraw (const SPRITE sprite, float x, float y, float z, enum xflip xflip, enum yflip yflip);
void spriteGetXY (const SPRITE s, int * x, int * y);
void spriteGetWH (const SPRITE s, int * w, int * h);
void spriteGetFG (const SPRITE s, int * f, int * g);

void spriteGetXYf (const SPRITE s, float * x, float * y);
void spriteGetWHf (const SPRITE s, float * w, float * h);

/* called like:
 *  sheetCreate (path, SHEET_MULTISIZE);
 *  sheetCreate (path, SHEET_CONSTANT, width, height);
 */
SPRITESHEET sheetCreate (const char * path, enum sheetFormat format, ...);
void sheetMultisizeInitialize (SPRITESHEET s, const char * path);
void sheetConstantInitialize (SPRITESHEET s, const char * path, va_list args);
void sheetLoadSpriteData (SPRITESHEET s, const char * path);
void sheetDestroy (SPRITESHEET s);

void sheetGetTextureSize (const SPRITESHEET s, int * width, int * height);
int sheetGetGBaselineOffset (const SPRITESHEET);
int sheetGetHeightRange (const SPRITESHEET s);

SPRITE sheetGetSpriteViaOffset (const SPRITESHEET s, int offset);
SPRITE sheetGetSpriteViaCoordinate (const SPRITESHEET s, int column, int row);
bool sheetGetSpriteValuesViaOffset (const SPRITESHEET s, int offset, unsigned int *x, unsigned int *y, unsigned int *w, unsigned int *h, int *f, int *g);
bool sheetGetSpriteValuesViaCoordinate (const SPRITESHEET s, int column, int row, unsigned int *x, unsigned int *y, unsigned int *w, unsigned int *h, int *f, int *g);
bool sheetGetCoordinateFVals (const SPRITESHEET s, int column, int row, float * x, float * y, float * w, float * h);

const TEXTURE sheetGetTexture (const SPRITESHEET s);

/* code for these two functions taken almost verbatim from the billboarding
 * tutorial at:
 *  http://www.lighthouse3d.com/opengl/billboarding/index.php?billCheat
 */
void billboardBegin ();
void billboardEnd ();

#endif /* XPH_SPRITE_H */