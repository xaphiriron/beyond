#ifndef XPH_HEX_DRAW_H
#define XPH_HEX_DRAW_H

#include <SDL/SDL_opengl.h>

#include "hex.h"
#include "hex_utility.h"
#include "ground_draw.h"

void hex_setDrawColor (float red, float green, float blue);
void hex_draw (const Hex hex, const Entity camera, const CameraGroundLabel label);
void hex_drawFiller (const CameraGroundLabel label, signed int scale);

#endif /* XPH_HEX_DRAW_H */