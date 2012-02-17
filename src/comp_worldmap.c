/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#include "comp_worldmap.h"

#include "video.h"

#include "font.h"
#include "map.h"
#include "map_internal.h"
#include "component_position.h"
#include "component_input.h"
#include "comp_gui.h"

static void worldmap_create (EntComponent comp, EntSpeech speech);
static void worldmap_destroy (EntComponent comp, EntSpeech speech);
static void worldmap_input (EntComponent comp, EntSpeech speech);
static void worldmap_gainFocus (EntComponent comp, EntSpeech speech);
static void worldmap_loseFocus (EntComponent comp, EntSpeech speech);
static void worldmap_draw (EntComponent comp, EntSpeech speech);

static TEXTURE mapGenerateMapTexture (const hexPos centre, unsigned char span, float facing, const char * dataVal);

static Entity
	Worldmap;

void worldmap_define (EntComponent comp, EntSpeech speech)
{
	component_registerResponse ("worldmap", "__create", worldmap_create);
	component_registerResponse ("worldmap", "__destroy", worldmap_destroy);

	component_registerResponse ("worldmap", "FOCUS_INPUT", worldmap_input);

	component_registerResponse ("worldmap", "gainFocus", worldmap_gainFocus);
	component_registerResponse ("worldmap", "loseFocus", worldmap_loseFocus);

	component_registerResponse ("worldmap", "guiDraw", worldmap_draw);
}

static void worldmap_create (EntComponent comp, EntSpeech speech)
{
	Entity
		this = component_entityAttached (comp);
	worldmapData
		map;
	if (Worldmap)
		return;
	Worldmap = this;
	map = xph_alloc (sizeof (struct xph_worldmap_data));
	map->worldSpan = mapGetSpan ();
	map->spanFocus = map->worldSpan;
	map->typeFocus = -1;
	map->spanTypeFocus = FOCUS_SPAN;

	map->spanTextures = xph_alloc (sizeof (TEXTURE *) * (map->worldSpan + 1));

	component_setData (comp, map);
}

static void worldmap_destroy (EntComponent comp, EntSpeech speech)
{
	Entity
		this = component_entityAttached (comp);
	worldmapData
		map;
	if (this != Worldmap)
		return;
	map = component_getData (comp);
	// TODO: free any cached textures
	xph_free (map->spanTextures);
	xph_free (map);
	component_clearData (comp);

	Worldmap = NULL;
}

static void worldmap_input (EntComponent comp, EntSpeech speech)
{
	Entity
		this = component_entityAttached (comp);
	worldmapData
		map = component_getData (entity_getAs (Worldmap, "worldmap"));
	struct input_event
		* input = speech->arg;
	hexPos
		pos = position_get (entity_getByName ("PLAYER"));
	// the span index is backwards compared to usual because i'd like it to maintain the usual span mapping, which is 0 = hex level, MapSpan = pole level - xph 2012 01 17
	if (!input->active)
		return;

	switch (input->code)
	{
		case IR_UI_MENU_INDEX_DOWN:
			if (map->spanTypeFocus == FOCUS_SPAN && map->spanFocus != 0)
			{
				if (map->typeFocus != 0)
				{
					textureDestroy (map->spanTextures[map->spanFocus]);
					map->spanTextures[map->spanFocus] = NULL;
				}
				map->spanFocus--;
				map->types = mapDataTypes (hexPos_platter (pos, map->spanFocus));
				if (map->types)
					map->typeFocus = 0;
			}
			else if (map->types && map->typeFocus < (dynarr_size (map->types) - 1))
			{
				map->typeFocus++;
				if (map->spanTextures[map->spanFocus])
				{
					textureDestroy (map->spanTextures[map->spanFocus]);
					map->spanTextures[map->spanFocus] = NULL;
				}
			}
			break;
		case IR_UI_MENU_INDEX_UP:
			if (map->spanTypeFocus == FOCUS_SPAN && map->spanFocus != map->worldSpan)
			{
				if (map->typeFocus != 0)
				{
					textureDestroy (map->spanTextures[map->spanFocus]);
					map->spanTextures[map->spanFocus] = NULL;
				}
				map->spanFocus++;
				map->types = mapDataTypes (hexPos_platter (pos, map->spanFocus));
				if (map->types)
					map->typeFocus = 0;
			}
			else if (map->typeFocus != 0)
			{
				map->typeFocus--;
				if (map->spanTextures[map->spanFocus])
				{
					textureDestroy (map->spanTextures[map->spanFocus]);
					map->spanTextures[map->spanFocus] = NULL;
				}
			}
			break;
		case IR_UI_MODE_SWITCH:
			map->spanTypeFocus ^= 1;
			break;
		case IR_WORLDMAP_SWITCH:
			printf ("focusing world\n");
			entity_message (this, NULL, "loseFocus", NULL);
			entity_messageGroup ("PlayerAvatarEntities", NULL, "gainFocus", NULL);
			break;
		default:
			break;
	}
}

static void worldmap_gainFocus (EntComponent comp, EntSpeech speech)
{
	worldmapData
		map = component_getData (entity_getAs (Worldmap, "worldmap"));
	hexPos
		pos = position_get (entity_getByName ("PLAYER"));

	map->types = mapDataTypes (hexPos_platter (pos, map->spanFocus));
	if (map->types)
		map->typeFocus = 0;
}

static void worldmap_loseFocus (EntComponent comp, EntSpeech speech)
{
	Entity
		this = component_entityAttached (comp);
	worldmapData
		map = component_getData (entity_getAs (Worldmap, "worldmap"));
	int
		i = 0;
	while (i <= map->worldSpan)
	{
		if (map->spanTextures[i])
			textureDestroy (map->spanTextures[i]);
		map->spanTextures[i] = NULL;
		i++;
	}
	map->types = NULL;
	map->typeFocus = -1;
	map->spanTypeFocus = FOCUS_SPAN;
	gui_removeFromStack (this);
}

static void worldmap_draw (EntComponent comp, EntSpeech speech)
{
	float
		zNear = video_getZnear () - 0.001;
	unsigned int
		width, height;
	int
		i = 0,
		frameMargin = 32,
		sMargin = 8,
		sWidth = 55,
		sHeight = 34,
		maxWidth,
		maxHeight,
		mapSize,
		mapXMargin,
		mapYMargin;
	worldmapData
		map;
	const char
		* type;
	hexPos
		position;
	
	if (!Worldmap || !input_hasFocus (Worldmap))
		return;
	map = component_getData (entity_getAs (Worldmap, "worldmap"));
	video_getDimensions (&width, &height);
	glColor4ub (0xff, 0xff, 0xff, 0xff);
	glBindTexture (GL_TEXTURE_2D, 0);

	glBegin (GL_QUADS);
	i = 0;
	while (i <= map->worldSpan)
	{
		glVertex3f (
			video_xMap (width - (frameMargin + sWidth)),
			video_yMap (i * (sHeight + sMargin) + frameMargin),
			zNear
		);
		glVertex3f (
			video_xMap (width - (frameMargin + sWidth)),
			video_yMap (i * (sHeight + sMargin) + sHeight + frameMargin),
			zNear
		);
		glVertex3f (
			video_xMap (width - frameMargin),
			video_yMap (i * (sHeight + sMargin) + sHeight + frameMargin),
			zNear
		);
		glVertex3f (
			video_xMap (width - frameMargin),
			video_yMap (i * (sHeight + sMargin) + frameMargin),
			zNear
		);
		i++;
	}
	glColor3ub (0xff, 0x00, 0x99);
	if (map->spanTypeFocus == FOCUS_SPAN)
	{
		glVertex3f (video_xMap (width - (frameMargin - 8)), video_yMap ((map->worldSpan - map->spanFocus) * (sHeight + sMargin) + frameMargin + (sHeight / 2.0) + 8), zNear);
		glVertex3f (video_xMap (width - (frameMargin - 8)), video_yMap ((map->worldSpan - map->spanFocus) * (sHeight + sMargin) + frameMargin + (sHeight / 2.0) - 8), zNear);
		glVertex3f (video_xMap (width - (frameMargin + 8)), video_yMap ((map->worldSpan - map->spanFocus) * (sHeight + sMargin) + frameMargin + (sHeight / 2.0) - 8), zNear);
		glVertex3f (video_xMap (width - (frameMargin + 8)), video_yMap ((map->worldSpan - map->spanFocus) * (sHeight + sMargin) + frameMargin + (sHeight / 2.0) + 8), zNear);
	}
	else
	{
		int
			fh = fontLineHeight ();
		glVertex3f (video_xMap (frameMargin - 8), video_yMap (frameMargin + (map->typeFocus + 1) * fh + ((fh - 16) / 2) + 8), zNear);
		glVertex3f (video_xMap (frameMargin + 8), video_yMap (frameMargin + (map->typeFocus + 1) * fh + ((fh - 16) / 2) + 8), zNear);
		glVertex3f (video_xMap (frameMargin + 8), video_yMap (frameMargin + (map->typeFocus + 1) * fh + ((fh - 16) / 2) - 8), zNear);
		glVertex3f (video_xMap (frameMargin - 8), video_yMap (frameMargin + (map->typeFocus + 1) * fh + ((fh - 16) / 2) - 8), zNear);
	}
	glEnd ();

	position = position_get (entity_getByName ("PLAYER"));

	if (map->types != NULL)
	{
		i = 0;
		fontPrintAlign (ALIGN_LEFT);
		while ((type = *(const char **)dynarr_at (map->types, i++)))
		{
			if (i - 1 == map->typeFocus)
				glColor4ub (0xff, 0xff, 0xff, 0xff);
			else
				glColor4ub (0xaf, 0xaf, 0xaf, 0xff);
			fontPrint (type, frameMargin, frameMargin + fontLineHeight () * (i - 1));
		}
	}

	if (map->spanTextures[map->spanFocus] == NULL)
	{
		printf ("generating map texture for span %d using pos %p\n", map->spanFocus, position);
		if (map->types)
			map->spanTextures[map->spanFocus] = mapGenerateMapTexture (position, map->spanFocus, 0, *(const char **)dynarr_at (map->types, map->typeFocus));
		else
			map->spanTextures[map->spanFocus] = mapGenerateMapTexture (position, map->spanFocus, 0, NULL);
	}
	glBindTexture (GL_TEXTURE_2D, textureName (map->spanTextures[map->spanFocus]));
	glColor4ub (0xff, 0xff, 0xff, 0xff);

	maxWidth = width - (frameMargin * 2 + sWidth + sMargin);
	maxHeight = height - (frameMargin * 2);
	mapSize = maxWidth < maxHeight ? maxWidth : maxHeight;
	mapXMargin = (width - mapSize) / 2;
	mapYMargin = (height - mapSize) / 2;

	glBegin (GL_QUADS);
	glTexCoord2f (0.00, 0.00);
	glVertex3f (video_xMap (mapXMargin), video_yMap (mapYMargin), zNear);
	glTexCoord2f (0.00, 1.00);
	glVertex3f (video_xMap (mapXMargin), video_yMap (mapYMargin + mapSize), zNear);
	glTexCoord2f (1.00, 1.00);
	glVertex3f (video_xMap (mapXMargin + mapSize), video_yMap (mapYMargin + mapSize), zNear);
	glTexCoord2f (1.00, 0.00);
	glVertex3f (video_xMap (mapXMargin + mapSize), video_yMap (mapYMargin), zNear);
	glEnd ();

	glBindTexture (GL_TEXTURE_2D, 0);

}




static TEXTURE mapGenerateMapTexture (const hexPos centre, unsigned char span, float facing, const char * dataVal)
{
	TEXTURE
		texture;
	VECTOR3
		mapCoords,
		rot;
	SUBHEX
		centreLevel = hexPos_platter (centre, span),
		hex;
	VECTOR3
		borders[4];
	int
		l = 0,
		final,
		x, y,
		texWidth = 512,
		texHeight = 512,
		* scale;
	signed int
		data = 0,
		max = 0,
		min = 0,
		range = 0;
	unsigned char
		mapSpan,
		color[4] = {0xff, 0xff, 0xff, 0xff};

	textureSetBackgroundColor (0x00, 0x00, 0x00, 0x00);
	texture = textureGenBlank (texWidth, texHeight, 4);

	borders[0] = vectorCreate (0, 0, 0);
	borders[1] = vectorCreate (texWidth - 1, 0, 0);
	borders[2] = vectorCreate (texWidth - 1, texHeight - 1, 0);
	borders[3] = vectorCreate (0, texHeight - 1, 0);

	if (centreLevel == NULL)
	{
		ERROR ("Can't construct map texture for span level %d", span);
		return NULL;
	}
	//printf ("have base %p @ span %d\n", centreLevel, subhexSpanLevel (centreLevel));

	mapSpan = mapGetSpan ();
	if (span == mapSpan)
	{
		unsigned int
			r, k, i,
			tri;
		unsigned char
			pole;
		hex_xy2rki (centre->x[mapSpan], centre->y[mapSpan], &r, &k, &i);
		if (i < r / 2.0)
			tri = (k + 5) % 6;
		else
			tri = (k + 1) % 6;
		// TODO: draw one pole in the XY[k]-th direction and one in the XY[tri]-th direction. the issue is figuring out which pole is which
		pole = subhexPoleName (centre->platter[mapSpan]);
		switch (pole)
		{
			case 'r':
				textureSetColor (0xff, 0x00, 0x00, 0xff);
				break;
			case 'g':
				textureSetColor (0x00, 0xff, 0x00, 0xff);
				break;
			case 'b':
				textureSetColor (0x00, 0x00, 0xff, 0xff);
				break;
			default:
				textureSetColor (0xff, 0x00, 0xff, 0xff);
				break;
		}
		mapCoords = vectorCreate (0, 0, 0);
		textureDrawHex (texture, vectorCreate ((texWidth / 2.0) + mapCoords.x, (texHeight / 2.0) + mapCoords.z, 0.0), 32, -facing);

		textureSetColor (0x00, 0x00, 0x00, 0xff);
		textureDrawLine (texture, borders[0], borders[1]);
		textureDrawLine (texture, borders[1], borders[2]);
		textureDrawLine (texture, borders[2], borders[3]);
		textureDrawLine (texture, borders[3], borders[0]);

		textureBind (texture);
		return texture;
	}

	final = fx (24);
	if (span != 0 && dataVal != NULL)
	{
		l = 0;
		while (l < final)
		{
			hex_unlineate (l, &x, &y);
			hex = mapHexAtCoordinateAuto (centreLevel, 0, x, y);
			data = mapDataGet (hex, dataVal);
			if (data < min)
				min = data;
			if (data > max)
				max = data;
			l++;
		}
		range = max - min;
	}

	l = 0;
	while (l < final)
	{
		hex_unlineate (l, &x, &y);
		//printf ("%d: %d, %d\n", l, x, y);
		hex = mapHexAtCoordinateAuto (centreLevel, 0, x, y);
		//printf ("  got subhex %p\n", hex);
		if (hex == NULL)
		{
			color[0] = 0xff;
			color[1] = 0xff;
			color[2] = 0xff;
			color[3] = 0x00;
		}
		else if (hex->type == HS_HEX)
		{
			color[0] = 0xff;
			color[1] = 0xff;
			color[2] = 0xff;
			color[3] = 0xff;
		}
		else
		{
			if (range == 0)
			{
				color[0] = 0xff;
				color[1] = 0x00;
				color[2] = 0xff;
				color[3] = 0xff;
			}
			else
			{
				data = mapDataGet (hex, dataVal);
				color[0] = ((data - min) / (float)range) * 255;
				color[1] = ((data - min) / (float)range) * 255;
				color[2] = ((data - min) / (float)range) * 255;
				color[3] = 0xff;
			}
		}

		scale = mapSpanCentres (span);
		//printf ("  got %p with %d\n", scale, span);

		mapCoords = hex_xyCoord2Space
		(
			x * scale[0] + y * scale[2],
			x * scale[1] + y * scale[3]
		);
		mapCoords = vectorDivideByScalar (&mapCoords, 52 * hexMagnitude (scale[0], scale[1]));

		rot = mapCoords;
		mapCoords.x = rot.x * cos (-facing) - rot.z * sin (-facing);
		mapCoords.z = rot.x * sin (-facing) + rot.z * cos (-facing);

		mapCoords = vectorMultiplyByScalar (&mapCoords, 17);
		if (color[3] != 0x00)
		{
			textureSetColor (color[0], color[1], color[2], color[3]);
			textureDrawHex (texture, vectorCreate ((texWidth / 2.0) + mapCoords.x, (texHeight / 2.0) + mapCoords.z, 0.0), 8, -facing);
		}
		//printf ("{%d %d %d} %d, %d (real offset: %d, %d) at %2.2f, %2.2f (span %d) -- %2.2x%2.2x%2.2x / %p\n", r, k, i, x, y, localX + x, localY + y, mapCoords.x, mapCoords.z, span, color[0], color[1], color[2], hex);
		
		l++;
	}

	textureSetColor (0x00, 0x00, 0x00, 0xff);
	textureDrawLine (texture, borders[0], borders[1]);
	textureDrawLine (texture, borders[1], borders[2]);
	textureDrawLine (texture, borders[2], borders[3]);
	textureDrawLine (texture, borders[3], borders[0]);

	textureBind (texture);
	return texture;
}
