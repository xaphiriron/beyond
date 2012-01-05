#ifndef XPH_FONT_H
#define XPH_FONT_H

#include <ft2build.h>
#include FT_FREETYPE_H

#include <GL/glpng.h>
#include "video.h"
#include "xph_log.h"
#include "xph_path.h"
#include "xph_memory.h"


enum textAlignType {
ALIGN_LEFT = 1,
ALIGN_RIGHT = 2,
/*
ALIGN_CENTER = 3,
ALIGN_CENTRE = 3,
ALIGN_JUSTIFY = 4
*/
};

void fontLoad (const char * path, int fontSize);
void fontUnload ();

int fontLineHeight ();

enum textAlignType fontPrintAlign (enum textAlignType);
void fontPrint (const char * text, int x, int y);


#endif /* XPH_FONT_H */