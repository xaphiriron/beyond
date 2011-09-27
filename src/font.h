#ifndef XPH_FONT_H
#define XPH_FONT_H

enum textAlignType {
ALIGN_LEFT = 1,
ALIGN_RIGHT = 2,
/*
ALIGN_CENTER = 3,
ALIGN_CENTRE = 3,
ALIGN_JUSTIFY = 4
*/
};


void loadFont (const char * path);

int systemLineHeight ();

enum textAlignType textAlign (enum textAlignType);
void drawLine (const char * line, signed int x, signed int y);

#endif /* XPH_FONT_H */