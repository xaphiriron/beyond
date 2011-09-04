#include "component_ui.h"

#include "video.h"
#include "font.h"
#include "texture.h"
#include "hex_utility.h"

#include "system.h"

#include "component_position.h"
#include "component_input.h"
#include "map.h"

enum uiSkewTypes
{
	SKEW_LEFT	= 0x01,
	SKEW_RIGHT	= 0x02,
	SKEW_UP		= 0x04,
	SKEW_DOWN	= 0x08,
};

struct uiTextFragment
{
	char
		* text;
	signed int
		xAlign,
		yAlign;
	enum textAlignType
		align;
	/* the point of :reservedText is that sometimes (by which currently I mean
	 * "only when we're drawing the system debug text") the :text string is an
	 * external string instead of something allocated by the ui fragment
	 * itself, and in that case: 1) it shouldn't be freed when the fragment is
	 * destroyed and 2) in the future when text spacing is precalculated
	 * reserved text /can't/ be precalculated, since the text may change at any
	 * time
	 *  - xph 2011 08 28
	 */
	bool
		reservedText;
};

struct uiStaticText
{
	enum uiPanelTypes
		type;

	Dynarr
		fragments;
};

struct uiMapData
{
	enum uiPanelTypes
		type;

	Dynarr
		scales;
	unsigned char
		currentScale;
};

union uiPanels
{
	enum uiPanelTypes
		type;
	struct uiStaticText
		staticText;
	struct uiMapData
		map;
};

static unsigned int
	height,
	width;

static UIPANEL uiPanel_createEmpty ();
static void uiPanel_destroy (UIPANEL ui);

static struct uiTextFragment * uiFragmentCreate (const char * text, enum textAlignType align, signed int x, signed int y);
static void uiFragmentDestroy (void * v);

static void uiWorldmapGenLevel (struct uiMapData * map, unsigned char span);

static void uiDrawPane (signed int x, signed int y, signed int width, signed int height, const TEXTURE texture);
//static void uiDrawSkewPane (signed int x, signed int y, signed int width, signed int height, enum uiSkewTypes skew, const void * const * style);

void uiDrawCursor ()
{
	unsigned int
		centerHeight,
		centerWidth,
		halfTexHeight,
		halfTexWidth;
	float
		zNear = video_getZnear () - 0.001,
		top, bottom,
		left, right;
/*
	if (t == NULL)
		return;
*/
	centerHeight = height / 2;
	centerWidth = width / 2;
	halfTexHeight = 2;//texture_pxHeight (t) / 2;
	halfTexWidth = 2;//texture_pxWidth (t) / 2;
	top = video_pixelYMap (centerHeight + halfTexHeight);
	bottom = video_pixelYMap (centerHeight - halfTexHeight);
	left = video_pixelXMap (centerWidth - halfTexWidth);
	right = video_pixelXMap (centerWidth + halfTexWidth);
	//printf ("top: %7.2f; bottom: %7.2f; left: %7.2f; right: %7.2f\n", top, bottom, left, right);
	glColor3f (1.0, 1.0, 1.0);
	//glBindTexture (GL_TEXTURE_2D, texture_glID (t));
	glBegin (GL_TRIANGLE_FAN);
	glVertex3f (left, top, zNear);
	glVertex3f (right, top, zNear);
	glVertex3f (right, bottom, zNear);
	glVertex3f (left, bottom, zNear);
	glEnd ();
}

/***
 * UI COMPONENT RESPONSES
 */

void ui_classInit (EntComponent ui, void * arg)
{
	video_getDimensions (&width, &height);

	component_registerResponse ("ui", "__init", ui_create);
	component_registerResponse ("ui", "__destroy", ui_destroy);

	component_registerResponse ("ui", "setType", ui_setType);
	component_registerResponse ("ui", "getType", ui_getType);

	component_registerResponse ("ui", "FOCUS_INPUT", ui_handleInput);

	component_registerResponse ("ui", "draw", ui_draw);
}

void ui_create (EntComponent ui, void * arg)
{
	UIPANEL
		uiData = uiPanel_createEmpty ();

	component_setData (ui, uiData);
}

void ui_destroy (EntComponent ui, void * arg)
{
	UIPANEL
		uiData = component_getData (ui);
	
	uiPanel_destroy (uiData);
	component_clearData (ui);
}

void ui_setType (EntComponent ui, void * arg)
{
	Entity
		uiEntity = component_entityAttached (ui);
	UIPANEL
		uiData = component_getData (ui);
	enum uiPanelTypes
		type = (enum uiPanelTypes)arg;
	struct uiTextFragment
		* systemText;

	if (uiData->type != UI_NONE)
	{
		WARNING ("Entity #%d already has a UI type of %d; overwriting", entity_GUID (uiEntity), uiData->type);
	}

	uiData->type = type;
	switch (type)
	{
		case UI_STATICTEXT:
			uiData->staticText.fragments = dynarr_create (3, sizeof (struct uiTextFragment *));
			break;

		case UI_DEBUG_OVERLAY:
			uiData->staticText.fragments = dynarr_create (3, sizeof (struct uiTextFragment *));

			systemText = uiFragmentCreate (NULL, ALIGN_LEFT, 8, 8);
			systemText->text = systemGenDebugStr ();
			dynarr_assign (uiData->staticText.fragments, 0, systemText);

			dynarr_assign (uiData->staticText.fragments, 1, uiFragmentCreate ("raise em til your arms tired\nlet em know you here\nthat you struggling surviving\nthat you gonna persevere", ALIGN_RIGHT, -8, 8));
			break;

		case UI_WORLDMAP:

			uiData->map.scales = dynarr_create (mapGetSpan () + 1, sizeof (TEXTURE));
			uiWorldmapGenLevel (&uiData->map, 0);
			uiData->map.currentScale = 0;
			break;

		default:
			WARNING ("Can't set #%d's UI type; invalid type of %d given", entity_GUID (uiEntity), type);
			uiData->type = UI_NONE;
			break;
	}
}

void ui_getType (EntComponent ui, void * arg)
{
	UIPANEL
		uiData = component_getData (ui);
	enum uiPanelTypes
		* type = arg;
	*type = uiData->type;
}

void ui_handleInput (EntComponent ui, void * arg)
{
	UIPANEL
		uiData = component_getData (ui);
	struct input_event
		* inputEvent = arg;
	switch (uiData->type)
	{
		case UI_WORLDMAP:
			switch (inputEvent->ir)
			{
				case IR_UI_WORLDMAP_SCALE_UP:
					if (uiData->map.currentScale >= mapGetSpan ())
						break;
					uiData->map.currentScale++;
					if (*(TEXTURE *)dynarr_at (uiData->map.scales, uiData->map.currentScale) == NULL)
					{
						uiWorldmapGenLevel (&uiData->map, uiData->map.currentScale);
					}

					break;
				case IR_UI_WORLDMAP_SCALE_DOWN:
					if (uiData->map.currentScale == 0)
						break;
					uiData->map.currentScale--;
					if (*(TEXTURE *)dynarr_at (uiData->map.scales, uiData->map.currentScale) == NULL)
					{
						uiWorldmapGenLevel (&uiData->map, uiData->map.currentScale);
					}
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
}

void ui_draw (EntComponent ui, void * arg)
{
	UIPANEL
		uiData = component_getData (ui);

	struct uiTextFragment
		* fragment = NULL;
	int
		i = 0;

	switch (uiData->type)
	{
		case UI_DEBUG_OVERLAY:
			systemGenDebugStr ();
			/* fall through */
		case UI_STATICTEXT:
			glColor3f (0.0, 0.0, 0.0);
			while ((fragment = *(struct uiTextFragment **)dynarr_at (uiData->staticText.fragments, i++)) != NULL)
			{
				textAlign (fragment->align);
				drawLine (fragment->text, fragment->xAlign, fragment->yAlign);
			}
			break;

		case UI_WORLDMAP:
			
			glColor3f (1.0, 1.0, 1.0);
			uiDrawPane ((width - (height - 32)) / 2, 16, height - 32, height - 32, *(TEXTURE *)dynarr_at (uiData->map.scales, uiData->map.currentScale));
			break;

		default:
			break;	
	}
}

/***
 * INTERNAL FUNCTIONS
 */

static UIPANEL uiPanel_createEmpty ()
{
	UIPANEL
		ui = xph_alloc (sizeof (union uiPanels));
	memset (ui, 0, sizeof (union uiPanels));

	return ui;
}

static void uiPanel_destroy (UIPANEL ui)
{
	switch (ui->type)
	{
		case UI_DEBUG_OVERLAY:
		case UI_STATICTEXT:
			dynarr_map (ui->staticText.fragments, uiFragmentDestroy);
			dynarr_destroy (ui->staticText.fragments);
			break;

		case UI_WORLDMAP:
			dynarr_map (ui->map.scales, (void (*)(void *))textureDestroy);
			dynarr_destroy (ui->map.scales);
			break;
		default:
			break;
	}

	xph_free (ui);
}


static struct uiTextFragment * uiFragmentCreate (const char * text, enum textAlignType align, signed int x, signed int y)
{
	struct uiTextFragment
		* uit = xph_alloc (sizeof (struct uiTextFragment));
	if (text != NULL)
	{
		uit->text = xph_alloc (strlen (text) + 1);
		strcpy (uit->text, text);
		uit->reservedText = FALSE;
	}
	else
	{
		uit->text = NULL;
		uit->reservedText = TRUE;
	}
	uit->align = align;
	if (x < 0)
		uit->xAlign = (signed int)width + x;
	else
		uit->xAlign = x;

	if (y < 0)
		uit->yAlign = (signed int)height + y;
	else
		uit->yAlign = y;

	return uit;
}

static void uiFragmentDestroy (void * v)
{
	struct uiTextFragment
		* uit = v;
	if (!uit->reservedText)
		xph_free (uit->text);
	xph_free (uit);
}

static void uiWorldmapGenLevel (struct uiMapData * map, unsigned char span)
{
	SUBHEX
		playerLocation;
	Entity
		player = input_getPlayerEntity ();
	float
		facing;
	entity_message (player, NULL, "getHex", &playerLocation);
	entity_message (player, NULL, "getHexAngle", &facing);

	dynarr_assign (map->scales, span, mapGenerateMapTexture (playerLocation, facing, span));
}


static void uiDrawPane (signed int x, signed int y, signed int width, signed int height, const TEXTURE texture)
{
	float
		top = video_pixelYMap (y),
		left = video_pixelXMap (x),
		bottom = video_pixelYMap (y + height),
		right = video_pixelXMap (x + width),
		zNear = video_getZnear () - 0.001;

	if (texture)
		glBindTexture (GL_TEXTURE_2D, textureName (texture));
	else
		glBindTexture (GL_TEXTURE_2D, 0);
	glBegin (GL_TRIANGLE_FAN);
	glTexCoord2f (0.0, 1.0);
	glVertex3f (left, top, zNear);
	glTexCoord2f (0.0, 0.0);
	glVertex3f (left, bottom, zNear);
	glTexCoord2f (1.0, 0.0);
	glVertex3f (right, bottom, zNear);
	glTexCoord2f (1.0, 1.0);
	glVertex3f (right, top, zNear);
	glEnd ();
}

/*
static void uiDrawSkewPane (signed int x, signed int y, signed int width, signed int height, enum uiSkewTypes skew, const void * const * style)
{
	float
		top = video_pixelYMap (y),
		left = video_pixelXMap (x),
		bottom = video_pixelYMap (y + height),
		right = video_pixelXMap (x + width),
		zNear = video_getZnear () - 0.001,
		su = 0,
		sd = 0,
		sl = 0,
		sr = 0;

	if (skew & SKEW_UP)
		su = 10;
	else if (skew & SKEW_DOWN)
		sd = 10;

	if (skew & SKEW_LEFT)
		sl = 10;
	else if (skew & SKEW_RIGHT)
		sr = 10;

	glColor3f (1.0, 1.0, 1.0);
	glBegin (GL_TRIANGLE_FAN);
	glVertex3f (left, top, zNear - (sl + su));
	glVertex3f (left, bottom, zNear - (sl + sd));
	glVertex3f (right, bottom, zNear - (sr + sd));
	glVertex3f (right, top, zNear - (sr + su));
	glEnd ();
}
*/
