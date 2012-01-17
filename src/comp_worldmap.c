#include "comp_worldmap.h"

#include "video.h"

#include "map.h"
#include "map_internal.h"
#include "component_position.h"
#include "component_input.h"

static void worldmap_create (EntComponent comp, EntSpeech speech);
static void worldmap_destroy (EntComponent comp, EntSpeech speech);
static void worldmap_draw (EntComponent comp, EntSpeech speech);
static void worldmap_input (EntComponent comp, EntSpeech speech);

static TEXTURE mapGenerateMapTexture (SUBHEX centre, float facing, unsigned char span);

static Entity
	Worldmap;

void worldmap_define (EntComponent comp, EntSpeech speech)
{
	component_registerResponse ("worldmap", "__create", worldmap_create);
	component_registerResponse ("worldmap", "__destroy", worldmap_destroy);

	component_registerResponse ("worldmap", "FOCUS_INPUT", worldmap_input);
	component_registerResponse ("worldmap", "CONTROL_INPUT", worldmap_input);

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

	map->spanTextures = xph_alloc (sizeof (TEXTURE *) * map->worldSpan);

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
	struct input_event
		* input = speech->arg;
	worldmapData
		map = component_getData (entity_getAs (Worldmap, "worldmap"));
	if (!map)
		return;
	switch (input->ir)
	{
		case IR_UI_MENU_INDEX_DOWN:
			if (map->spanFocus != 0)
				map->spanFocus--;
			break;
		case IR_UI_MENU_INDEX_UP:
			if (map->spanFocus != map->worldSpan)
				map->spanFocus++;
			break;
		default:
			break;
	}
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
		sHeight = 34;
	worldmapData
		map;
	hexPos
		position;
	SUBHEX
		mapCentre;
	
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
	glColor3ub (0xff, 0x00, 0x00);
	glVertex3f (video_xMap (width - (frameMargin - 8)), video_yMap ((map->worldSpan - map->spanFocus) * (sHeight + sMargin) + frameMargin + (sHeight / 2.0) + 8), zNear);
	glVertex3f (video_xMap (width - (frameMargin - 8)), video_yMap ((map->worldSpan - map->spanFocus) * (sHeight + sMargin) + frameMargin + (sHeight / 2.0) - 8), zNear);
	glVertex3f (video_xMap (width - (frameMargin + 8)), video_yMap ((map->worldSpan - map->spanFocus) * (sHeight + sMargin) + frameMargin + (sHeight / 2.0) - 8), zNear);
	glVertex3f (video_xMap (width - (frameMargin + 8)), video_yMap ((map->worldSpan - map->spanFocus) * (sHeight + sMargin) + frameMargin + (sHeight / 2.0) + 8), zNear);
	glEnd ();

	position = position_get (entity_getByName ("PLAYER"));

	if (map->spanTextures[map->spanFocus] == NULL)
	{
		mapCentre = hexPos_platter (position, 0);
		printf ("generating map texture for span %d using hex %p (level %d)\n", map->spanFocus, mapCentre, subhexSpanLevel (mapCentre));
		map->spanTextures[map->spanFocus] = mapGenerateMapTexture (mapCentre, 0, map->spanFocus);
	}
	glBindTexture (GL_TEXTURE_2D, textureName (map->spanTextures[map->spanFocus]));
	glColor4ub (0xff, 0xff, 0xff, 0xff);

	glBegin (GL_QUADS);
	glTexCoord2f (0.00, 0.00);
	glVertex3f (video_xMap (frameMargin), video_yMap (frameMargin), zNear);
	glTexCoord2f (0.00, 1.00);
	glVertex3f (video_xMap (frameMargin), video_yMap (height - frameMargin), zNear);
	glTexCoord2f (1.00, 1.00);
	glVertex3f (video_xMap (width - (frameMargin + sWidth + sMargin)), video_yMap (height - frameMargin), zNear);
	glTexCoord2f (1.00, 0.00);
	glVertex3f (video_xMap (width - (frameMargin + sWidth + sMargin)), video_yMap (frameMargin), zNear);
	glEnd ();

	glBindTexture (GL_TEXTURE_2D, 0);

}



TEXTURE mapGenerateMapTexture (SUBHEX centre, float facing, unsigned char span)
{
	TEXTURE
		texture;
	VECTOR3
		mapCoords,
		rot;
	SUBHEX
		centreLevel = centre,
		hex;
	int
		i = 0,
		max,
		localX = 0, localY = 0,
		x, y,
		texWidth = 512,
		texHeight = 512,
		targetSpan = 0,
		* scale;
	unsigned char
		color[4] = {0xff, 0xff, 0xff, 0xff};

	textureSetBackgroundColor (0x00, 0x00, 0x00, 0x00);
	texture = textureGenBlank (texWidth, texHeight, 4);

	subhexLocalCoordinates (centre, &localX, &localY);

	while (targetSpan < span)
	{
		centreLevel = subhexParent (centreLevel);
		targetSpan++;
	}
	if (centreLevel == NULL)
	{
		ERROR ("Can't construct map texture for span level %d", span);
		return NULL;
	}
	printf ("have base %p @ span %d\n", centreLevel, subhexSpanLevel (centreLevel));

	i = 0;
	max = fx (16);
	while (i < max)
	{
		hex_unlineate (i, &x, &y);
		printf ("%d: %d, %d\n", i, x, y);
		hex = mapHexAtCoordinateAuto (centreLevel, 0, x, y);
		printf ("  got subhex %p\n", hex);
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
			color[0] = mapDataGet (hex, "height") / (float)(1 << 9) * 255;
			color[1] = mapDataGet (hex, "height") / (float)(1 << 9) * 255;
			color[2] = mapDataGet (hex, "height") / (float)(1 << 9) * 255;
			color[3] = 0xff;
		}

		scale = mapSpanCentres (span);
		printf ("  got %p with %d\n", scale, span);

		mapCoords = hex_xyCoord2Space
		(
			x * scale[0] + y * scale[2],
			x * scale[1] + y * scale[3]
		);
		mapCoords = vectorDivideByScalar (&mapCoords, 52 * hexMagnitude (scale[0], scale[1]));

		rot = mapCoords;
		mapCoords.x = rot.x * cos (-facing) - rot.z * sin (-facing);
		mapCoords.z = rot.x * sin (-facing) + rot.z * cos (-facing);

		mapCoords = vectorMultiplyByScalar (&mapCoords, 14);
		if (color[3] != 0x00)
		{
			textureSetColor (color[0], color[1], color[2], color[3]);
			textureDrawHex (texture, vectorCreate ((texWidth / 2.0) + mapCoords.x, (texHeight / 2.0) + mapCoords.z, 0.0), 6, -facing);
		}
		//printf ("{%d %d %d} %d, %d (real offset: %d, %d) at %2.2f, %2.2f (span %d) -- %2.2x%2.2x%2.2x / %p\n", r, k, i, x, y, localX + x, localY + y, mapCoords.x, mapCoords.z, span, color[0], color[1], color[2], hex);
		
		i++;
	}
	textureBind (texture);

	return texture;
}
