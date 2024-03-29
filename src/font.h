/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#ifndef XPH_FONT_H
#define XPH_FONT_H

#include <ft2build.h>
#include FT_FREETYPE_H

#include "video.h"
#include "xph_log.h"
#include "xph_path.h"
#include "xph_memory.h"


enum textAlignType {
ALIGN_LEFT = 1,
ALIGN_RIGHT = 2,
ALIGN_CENTER = 3,
ALIGN_CENTRE = 3,
/*
ALIGN_JUSTIFY = 4
*/
};

typedef struct font_textcache * Text;

void fontLoad (const char * path, int fontSize);
void fontUnload ();

int fontLineHeight ();

Text fontGenerate (const char * text, enum textAlignType align, int x, int y, int w);
void fontDestroyText (Text t);

enum textAlignType fontPrintAlign (enum textAlignType);
void fontPrint (const char * text, int x, int y);
void fontTextPrint (Text t);


#endif /* XPH_FONT_H */