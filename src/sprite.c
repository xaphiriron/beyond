#include "sprite.h"

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>
#include <GL/glpng.h>

#include "video.h"
#include "xph_log.h"
#include "xph_memory.h"
#include "xph_path.h"
#include "dynarr.h"

#include "texture_internal.h"

struct spriteSheet
{
	TEXTURE
		texture;
	pngRawInfo
		* raw;
  /* SHEET_CONSTANT: each sprite is ->spriteHeight by ->spriteWidth pixels.
   *  There are no guidelines in the file, so the sheet must contain
   * (int)(->raw.width / spriteWidth) * (int)(->raw.height / spriteHeight)
   * sprites. The first sprite must be transparent-- this can be png
   * transparency or a stencil colour to use as transparency.
   *  ->spritesPerRow and ->sprites are NULL. Sprite data may only be
   * retrieved by the sheetGetSpriteValues* fuctions, NOT by the
   * sheetGetSprite* functions.
   * SHEET_MULTISIZE: each sprite has a guideline to denote how large it is.
   *  ->spritesPerRow[] and ->sprites[] are used to count the (potentially
   * varying) number of sprites in each row and the number of sprites in the
   * sheet. There must be a guideline legend on the top-right of the image,
   * vertically. Png transparency is allowed-- to turn it on, specify the
   * transparency colour in the legend as png transparency. Otherwise, the
   * transparent colour will be used to stencil the sprites.
   */
	enum sheetFormat
	 	format;
  int
    spriteHeight,
    spriteWidth,
    sheetColumns,
    sheetRows,
    spriteCount,
    * spritesPerRow,
    fMax, fMin, fVariance,
    gMax, gMin, gVariance,	/* the highest, lowest, and range of g values */
	maxHeightBelowG;
	Dynarr
		sprites;
};

struct sprite {
  unsigned int
   x, y,	/* px offset from top left corner of sheet */
   w, h;	/* px width and height of sprite */
  int
   f, g;	/* px horizontal and vertical offset */
  struct spriteSheet * sheet;
};

struct pngColour {
  short
   r, g, b, a;
};

static struct pngColour sheetPixelCoordinate (const SPRITESHEET s, int x, int y);
static int pngColourCmp (const struct pngColour * a, const struct pngColour * b);

struct sprite * spriteCreate () {
  struct sprite * sprite = xph_alloc (sizeof (struct sprite));
  sprite->sheet = NULL;
  return sprite;
}

void spriteDestroy (struct sprite * sprite) {
  xph_free (sprite);
}

bool spriteGetFromSheetOffset (const SPRITESHEET sheet, struct sprite * sprite, int offset) {
  bool r = sheetGetSpriteValuesViaOffset (sheet, offset, &sprite->x, &sprite->y, &sprite->w, &sprite->h, &sprite->f, &sprite->g);
  if (r == true)
    sprite->sheet = (struct spriteSheet *)sheet;
  return r;
}

bool spriteGetFromSheetCoordinate (const SPRITESHEET sheet, SPRITE sprite, int column, int row) {
  bool r = sheetGetSpriteValuesViaCoordinate (sheet, column, row, &sprite->x, &sprite->y, &sprite->w, &sprite->h, &sprite->f, &sprite->g);
  if (r == true)
    sprite->sheet = (struct spriteSheet *)sheet;
  return r;
}

bool spriteDraw (const SPRITE sprite, float x, float y, float z, enum xflip xflip, enum yflip yflip) {
  float
   cameraMatrix[16],
   tx = 0, ty = 0,
   tw = 0, th = 0,
   temp = 0,
   glX = x,
   glY = y,
   glZ = z;
  assert (sprite->sheet != NULL);
  tx = (float)sprite->x / sprite->sheet->raw->Width;
  ty = (float)sprite->y / sprite->sheet->raw->Height;
  tw = (float)(sprite->x + sprite->w) / sprite->sheet->raw->Width;
  th = (float)(sprite->y + sprite->h) / sprite->sheet->raw->Height;
  if (xflip == FLIP_X) {
    temp = tw;
    tw = tx;
    tx = temp;
    temp = 0;
  }
  if (yflip == FLIP_Y) {
    temp = th;
    th = ty;
    ty = temp;
    temp = 0;
  }
  glGetFloatv (GL_MODELVIEW_MATRIX, cameraMatrix);
  glX = x * cameraMatrix[0] +
        y * cameraMatrix[4] +
        (z - video_getZnear ()) * cameraMatrix[8];
  glY = x * cameraMatrix[1] +
        y * cameraMatrix[5] +
        (z - video_getZnear ()) * cameraMatrix[9];
  glZ = x * cameraMatrix[2] +
        y * cameraMatrix[6] +
        (z - video_getZnear ()) * cameraMatrix[10];
/*
  logLine (E_DEBUG, "passed: %f, %f, %f; real: %f, %f, %f", x, y, z, glX, glY, glZ);
*/
  billboardBegin ();
  glColor3f (1.0, 1.0, 1.0);
  glBindTexture (GL_TEXTURE_2D, sprite->sheet->texture->name);
  glBegin (GL_QUADS);
  glTexCoord2f (tx, ty);
  glVertex3f (
   glX - sprite->f,
   glY + sprite->g,
   glZ
  );
  glTexCoord2f (tx, th);
  glVertex3f (
   glX - sprite->f,
   glY + sprite->g - sprite->h,
   glZ
  );
  glTexCoord2f (tw, th);
  glVertex3f (
   glX - sprite->f + sprite->w,
   glY + sprite->g - sprite->h,
   glZ
  );
  glTexCoord2f (tw, ty);
  glVertex3f (
   glX - sprite->f + sprite->w,
   glY + sprite->g,
   glZ
  );
  glEnd ();
  glBindTexture (GL_TEXTURE_2D, 0);
  billboardEnd ();
  return true;
}

void spriteGetXY (const SPRITE s, int * x, int * y)
{
	assert (s != NULL);
	assert (x && y);
	*x = s->x;
	*y = s->y;
}
void spriteGetWH (const SPRITE s, int * w, int * h)
{
	assert (s != NULL);
	assert (w && h);
	*w = s->w;
	*h = s->h;
}
void spriteGetFG (const SPRITE s, int * f, int * g)
{
	assert (s != NULL);
	assert (f && g);
	*f = s->f;
	*g = s->g;
}

void spriteGetXYf (const SPRITE s, float * x, float * y)
{
	assert (s != NULL);
	if (x != NULL)
		*x = (float)s->x / s->sheet->raw->Width;
	if (y != NULL)
		*y = (float)s->y / s->sheet->raw->Height;
}

void spriteGetWHf (const SPRITE s, float * w, float * h)
{
	assert (s != NULL);
	if (w != NULL)
		*w = (float)s->w / s->sheet->raw->Width;
	if (h != NULL)
		*h = (float)s->h / s->sheet->raw->Height;
}

struct spriteSheet * sheetCreate (const char * path, enum sheetFormat format, ...) {
	struct spriteSheet
		* s;
	char
		* absPath = NULL;
	va_list
		args;
	if (format != SHEET_CONSTANT && format != SHEET_MULTISIZE)
	{
		ERROR ("%s: invalid format arg (%d); unable to create sheet.", __FUNCTION__, format);
		return NULL;
	}
	s = xph_alloc (sizeof (struct spriteSheet));
	memset (s, 0, sizeof (struct spriteSheet));
  s->raw = xph_alloc (sizeof (pngRawInfo));
  absPath = absolutePath (path);
  if (!pngLoadRaw (absPath, s->raw)) {
    ERROR ("%s: some issue with loading raw texture data of \"%s\"", __FUNCTION__, absPath);
    xph_free (s->raw);
    xph_free (s);
    return NULL;
  }
  if (SHEET_MULTISIZE == format) {
    s->format = SHEET_MULTISIZE;
    sheetMultisizeInitialize (s, path);
  } else {
    s->format = SHEET_CONSTANT;
    va_start (args, format);
    sheetConstantInitialize (s, path, args);
    va_end (args);
  }
  /* free ->raw palette & data */
  if (s->raw->Palette != NULL) {
    free (s->raw->Palette);
    s->raw->Palette = NULL;
  }
  free (s->raw->Data);
  s->raw->Data = NULL;
  return s;
}

void sheetMultisizeInitialize (SPRITESHEET s, const char * path) {
  /*  it is unwise to use ->sheetColumns for a multisize sheet-- that's what
   * ->spritesPerRow is for.
   */
  s->sheetColumns = 0;
  s->spriteHeight = 0;
  s->spriteWidth = 0;
  s->sprites = dynarr_create (4, sizeof (struct sprite *));
  sheetLoadSpriteData (s, path);
  return;
}

void sheetConstantInitialize (SPRITESHEET s, const char * path, va_list args) {
  struct pngColour px;
  s->spriteWidth = (unsigned short)va_arg (args, int);
  s->spriteHeight = (unsigned short)va_arg (args, int);
  s->sheetColumns = (s->raw->Width / s->spriteWidth);
  s->sheetRows = (s->raw->Height / s->spriteHeight);
  s->spriteCount = s->sheetColumns * s->sheetRows;
  s->spritesPerRow = NULL;
  s->sprites = NULL;
  s->fMax = s->fMin = s->fVariance = 0;
  s->gMax = s->gMin = s->gVariance = 0;
	s->maxHeightBelowG = 0;
  px = sheetPixelCoordinate (s, 0, 0);
	if (px.a == 0)
	{
		// this should load the image as alpha-transparent
		s->texture = textureGenFromImage (path);
	}
	else
	{
		// this should load the image with px.r, px.g, px.b as alpha 0 and everything else as alpha 255
		s->texture = textureGenFromImage (path);
		textureActiveStencil (s->texture, px.r, px.g, px.b);
	}
	textureBind (s->texture);
}

/*
 * this does not set ->spritesPerRow.
 * this does not set ->fMax, ->fMin, or ->fVariance
 * this does not set ->gMax, ->gMin, or ->gVariance
 * it should do all of those!
 */
/* what yes it does set the ->f* and ->g* values; did you mean it doesn't set them /correctly/??? (i have no clue if they're 'correct' or what because i have no real clue what they're supposed to represent)
 *  - xph 2011 06 15
 */
void sheetLoadSpriteData (SPRITESHEET s, const char * path) {
  enum sheet_states {
    SEEKING_NEXT_HORIZ_CORNER,
    SEEKING_HORIZ_GUIDELINE,
    SEEKING_HORIZ_GUIDELINE_WITH_EXT_BALANCE,
    READING_HORIZ_GUIDELINE_WITH_EXT_BALANCE,
    READING_HORIZ_GUIDELINE_WITH_INT_BALANCE,
    READING_HORIZ_GUIDELINE,
    SEEKING_HORIZ_CLOSING_EXT_BALANCE,
    SEEKING_NEXT_VERT_CORNER,
    SEEKING_VERT_GUIDELINE,
    SEEKING_VERT_GUIDELINE_WITH_EXT_BALANCE,
    READING_VERT_GUIDELINE_WITH_EXT_BALANCE,
    READING_VERT_GUIDELINE_WITH_INT_BALANCE,
    READING_VERT_GUIDELINE,
    SEEKING_VERT_CLOSING_EXT_BALANCE,
    SCAN_COMPLETE
  } state = SEEKING_NEXT_HORIZ_CORNER;
  enum special_types {
    SPEC_EDGE,
    SPEC_CORNER,
    SPEC_TRANSPARENT,
    SPEC_GUIDE,
    SPEC_INT_BALANCE,
    SPEC_EXT_BALANCE,
    SPEC_OOB
  };
  struct grid {
    int
     x, y;
  } lastCorner, pos, move;
  struct pngColour
   c,
   spec[7];
  struct sprite * sprite = NULL;
  int
   balance = 0,
   i = 0;

	FUNCOPEN ();

  while (i < 6) {
    spec[i] = sheetPixelCoordinate (s, s->raw->Width-1, i);
    i++;
  }
	DEBUG ("loaded special pixels");
  spec[SPEC_OOB].r = spec[SPEC_OOB].g = spec[SPEC_OOB].b = spec[SPEC_OOB].a = -1;
	DEBUG ("set the out of bounds color");
	if (spec[SPEC_TRANSPARENT].a == 0)
	{
		DEBUG ("loading png as alpha");
		// load as alpha transparent
		s->texture = textureGenFromImage (path);
	}
	else
	{
		DEBUG ("loading png as stenciled");
		// load as stenciled with spec[SPEC_TRANSPARENT].r, spec[SPEC_TRANSPARENT].g, spec[SPEC_TRANSPARENT].b as the stencil color
		s->texture = textureGenFromImage (path);
		textureActiveStencil (s->texture, spec[SPEC_TRANSPARENT].r, spec[SPEC_TRANSPARENT].g, spec[SPEC_TRANSPARENT].b);
	}
	textureBind (s->texture);
  s->fMax = s->gMax = INT_MIN;
  s->fMin = s->gMin = INT_MAX;
  s->maxHeightBelowG = INT_MIN;

  lastCorner.x = lastCorner.y = 0;
  pos.x = pos.y = 0;
  move.x = 1;
  move.y = 0;
	DEBUG ("starting state machine for texture at '%s'", path);
  while (state != SCAN_COMPLETE) {
    c = sheetPixelCoordinate (s, pos.x, pos.y);
/*
      logLine (E_DEBUG, "%d @ %3d, %3d w/ %3d, %3d, %3d, %3d.", state, pos.x, pos.y, c.r, c.g, c.b, c.a);
    if (s->sprites->offset > 31) {
      q = listIndex (s->sprites, 31);
      logLine (E_DEBUG, "#%3d/%p: %3d,%3d w: %3d h: %3d o: %3d,%3d", i-1, (void *)q, q->x, q->y, q->w, q->h, q->f, q->g);
    }
*/
    switch (state) {
      case SEEKING_NEXT_HORIZ_CORNER:
        if (pngColourCmp (&c, &spec[SPEC_CORNER]) == 0) {
          state = SEEKING_HORIZ_GUIDELINE;
          lastCorner = pos;
          sprite = spriteCreate ();
          dynarr_push (s->sprites, sprite);
          sprite->sheet = s;
          DEBUG ("creating sprites; @ %d, %d made %p/%d (horiz)", pos.x, pos.y, sprite, dynarr_size (s->sprites));
          move.x = 1;
          move.y = 0;
        } else if (pngColourCmp (&c, &spec[SPEC_OOB]) == 0) {
          state = SEEKING_NEXT_VERT_CORNER;
          pos.x = 0;
          move.x = 0;
          move.y = 1;
        }
        break;
      case SEEKING_HORIZ_GUIDELINE:
        if (pngColourCmp (&c, &spec[SPEC_CORNER]) == 0) {
          state = SEEKING_NEXT_HORIZ_CORNER;
          pos.x--;
        } else if (pngColourCmp (&c, &spec[SPEC_OOB]) == 0) {
          state = SEEKING_NEXT_VERT_CORNER;
          pos.x = 0;
          move.x = 0;
          move.y = 1;
        } else if (pngColourCmp (&c, &spec[SPEC_EXT_BALANCE]) == 0) {
          state = SEEKING_HORIZ_GUIDELINE_WITH_EXT_BALANCE;
          balance = pos.x;
        } else if (pngColourCmp (&c, &spec[SPEC_GUIDE]) == 0) {
          state = READING_HORIZ_GUIDELINE;
          sprite->x = pos.x;
        } else if (pngColourCmp (&c, &spec[SPEC_INT_BALANCE]) == 0) {
          state = READING_HORIZ_GUIDELINE_WITH_INT_BALANCE;
          sprite->x = pos.x;
          sprite->f = 0;
          if (s->fMax < sprite->f) {
           s->fMax = sprite->f;
          }
          if (s->fMin > sprite->f) {
           s->fMin = sprite->f;
          }
        }
        break;
      case SEEKING_HORIZ_GUIDELINE_WITH_EXT_BALANCE:
        if (pngColourCmp (&c, &spec[SPEC_CORNER]) == 0) {
          state = SEEKING_NEXT_HORIZ_CORNER;
          sprite->w = sprite->h = 0;
          pos.x--;
        } else if (pngColourCmp (&c, &spec[SPEC_OOB]) == 0) {
          state = SEEKING_NEXT_VERT_CORNER;
          pos.x = 0;
          move.x = 0;
          move.y = 1;
        } else if (pngColourCmp (&c, &spec[SPEC_EXT_BALANCE]) == 0) {
          balance = pos.x;
        } else if (pngColourCmp (&c, &spec[SPEC_GUIDE]) == 0) {
          state = READING_HORIZ_GUIDELINE_WITH_EXT_BALANCE;
          sprite->x = pos.x;
          sprite->f = balance - sprite->x;
          if (s->fMax < sprite->f) {
           s->fMax = sprite->f;
          }
          if (s->fMin > sprite->f) {
           s->fMin = sprite->f;
          }
        } else if (pngColourCmp (&c, &spec[SPEC_INT_BALANCE]) == 0) {
          state = READING_HORIZ_GUIDELINE_WITH_INT_BALANCE;
          sprite->x = pos.x;
          sprite->f = 0;
          if (s->fMax < sprite->f) {
           s->fMax = sprite->f;
          }
          if (s->fMin > sprite->f) {
           s->fMin = sprite->f;
          }
        }
        break;
      case READING_HORIZ_GUIDELINE_WITH_EXT_BALANCE:
        if (pngColourCmp (&c, &spec[SPEC_INT_BALANCE]) == 0) {
          sprite->f = pos.x - sprite->x;
          if (s->fMax < sprite->f) {
           s->fMax = sprite->f;
          }
          if (s->fMin > sprite->f) {
           s->fMin = sprite->f;
          }
        } else if (pngColourCmp (&c, &spec[SPEC_GUIDE]) != 0) {
          state = SEEKING_VERT_GUIDELINE;
          sprite->w = pos.x - sprite->x;
          pos = lastCorner;
          move.x = 0;
          move.y = 1;
        }
        break;
      case READING_HORIZ_GUIDELINE_WITH_INT_BALANCE:
        if (pngColourCmp (&c, &spec[SPEC_GUIDE]) != 0) {
          state = SEEKING_VERT_GUIDELINE;
          sprite->w = pos.x - sprite->x;
          pos = lastCorner;
          move.x = 0;
          move.y = 1;
        }
        break;
      case READING_HORIZ_GUIDELINE:
        if (pngColourCmp (&c, &spec[SPEC_CORNER]) == 0 ||
            pngColourCmp (&c, &spec[SPEC_OOB]) == 0) {
          state = SEEKING_VERT_GUIDELINE;
          sprite->w = pos.x - sprite->x;
          sprite->f = sprite->w / 2;
          pos = lastCorner;
          move.x = 0;
          move.y = 1;
        } else if (pngColourCmp (&c, &spec[SPEC_INT_BALANCE]) == 0) {
          state = READING_HORIZ_GUIDELINE_WITH_INT_BALANCE;
          sprite->f = pos.x - sprite->x;
          if (s->fMax < sprite->f) {
           s->fMax = sprite->f;
          }
          if (s->fMin > sprite->f) {
           s->fMin = sprite->f;
          }
        } else if (pngColourCmp (&c, &spec[SPEC_EXT_BALANCE]) == 0) {
          state = SEEKING_VERT_GUIDELINE;
          sprite->w = pos.x - sprite->x;
          sprite->f = sprite->w;
          pos = lastCorner;
          move.x = 0;
          move.y = 1;
          if (s->fMax < sprite->f) {
           s->fMax = sprite->f;
          }
          if (s->fMin > sprite->f) {
           s->fMin = sprite->f;
          }
        } else if (pngColourCmp (&c, &spec[SPEC_GUIDE]) != 0) {
          state = SEEKING_HORIZ_CLOSING_EXT_BALANCE;
          sprite->w = pos.x - sprite->x;
        }
        break;
      case SEEKING_HORIZ_CLOSING_EXT_BALANCE:
        if (pngColourCmp (&c, &spec[SPEC_EXT_BALANCE]) == 0) {
          state = SEEKING_VERT_GUIDELINE;
          sprite->f = pos.x - sprite->x;
          if (s->fMax < sprite->f) {
           s->fMax = sprite->f;
          }
          if (s->fMin > sprite->f) {
           s->fMin = sprite->f;
          }
        } else if (pngColourCmp (&c, &spec[SPEC_GUIDE]) != 0) {
          state = SEEKING_VERT_GUIDELINE;
          sprite->f = sprite->w / 2;
          if (s->fMax < sprite->f) {
           s->fMax = sprite->f;
          }
          if (s->fMin > sprite->f) {
           s->fMin = sprite->f;
          }
          pos = lastCorner;
          move.x = 0;
          move.y = 1;
        }
        break;
      case SEEKING_NEXT_VERT_CORNER:
        if (pngColourCmp (&c, &spec[SPEC_CORNER]) == 0) {
          state = SEEKING_HORIZ_GUIDELINE;
          lastCorner = pos;
          sprite = spriteCreate ();
          dynarr_push (s->sprites, sprite);
          sprite->sheet = s;
          DEBUG ("creating sprites; @ %d, %d made %p/%d (vert)", pos.x, pos.y, sprite, dynarr_size (s->sprites));
          move.x = 1;
          move.y = 0;
        } else if (pngColourCmp (&c, &spec[SPEC_OOB]) == 0) {
          state = SCAN_COMPLETE;
          move.x = 0;
          move.y = 0;
        }
        break;
      case SEEKING_VERT_GUIDELINE:
        if (pngColourCmp (&c, &spec[SPEC_CORNER]) == 0) {
          state = SEEKING_NEXT_HORIZ_CORNER;
          sprite->w = sprite->h = 0;
          pos.x--;
          move.x = 1;
          move.y = 0;
        } else if (pngColourCmp (&c, &spec[SPEC_OOB]) == 0) {
          state = SEEKING_NEXT_VERT_CORNER;
          pos.x = 0;
        } else if (pngColourCmp (&c, &spec[SPEC_EXT_BALANCE]) == 0) {
          state = SEEKING_VERT_GUIDELINE_WITH_EXT_BALANCE;
          balance = pos.y;
        } else if (pngColourCmp (&c, &spec[SPEC_GUIDE]) == 0) {
          state = READING_VERT_GUIDELINE;
          sprite->y = pos.y;
        } else if (pngColourCmp (&c, &spec[SPEC_INT_BALANCE]) == 0) {
          state = READING_VERT_GUIDELINE_WITH_INT_BALANCE;
          sprite->y = pos.y;
          sprite->g = 0;
          if (s->gMax < sprite->g) {
           s->gMax = sprite->g;
          }
          if (s->gMin > sprite->g) {
           s->gMin = sprite->g;
          }
        }
        break;
      case SEEKING_VERT_GUIDELINE_WITH_EXT_BALANCE:
        if (pngColourCmp (&c, &spec[SPEC_CORNER]) == 0) {
          state = SEEKING_NEXT_HORIZ_CORNER;
          sprite->w = sprite->h = 0;
          pos.x--;
          move.x = 1;
          move.y = 0;
        } else if (pngColourCmp (&c, &spec[SPEC_OOB]) == 0) {
          state = SEEKING_NEXT_VERT_CORNER;
          pos.x = 0;
        } else if (pngColourCmp (&c, &spec[SPEC_EXT_BALANCE]) == 0) {
          balance = pos.y;
        } else if (pngColourCmp (&c, &spec[SPEC_GUIDE]) == 0) {
          state = READING_VERT_GUIDELINE_WITH_EXT_BALANCE;
          sprite->y = pos.y;
          sprite->g = balance - sprite->y;
          if (s->gMax < sprite->g) {
           s->gMax = sprite->g;
          }
          if (s->gMin > sprite->g) {
           s->gMin = sprite->g;
          }
        } else if (pngColourCmp (&c, &spec[SPEC_INT_BALANCE]) == 0) {
          state = READING_VERT_GUIDELINE_WITH_INT_BALANCE;
          sprite->y = pos.y;
          sprite->g = 0;
          if (s->gMax < sprite->g) {
           s->gMax = sprite->g;
          }
          if (s->gMin > sprite->g) {
           s->gMin = sprite->g;
          }
        }
        break;
      case READING_VERT_GUIDELINE_WITH_EXT_BALANCE:
        if (pngColourCmp (&c, &spec[SPEC_INT_BALANCE]) == 0) {
          sprite->g = pos.y - sprite->y;
          if (s->gMax < sprite->g) {
           s->gMax = sprite->g;
          }
          if (s->gMin > sprite->g) {
           s->gMin = sprite->g;
          }
        } else if (pngColourCmp (&c, &spec[SPEC_GUIDE]) != 0) {
          state = SEEKING_NEXT_HORIZ_CORNER;
          sprite->h = pos.y - sprite->y;
          if ((signed int)(sprite->h - sprite->g) > s->maxHeightBelowG) {
            s->maxHeightBelowG = sprite->h - sprite->g;
          }
          pos = lastCorner;
          move.x = 1;
          move.y = 0;
        }
        break;
      case READING_VERT_GUIDELINE_WITH_INT_BALANCE:
        if (pngColourCmp (&c, &spec[SPEC_GUIDE]) != 0) {
          state = SEEKING_NEXT_HORIZ_CORNER;
          sprite->h = pos.y - sprite->y;
          if ((signed int)(sprite->h - sprite->g) > s->maxHeightBelowG) {
            s->maxHeightBelowG = sprite->h - sprite->g;
          }
          pos = lastCorner;
          move.x = 1;
          move.y = 0;
        }
        break;
      case READING_VERT_GUIDELINE:
        if (pngColourCmp (&c, &spec[SPEC_CORNER]) == 0 ||
            pngColourCmp (&c, &spec[SPEC_OOB]) == 0) {
          state = SEEKING_NEXT_HORIZ_CORNER;
          sprite->h = pos.y - sprite->y;
          sprite->g = sprite->h / 2;
          if ((signed int)(sprite->h - sprite->g) > s->maxHeightBelowG) {
            s->maxHeightBelowG = sprite->h - sprite->g;
          }
          pos = lastCorner;
          move.x = 1;
          move.y = 0;
        } else if (pngColourCmp (&c, &spec[SPEC_INT_BALANCE]) == 0) {
          state = READING_VERT_GUIDELINE_WITH_INT_BALANCE;
          sprite->g = pos.y - sprite->y;
          if (s->gMax < sprite->g) {
           s->gMax = sprite->g;
          }
          if (s->gMin > sprite->g) {
           s->gMin = sprite->g;
          }
        } else if (pngColourCmp (&c, &spec[SPEC_EXT_BALANCE]) == 0) {
          state = SEEKING_NEXT_HORIZ_CORNER;
          sprite->h = pos.y - sprite->y;
          sprite->g = sprite->h;
          if (s->gMax < sprite->g) {
           s->gMax = sprite->g;
          }
          if (s->gMin > sprite->g) {
           s->gMin = sprite->g;
          }
          if ((signed int)(sprite->h - sprite->g) > s->maxHeightBelowG) {
            s->maxHeightBelowG = sprite->h - sprite->g;
          }
          pos = lastCorner;
          move.x = 1;
          move.y = 0;
        } else if (pngColourCmp (&c, &spec[SPEC_GUIDE]) != 0) {
          state = SEEKING_VERT_CLOSING_EXT_BALANCE;
          sprite->h = pos.y - sprite->y;
/*
          logLine (E_DEBUG, "seeking closing ext. balance on sprite %d", s->sprites->offset);
*/
        }
        break;
      case SEEKING_VERT_CLOSING_EXT_BALANCE:
        if (pngColourCmp (&c, &spec[SPEC_EXT_BALANCE]) == 0) {
          state = SEEKING_NEXT_HORIZ_CORNER;
          sprite->g = pos.y - sprite->y;
          if (s->gMax < sprite->g) {
           s->gMax = sprite->g;
          }
          if (s->gMin > sprite->g) {
           s->gMin = sprite->g;
          }
          if ((signed int)(sprite->h - sprite->g) > s->maxHeightBelowG) {
            s->maxHeightBelowG = sprite->h - sprite->g;
          }
/*
          logLine (E_DEBUG, "reached ext. balance on sprite %d: set to %d; %d - %d", s->sprites->offset, sprite->g, pos.y, sprite->y);
*/
          pos = lastCorner;
          move.x = 1;
          move.y = 0;
        } else if (pngColourCmp (&c, &spec[SPEC_EDGE]) != 0) {
          state = SEEKING_NEXT_HORIZ_CORNER;
          sprite->g = sprite->h / 2;
          if (s->gMax < sprite->g) {
           s->gMax = sprite->g;
          }
          if (s->gMin > sprite->g) {
           s->gMin = sprite->g;
          }
          if ((signed int)(sprite->h - sprite->g) > s->maxHeightBelowG) {
            s->maxHeightBelowG = sprite->h - sprite->g;
          }
          pos = lastCorner;
          move.x = 1;
          move.y = 0;
/*
          logLine (E_DEBUG, "giving up on finding balance with sprite %d, hit %d, %d, %d", s->sprites->offset, c.r, c.g, c.b);
*/
        }
        break;
      case SCAN_COMPLETE:
      default:
        break;
    }
    pos.x += move.x;
    pos.y += move.y;
  }
  s->fVariance = s->fMax - s->fMin;
  s->gVariance = s->gMax - s->gMin;
	s->spriteCount = dynarr_size (s->sprites);
	FUNCCLOSE ();
}

void sheetDestroy (struct spriteSheet * sheet)
{
	struct sprite
		* sprite = NULL;
	if (sheet->raw != NULL)
	{
		if (sheet->raw->Palette)
			free (sheet->raw->Palette);
		if (sheet->raw->Data != NULL)
			free (sheet->raw->Data);
		xph_free (sheet->raw);
	}
	if (sheet->format == SHEET_MULTISIZE)
	{
		while ((sprite = *(SPRITE *)dynarr_pop (sheet->sprites)) != NULL)
		{
			spriteDestroy (sprite);
		}
		dynarr_destroy (sheet->sprites);
		if (sheet->spritesPerRow)
			xph_free (sheet->spritesPerRow);
	}
	textureDestroy (sheet->texture);
	xph_free (sheet);
}

void sheetGetTextureSize (const SPRITESHEET s, int * width, int * height)
{
	assert (s != NULL);
	assert (width && height);
	*width = s->raw->Width;
	*height = s->raw->Height;
}

int sheetGetGBaselineOffset (const SPRITESHEET s)
{
	assert (s != NULL);
	return s->gMax;
}

int sheetGetHeightRange (const SPRITESHEET s)
{
	return s->gMax + s->maxHeightBelowG;
}

SPRITE sheetGetSpriteViaOffset (const SPRITESHEET s, int offset) {
  if (s->format != SHEET_MULTISIZE) {
    ERROR ("Invalid call to sheetGetSprite*: cannot get a pointer from a constant sheet (%p).", s);
    return NULL;
  } else if (offset < 0 || offset > s->spriteCount) {
    ERROR ("Attempting to get sprite (%d) outside of sheet (%p) boundaries (%d).", offset, s, s->spriteCount);
    return NULL;
  }
  return *(SPRITE *)dynarr_at (s->sprites, offset);
}

SPRITE sheetGetSpriteViaCoordinate (const SPRITESHEET s, int column, int row) {
  int
   r = 0,
   t = 0;
  if (s->format != SHEET_MULTISIZE) {
    WARNING ("Invalid call to sheetGetSprite*: cannot get a pointer from a constant sheet (%p).", s);
    return NULL;
  } else if (row < 0 || row > s->sheetRows ||
             column < 0 || column >= s->spritesPerRow[row]) {
    WARNING ("Attempting to get sprite (%d, %d) outside of sheet (%p) boundaries.", column, row, s);
    return NULL;
  }
  while (r < row) {
    t += s->spritesPerRow[row++];
  }
  t += column;
  return *(SPRITE *)dynarr_at (s->sprites, t);
}

bool sheetGetSpriteValuesViaOffset (const SPRITESHEET s, int offset, unsigned int *x, unsigned int *y, unsigned int *w, unsigned int *h, int *f, int *g) {
  int
    tx = 0,
    ty = 0;
  struct sprite * p = NULL;
  if (offset < 0 || offset > s->spriteCount) {
    return false;
  }
  if (SHEET_MULTISIZE == s->format) {
    p = *(SPRITE *)dynarr_at (s->sprites, offset);
    *x = p->x;
    *y = p->y;
    *w = p->w;
    *h = p->h;
    *f = p->f;
    *g = p->g;
  } else if (SHEET_CONSTANT == s->format) {
    *w = s->spriteWidth;
    *h = s->spriteHeight;
    *f = 0;
    *g = 0;
    tx = offset;
    while ((tx * s->spriteWidth) > s->raw->Width) {
      tx -= s->sheetRows;
      ty++;
    }
    *x = tx * s->spriteWidth;
    *y = ty * s->spriteHeight;
  }
  return true;
}

bool sheetGetSpriteValuesViaCoordinate (const SPRITESHEET s, int column, int row, unsigned int *x, unsigned int *y, unsigned int *w, unsigned int *h, int *f, int *g) {
  int
   r = 0,
   t = 0;
  struct sprite * sp = NULL;
  if (row < 0 || row > s->sheetRows || column < 0 ||
      (s->format == SHEET_CONSTANT && column >= s->sheetColumns) ||
      (s->format == SHEET_MULTISIZE && column >= s->spritesPerRow[row])
     ) {
    return false;
  }
  if (SHEET_MULTISIZE == s->format) {
    while (r < row) {
      t += s->spritesPerRow[r++];
    }
    t += column;
    sp = *(SPRITE *)dynarr_at (s->sprites, t);
    *w = sp->w;
    *h = sp->h;
    *f = sp->f;
    *g = sp->g;
    *x = sp->x;
    *y = sp->y;
  } else if (SHEET_CONSTANT == s->format) {
    *w = s->spriteWidth;
    *h = s->spriteHeight;
    *f = 0;
    *g = 0;
    *x = column * s->spriteWidth;
    *y = row * s->spriteHeight;
  }
  return true;
}

bool sheetGetCoordinateFVals (const SPRITESHEET s, int column, int row, float * x, float * y, float * w, float * h)
{
	int
		r = 0,
		t = 0;
	struct sprite
		* sp = NULL;
	if (
		row < 0 || row > s->sheetRows || column < 0 ||
		(s->format == SHEET_CONSTANT && column >= s->sheetColumns) ||
		(s->format == SHEET_MULTISIZE && column >= s->spritesPerRow[row])
	)
	{
		return false;
	}
	if (SHEET_MULTISIZE == s->format)
	{
		while (r < row) {
			t += s->spritesPerRow[r++];
		}
		t += column;
		sp = *(SPRITE *)dynarr_at (s->sprites, t);

		if (x != NULL)
			*x = (float)sp->x / s->raw->Width;
		if (y != NULL)
			*y = (float)sp->y / s->raw->Height;

		if (w != NULL)
			*w = (float)sp->w / s->raw->Width;
		if (h != NULL)
			*h = (float)sp->h / s->raw->Height;
		return true;
	}
	if (x != NULL)
		*x = (float)(column * s->spriteWidth) / s->raw->Width;
	if (y != NULL)
		*y = (float)(row * s->spriteHeight) / s->raw->Height;
	if (w != NULL)
		*w = (float)s->spriteWidth / s->raw->Width;
	if (h != NULL)
		*h = (float)s->spriteHeight / s->raw->Height;
	return true;
}

static struct pngColour sheetPixelCoordinate (const SPRITESHEET s, int x, int y) {
  struct pngColour px;
  px.r = px.g = px.b = px.a = -1;
  assert (s != NULL);
  if (s->raw == NULL || s->raw->Data == NULL) {
    ERROR ("requested pixel data (%d, %d) for a sprite sheet (%p) that doesn't have raw data.", x, y, s);
    return px;
  } else if (x < 0 || x > s->raw->Width || y < 0 || y > s->raw->Height) {
    INFO ("requested pixel data (%d, %d) outside the boundry of the sprite sheet (%p).", x, y, s);
    return px;
  }
  if (s->raw->Components == 1) {
    assert (s->raw->Palette != NULL);
    WARNING ("attempting to get pixel data (%d, %d) from a paletted .png (...). This will probably fail if the image has alpha... or if the image does not have alpha.", x, y);
    px.a = s->raw->Data[y * s->raw->Width + x];
    if (s->raw->Alpha == 0) {
      px.r = s->raw->Palette[px.a * 3 + 0];
      px.g = s->raw->Palette[px.a * 3 + 1];
      px.b = s->raw->Palette[px.a * 3 + 2];
      px.a = 255;
    } else {
      px.r = s->raw->Palette[px.a * 4 + 0];
      px.g = s->raw->Palette[px.a * 4 + 1];
      px.b = s->raw->Palette[px.a * 4 + 2];
      px.a = s->raw->Palette[px.a * 4 + 3];
   }
  } else {
    px.r = s->raw->Data[(y * s->raw->Width + x) * s->raw->Components + 0];
    px.g = s->raw->Data[(y * s->raw->Width + x) * s->raw->Components + 1];
    px.b = s->raw->Data[(y * s->raw->Width + x) * s->raw->Components + 2];
    if (s->raw->Components > 3) {
      px.a = s->raw->Data[(y * s->raw->Width + x) * s->raw->Components + 3];
    }
  }
  return px;
}

static int pngColourCmp (const struct pngColour * a, const struct pngColour * b) {
  return ((a->r << 6) + (a->g << 4) + (a->b << 2) + (a->a)) -
         ((b->r << 6) + (b->g << 4) + (b->b << 2) + (b->a));
}

const TEXTURE sheetGetTexture (const SPRITESHEET s)
{
	assert (s != NULL);
	return s->texture;
}


/* code for these two functions taken almost verbatim from the billboarding
 * tutorial at:
 *  http://www.lighthouse3d.com/opengl/billboarding/index.php?billCheat
 */
void billboardBegin () {
  float modelview[16];
  int
    i = 0,
    j = 0;
  glPushMatrix ();
  glGetFloatv (GL_MODELVIEW_MATRIX, modelview);
  while (i < 3) {
    j = 0;
    while (j < 3) {
      if (i == j) {
        modelview[i*4+j] = 1.0;
      } else {
        modelview[i*4+j] = 0.0;
      }
      j++;
    }
    i++;
  }
  glLoadMatrixf (modelview);
}

void billboardEnd () {
  glPopMatrix ();
}
