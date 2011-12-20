#include "component_ui.h"

#include "video.h"
#include <GL/glpng.h>
#include "texture_internal.h"
#include "texture.h"
#include "font.h"
#include "sprite.h"
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

struct uiMenuOpt
{
	float
		highlight;

	enum input_responses
		action;
};

struct uiFrame
{
	enum uiFramePos
		positionType;
	enum uiFrameBackground
		background;
	signed int
		x, y,
		xMargin,
		border,
		lineSpacing;
};

struct uiStaticText
{
	enum uiPanelTypes
		type;
	struct uiFrame
		* frame;

	Dynarr
		fragments;
};

struct uiMenu
{
	enum uiPanelTypes
		type;
	struct uiFrame
		* frame;

	Dynarr
		fragments,
		menuOpt;

	unsigned int
		lastIndex,
		activeIndex;
	float
		fadeScale;
	TIMER
		timer;
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
	struct uiMenu
		menu;
};

static unsigned int
	height,
	width;

struct panelTexture
{
	unsigned int
		spriteSize;
	SPRITESHEET
		base;	// holds non-repeatable versions of all the ui sprites
	TEXTURE
		lrSide,	// repeatable left and right side
		tbSide,	// repeatable top and bottom side
		inside;	// repeatable inside
};

static void panelTextureDestroy (struct panelTexture * pt);
static struct panelTexture * panelTextureLoad (const char * path);

static UIPANEL uiPanel_createEmpty ();
static void uiPanel_destroy (UIPANEL ui);

static struct uiFrame * uiFrame_createEmpty ();
static void uiFrame_destroy (struct uiFrame * frame);

static void uiFrame_getXY (struct uiFrame * frame, signed int * x, signed int * y);

static struct uiTextFragment * uiFragmentCreate (const char * text, enum textAlignType align, signed int x, signed int y);
static void uiFragmentDestroy (void * v);

static struct uiMenuOpt * uiMenuOpt_createEmpty ();
static void uiMenuOpt_destroy (struct uiMenuOpt * opt);

// should this maybe be something that a message can be sent about? - xph 2011 09 29
static void uiMenu_setActiveIndex (UIPANEL ui, unsigned int index);
static signed int uiMenu_mouseHit (UIPANEL ui, signed int mx, signed int my);

static void uiWorldmapGenLevel (struct uiMapData * map, unsigned char span);

static void uiDrawPane (signed int x, signed int y, signed int width, signed int height, const TEXTURE texture);
static void ui_drawIndex (struct uiMenu * menu);
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

static struct panelTexture
	* SystemPanel = NULL;
void uiLoadPanelTexture (const char * path)
{
	if (SystemPanel != NULL)
	{
		WARNING ("Resetting system panel (with \"%s\")", path);
		panelTextureDestroy (SystemPanel);
		SystemPanel = NULL;
	}
	SystemPanel = panelTextureLoad (path);
}

static void panelTextureDestroy (struct panelTexture * pt)
{
	sheetDestroy (pt->base);
	textureDestroy (pt->lrSide);
	textureDestroy (pt->tbSide);
	textureDestroy (pt->inside);
	xph_free (pt);
}

static struct panelTexture * panelTextureLoad (const char * path)
{
	struct panelTexture
		* pt = xph_alloc (sizeof (struct panelTexture));
	TEXTURE
		base;
	unsigned int
		spriteSize;
	pngSetStandardOrientation (1);
	base = textureGenFromImage (path);
	// divide the sheet into four sprites across, eight sprites down, no matter the side of the image (so long as it's got the right proportions)
	if (base->width >> 2 != base->height >> 3)
	{
		ERROR ("Invalid image resolution for ui frames; need image twice as tall as wide, with side lengths that are powers of two (e.g., 1x2, 2x4, 4x8, 8x16, 16x32, 32x64 64x128, etc) got %dx%d, which is wrong.", base->width, base->height);
		return NULL;
	}
	spriteSize = base->width >> 2;
	pt->spriteSize = spriteSize;
	pngSetStandardOrientation (0);
	pt->base = sheetCreate (path, SHEET_CONSTANT, spriteSize, spriteSize);
	pt->lrSide = textureGenBlank (spriteSize * 2, spriteSize, base->mode);
	pt->tbSide = textureGenBlank (spriteSize, spriteSize * 2, base->mode);
	pt->inside = textureGenBlank (spriteSize, spriteSize, base->mode);

	// set the 0,0 texture to the left side (at 0,1 on base)
	textureCopyChunkFromRaw (pt->lrSide, 0, 0, spriteSize, spriteSize, base, 0, spriteSize);
	// set the 1,0 texture to the right side (at 2,1 on base)
	textureCopyChunkFromRaw (pt->lrSide, spriteSize, 0, spriteSize, spriteSize, base, spriteSize * 2, spriteSize);
	textureBind (pt->lrSide);

	// set the 0,0 texture to the top side (1, 0 on base)
	textureCopyChunkFromRaw (pt->tbSide, 0, 0, spriteSize, spriteSize, base, spriteSize, 0);
	// set the 0,1 texture to the bottom side (1, 2 on base)
	textureCopyChunkFromRaw (pt->tbSide, 0, spriteSize, spriteSize, spriteSize, base, spriteSize, spriteSize * 2);
	textureBind (pt->tbSide);

	// set the 0,0 texture to the inside (1,1 on base)
	textureCopyChunkFromRaw (pt->inside, 0, 0, spriteSize, spriteSize, base, spriteSize, spriteSize);
	textureBind (pt->inside);

	textureDestroy (base);

	return pt;
}



void ui_getXY (UIPANEL ui, signed int * x, signed int * y)
{
	signed int
		fx, fy;
	struct uiFrame
		* frame = NULL;
	if (ui->type != UI_MENU && ui->type != UI_STATICTEXT)
	{
		if (x != NULL)
			*x = 0;
		if (y != NULL)
			*y = 0;
		return;
	}
	if (ui->type == UI_MENU)
	{
		frame = ui->menu.frame;
	}
	else
	{
		frame = ui->staticText.frame;
	}

	uiFrame_getXY (frame, &fx, &fy);
	if (x != NULL)
		*x = fx - frame->border;
	if (y != NULL)
		*y = fy - frame->border;
}

void ui_getWH (UIPANEL ui, signed int * w, signed int * h)
{
	struct uiFrame
		* frame = NULL;
	Dynarr
		fragments = NULL;

	if (ui->type != UI_MENU && ui->type != UI_STATICTEXT)
	{
		if (w != NULL)
			*w = 0;
		if (h != NULL)
			*h = 0;
		return;
	}
	if (ui->type == UI_MENU)
	{
		frame = ui->menu.frame;
		fragments = ui->menu.fragments;
	}
	else
	{
		frame = ui->staticText.frame;
		fragments = ui->staticText.fragments;
	}

	if (w != NULL)
		*w = frame->xMargin + frame->border;
	if (h != NULL)
		*h = dynarr_size (fragments) * frame->lineSpacing + frame->border;
}

/***
 * UI COMPONENT RESPONSES
 */

void ui_classInit (EntComponent ui, EntSpeech speech)
{
	video_getDimensions (&width, &height);

	component_registerResponse ("ui", "__create", ui_create);
	component_registerResponse ("ui", "__destroy", ui_destroy);

	component_registerResponse ("ui", "__update", ui_update);

	component_registerResponse ("ui", "setType", ui_setType);
	component_registerResponse ("ui", "getType", ui_getType);

	component_registerResponse ("ui", "addValue", ui_addValue);
	component_registerResponse ("ui", "setAction", ui_setAction);

	component_registerResponse ("ui", "setPosType", ui_setPositionType);
	component_registerResponse ("ui", "setFrameBG", ui_setBackground);
	component_registerResponse ("ui", "setBorder", ui_setBorder);
	component_registerResponse ("ui", "setLineSpacing", ui_setLineSpacing);
	component_registerResponse ("ui", "setXPos", ui_setXPosition);
	component_registerResponse ("ui", "setYPos", ui_setYPosition);
	component_registerResponse ("ui", "setWidth", ui_setWidth);

	component_registerResponse ("ui", "FOCUS_INPUT", ui_handleInput);

	component_registerResponse ("ui", "draw", ui_draw);
}

void ui_create (EntComponent ui, EntSpeech speech)
{
	UIPANEL
		uiData = uiPanel_createEmpty ();

	component_setData (ui, uiData);
}

void ui_destroy (EntComponent ui, EntSpeech speech)
{
	UIPANEL
		uiData = component_getData (ui);
	
	uiPanel_destroy (uiData);
	component_clearData (ui);
}

void ui_update (EntComponent ui, EntSpeech speech)
{
	Dynarr
		uiEntities = entity_getWith (1, "ui");
	Entity
		uiEntity;
	UIPANEL
		uiData;
	struct uiMenuOpt
		* opt;
	int
		i = 0,
		j = 0;

	while ((uiEntity = *(Entity *)dynarr_at (uiEntities, i++)) != NULL)
	{
		uiData = component_getData (entity_getAs (uiEntity, "ui"));
		j = 0;

		switch (uiData->type)
		{
			case UI_MENU:
				while ((opt = *(struct uiMenuOpt **)dynarr_at (uiData->menu.menuOpt, j++)) != NULL)
				{
					if (opt->highlight == 0.0)
						continue;
					opt->highlight -= lastTimestep (uiData->menu.timer) * uiData->menu.fadeScale;
					if (opt->highlight < 0.0)
						opt->highlight = 0.0;
				}
				break;
			default:
				break;
		}

	}
}

void ui_setType (EntComponent ui, EntSpeech speech)
{
	Entity
		uiEntity = component_entityAttached (ui);
	UIPANEL
		uiData = component_getData (ui);
	enum uiPanelTypes
		type = (enum uiPanelTypes)speech->arg;
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
			uiData->staticText.frame = uiFrame_createEmpty ();
			break;

		case UI_DEBUG_OVERLAY:
			uiData->staticText.fragments = dynarr_create (3, sizeof (struct uiTextFragment *));

			systemText = uiFragmentCreate (NULL, ALIGN_LEFT, 8, 8);
			systemText->text = systemGenDebugStr ();
			dynarr_assign (uiData->staticText.fragments, 0, systemText);

			dynarr_assign (uiData->staticText.fragments, 1, uiFragmentCreate ("the ocean floor was open long ago\nand currents started forming long ago\nwater began circling long ago\nthis ship started sinking long ago", ALIGN_RIGHT, -8, 8));
			/*
			dynarr_assign (uiData->staticText.fragments, 1, uiFragmentCreate ("raise em til your arms tired\nlet em know you here\nthat you struggling surviving\nthat you gonna persevere", ALIGN_RIGHT, -8, 8));
			 */
			break;

		case UI_WORLDMAP:
			uiData->map.scales = dynarr_create (mapGetSpan () + 1, sizeof (TEXTURE));
			uiWorldmapGenLevel (&uiData->map, 0);
			uiData->map.currentScale = 0;
			break;

		case UI_MENU:
			uiData->menu.fragments = dynarr_create (3, sizeof (struct uiTextFragment *));
			uiData->menu.menuOpt = dynarr_create (3, sizeof (struct uiMenuOpt *));
			uiData->menu.frame = uiFrame_createEmpty ();
			uiData->menu.lastIndex = 0;
			uiData->menu.activeIndex = 0;
			uiData->menu.timer = timerCreate ();
			uiData->menu.fadeScale = 6.0;
			//timerPause (uiData->menu.timer);
			break;

		default:
			WARNING ("Can't set #%d's UI type; invalid type of %d given", entity_GUID (uiEntity), type);
			uiData->type = UI_NONE;
			break;
	}
}

void ui_getType (EntComponent ui, EntSpeech speech)
{
	UIPANEL
		uiData = component_getData (ui);
	enum uiPanelTypes
		* type = speech->arg;
	*type = uiData->type;
}

void ui_addValue (EntComponent ui, EntSpeech speech)
{
	UIPANEL
		uiData = component_getData (ui);
	struct uiFrame
		* frame;
	Dynarr
		fragments;
	int
		x, y,
		yOffset = 0;
	if (uiData->type != UI_STATICTEXT && uiData->type != UI_MENU)
	{
		// TODO: maybe warn here?
		return;
	}
	if (uiData->type == UI_STATICTEXT)
	{
		fragments = uiData->staticText.fragments;
		frame = uiData->staticText.frame;
	}
	else
	{
		fragments = uiData->menu.fragments;
		frame = uiData->menu.frame;
	}

	uiFrame_getXY (frame, &x, &y);

	yOffset = dynarr_size (fragments) * systemLineHeight ();
	dynarr_push (fragments, uiFragmentCreate (speech->arg, ALIGN_LEFT, x, y + yOffset));
	if (uiData->type == UI_MENU)
	{
		dynarr_push (uiData->menu.menuOpt, uiMenuOpt_createEmpty ());
	}
}

void ui_setAction (EntComponent ui, EntSpeech speech)
{
	UIPANEL
		uiData = component_getData (ui);
	struct uiMenuOpt
		* opt;
	enum input_responses
		action = (enum input_responses)speech->arg;

	if (uiData->type != UI_MENU)
		return;
	opt = *(struct uiMenuOpt **)dynarr_back (uiData->menu.menuOpt);
	opt->action = action;
}

void ui_setPositionType (EntComponent ui, EntSpeech speech)
{
	UIPANEL
		uiData = component_getData (ui);
	enum uiFramePos
		posType = (enum uiFramePos)speech->arg;
	struct uiFrame
		* frame;
	Dynarr
		fragments;
	struct uiTextFragment
		* text;
	signed int
		x, y,
		i = 0;
	if (uiData->type != UI_STATICTEXT && uiData->type != UI_MENU)
	{
		return;
	}
	if (uiData->type == UI_STATICTEXT)
	{
		frame = uiData->staticText.frame;
		fragments = uiData->staticText.fragments;
	}
	else
	{
		frame = uiData->menu.frame;
		fragments = uiData->menu.fragments;
	}

	if (posType & PANEL_X_MASK)
	{
		frame->positionType &= ~PANEL_X_MASK;
		frame->positionType |= (posType & PANEL_X_MASK);
	}
	if (posType & PANEL_Y_MASK)
	{
		frame->positionType &= ~PANEL_Y_MASK;
		frame->positionType |= (posType & PANEL_Y_MASK);
	}

	uiFrame_getXY (frame, &x, &y);
	while ((text = *(struct uiTextFragment **)dynarr_at (fragments, i)) != NULL)
	{
		text->xAlign = x;
		text->yAlign = y + frame->lineSpacing * i;
		i++;
	}
}

void ui_setBackground (EntComponent ui, EntSpeech speech)
{
	UIPANEL
		uiData = component_getData (ui);
	enum uiFrameBackground
		bg = (enum uiFrameBackground)speech->arg;
	struct uiFrame
		* frame;
	if (uiData->type != UI_STATICTEXT && uiData->type != UI_MENU)
	{
		return;
	}
	if (uiData->type == UI_STATICTEXT)
		frame = uiData->staticText.frame;
	else
		frame = uiData->menu.frame;

	frame->background = bg;
}

void ui_setBorder (EntComponent ui, EntSpeech speech)
{
	UIPANEL
		uiData = component_getData (ui);
	struct uiFrame
		* frame;
	signed int
		border = (signed int)speech->arg;

	if (uiData->type != UI_MENU && uiData->type != UI_STATICTEXT)
		return;
	if (uiData->type == UI_STATICTEXT)
		frame = uiData->staticText.frame;
	else
		frame = uiData->menu.frame;

	frame->border = border;
}

void ui_setLineSpacing (EntComponent ui, EntSpeech speech)
{
	UIPANEL
		uiData = component_getData (ui);
	struct uiFrame
		* frame;
	signed int
		lineSpacing = (signed int)speech->arg;

	if (uiData->type != UI_MENU && uiData->type != UI_STATICTEXT)
		return;
	if (uiData->type == UI_STATICTEXT)
		frame = uiData->staticText.frame;
	else
		frame = uiData->menu.frame;

	frame->lineSpacing = systemLineHeight () + lineSpacing;

	/* if we change the spacing we need to recalculate the text fragments - xph 2011 09 27 */
	entity_message (component_entityAttached (ui), NULL, "setPosType", (void *)frame->positionType);
}

void ui_setXPosition (EntComponent ui, EntSpeech speech)
{
	UIPANEL
		uiData = component_getData (ui);
	struct uiFrame
		* frame;
	signed int
		xPos = (signed int)speech->arg;

	if (uiData->type != UI_MENU && uiData->type != UI_STATICTEXT)
		return;
	if (uiData->type == UI_STATICTEXT)
		frame = uiData->staticText.frame;
	else
		frame = uiData->menu.frame;


	if ((frame->positionType & PANEL_X_MASK) != PANEL_X_FREE)
		return;
	frame->x = xPos;
}

void ui_setYPosition (EntComponent ui, EntSpeech speech)
{
	UIPANEL
		uiData = component_getData (ui);
	struct uiFrame
		* frame;
	signed int
		yPos = (signed int)speech->arg;


	if (uiData->type != UI_MENU && uiData->type != UI_STATICTEXT)
		return;
	if (uiData->type == UI_STATICTEXT)
		frame = uiData->staticText.frame;
	else
		frame = uiData->menu.frame;

	if ((frame->positionType & PANEL_Y_MASK) != PANEL_Y_FREE)
		return;
	frame->y = yPos;
}

void ui_setWidth (EntComponent ui, EntSpeech speech)
{
	UIPANEL
		uiData = component_getData (ui);
	struct uiFrame
		* frame;

	signed int
		width = (signed int)speech->arg;

	if (uiData->type != UI_MENU && uiData->type != UI_STATICTEXT)
		return;
	if (uiData->type == UI_STATICTEXT)
		frame = uiData->staticText.frame;
	else
		frame = uiData->menu.frame;

	frame->xMargin = width;
	if (frame->positionType & PANEL_X_CENTER)
	{
		/* if we change the width and this is a centered frame we need to recalculate the text fragments - xph 2011 09 26*/
		entity_message (component_entityAttached (ui), NULL, "setPosType", (void *)frame->positionType);
	}
}

void ui_handleInput (EntComponent ui, EntSpeech speech)
{
	UIPANEL
		uiData = component_getData (ui);
	struct input_event
		* inputEvent = speech->arg;
	struct uiMenuOpt
		* opt;
	signed int
		i = 0;
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
		case UI_MENU:
			switch (inputEvent->ir)
			{
				case IR_UI_MENU_INDEX_DOWN:
					if (uiData->menu.activeIndex < dynarr_size (uiData->menu.fragments) - 1)
					{
						uiMenu_setActiveIndex (uiData, uiData->menu.activeIndex + 1);
					}
					break;
				case IR_UI_MENU_INDEX_UP:
					if (uiData->menu.activeIndex > 0)
					{
						uiMenu_setActiveIndex (uiData, uiData->menu.activeIndex - 1);
					}
					break;
				case IR_UI_CONFIRM:
					opt = *(struct uiMenuOpt **)dynarr_at (uiData->menu.menuOpt, uiData->menu.activeIndex);
					input_sendAction (opt->action);
					break;
				case IR_UI_MOUSEMOVE:
					i = uiMenu_mouseHit (uiData, inputEvent->event->motion.x, inputEvent->event->motion.y);
					if (i < 0)
						break;
					uiMenu_setActiveIndex (uiData, i);
					break;
				case IR_UI_MOUSECLICK:
					i = uiMenu_mouseHit (uiData, inputEvent->event->button.x, inputEvent->event->button.y);
					if (i < 0)
						break;
					if (i != uiData->menu.activeIndex)
					{
						// ?!
						uiMenu_setActiveIndex (uiData, i);
					}
					input_sendAction (IR_UI_CONFIRM);
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
}

void ui_draw (EntComponent ui, EntSpeech speech)
{
	UIPANEL
		uiData = component_getData (ui);

	struct uiFrame
		* frame = NULL;
	struct uiTextFragment
		* fragment = NULL;
	struct uiMenuOpt
		* opt = NULL;
	int
		x, y,
		w, h,
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

		case UI_MENU:
			frame = uiData->menu.frame;
			ui_getXY (uiData, &x, &y);
			ui_getWH (uiData, &w, &h);
			if (frame->background != FRAMEBG_TRANSPARENT)
			{
				glColor3f (0.0, 0.0, 0.0);
				uiDrawPane (x, y, w, h, NULL);
			}
			while ((fragment = *(struct uiTextFragment **)dynarr_at (uiData->menu.fragments, i)) != NULL)
			{
				if (i == uiData->menu.activeIndex)
				{
					glColor3ub (0xff, 0xff, 0xff);
				}
				else
				{
					opt = *(struct uiMenuOpt **)dynarr_at (uiData->menu.menuOpt, i);
					glColor3f (
						0.8 + opt->highlight * 0.2,
						0.8 + opt->highlight * 0.2,
						0.8 + opt->highlight * 0.2
					);
				}
				textAlign (fragment->align);
				drawLine (fragment->text, fragment->xAlign, fragment->yAlign);
				i++;

			}
			ui_drawIndex (&uiData->menu);
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
			uiFrame_destroy (ui->staticText.frame);
			break;

		case UI_WORLDMAP:
			dynarr_map (ui->map.scales, (void (*)(void *))textureDestroy);
			dynarr_destroy (ui->map.scales);
			break;

		case UI_MENU:
			dynarr_map (ui->menu.fragments, uiFragmentDestroy);
			dynarr_destroy (ui->menu.fragments);
			dynarr_map (ui->menu.menuOpt, (void (*)(void *))uiMenuOpt_destroy);
			dynarr_destroy (ui->menu.menuOpt);
			uiFrame_destroy (ui->menu.frame);
			break;
		default:
			break;
	}

	xph_free (ui);
}

static struct uiFrame * uiFrame_createEmpty ()
{
	struct uiFrame
		* frame = xph_alloc (sizeof (struct uiFrame));
	memset (frame, 0, sizeof (struct uiFrame));
	frame->positionType = PANEL_X_CENTER | PANEL_Y_CENTER;
	frame->background = FRAMEBG_TRANSPARENT;
	frame->lineSpacing = systemLineHeight ();
	return frame;
}

static void uiFrame_destroy (struct uiFrame * frame)
{
	xph_free (frame);
}

static void uiFrame_getXY (struct uiFrame * frame, signed int * x, signed int * y)
{
	if (x != NULL)
	{
		switch (frame->positionType & PANEL_X_MASK)
		{
			case PANEL_X_FREE:
				*x = frame->x;
				break;
			case PANEL_X_ALIGN_14:
				*x = width / 4;
				break;
			case PANEL_X_ALIGN_12:
				*x = width / 2;
				break;
			case PANEL_X_ALIGN_34:
				*x = width * .75;
				break;
			case PANEL_X_CENTER:
				*x = width / 2 - frame->xMargin / 2;
				break;
			default:
				*x = 0;
				break;
		}
	}

	if (y != NULL)
	{
		switch (frame->positionType & PANEL_Y_MASK)
		{
			case PANEL_Y_FREE:
				*y = frame->y;
				break;
			case PANEL_Y_ALIGN_13:
				*y = height / 3;
				break;
			case PANEL_Y_ALIGN_12:
				*y = height / 2;
				break;
			case PANEL_Y_ALIGN_23:
				*y = height / 1.5;
				break;
			case PANEL_Y_CENTER:
				 /* FIXME: currently height is implicitly calculated using "systemLineHeight () * dynarr_size(fragments)"; that makes it impossible to calculate the proper centered height here. also that's a bad way of storing frame height. */
				*y = height / 2;
				break;
			default:
				*y = 0;
				break;
		}
	}
}

static struct uiTextFragment * uiFragmentCreate (const char * text, enum textAlignType align, signed int x, signed int y)
{
	struct uiTextFragment
		* uit = xph_alloc (sizeof (struct uiTextFragment));
	if (text != NULL)
	{
		uit->text = xph_alloc (strlen (text) + 1);
		strcpy (uit->text, text);
		uit->reservedText = false;
	}
	else
	{
		uit->text = NULL;
		uit->reservedText = true;
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

static struct uiMenuOpt * uiMenuOpt_createEmpty ()
{
	struct uiMenuOpt
		* opt = xph_alloc (sizeof (struct uiMenuOpt));
	opt->highlight = 0.0;
	opt->action = IR_NOTHING;

	return opt;
}

static void uiMenuOpt_destroy (struct uiMenuOpt * opt)
{
	xph_free (opt);
}

static void uiMenu_setActiveIndex (UIPANEL ui, unsigned int index)
{
	struct uiMenuOpt
		* opt;
	if (ui->type != UI_MENU)
		return;
	if (index == ui->menu.activeIndex)
		return;

	opt = *(struct uiMenuOpt **)dynarr_at (ui->menu.menuOpt, ui->menu.activeIndex);
	opt->highlight = 1.0;
	ui->menu.lastIndex = ui->menu.activeIndex;
	ui->menu.activeIndex = index;
	timerSetGoal (ui->menu.timer, 0.20);
	timerStart (ui->menu.timer);
}

static signed int uiMenu_mouseHit (UIPANEL ui, signed int mx, signed int my)
{
	signed int
		x, y,
		w, h,
		index = 0,
		hitHeight,
		hitY;
	if (ui->type != UI_MENU)
		return -1;

	ui_getXY (ui, &x, &y);
	ui_getWH (ui, &w, &h);
	if (mx < x || mx > x + w || my < y || my > y + h)
		return -1;
	hitHeight = ui->menu.frame->lineSpacing;
	hitY = y + ui->menu.frame->border;

	while (my > hitY)
	{
		if (index >= dynarr_size (ui->menu.fragments))
			break;
		if (my < hitY + hitHeight)
			return index;
		hitY += hitHeight;
		index++;
	}
	return -1;
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
		sx = video_pixelXOffset (SystemPanel->spriteSize),
		sy = video_pixelYOffset (SystemPanel->spriteSize),
		zNear = video_getZnear () - 0.001,
		tx, ty, tw, th,
		xRepeat, yRepeat;

	glColor3ub (0xff, 0xff, 0xff);
	glBindTexture (GL_TEXTURE_2D, textureName (sheetGetTexture (SystemPanel->base)));


	yRepeat = (float)(height - SystemPanel->spriteSize * 2) / SystemPanel->spriteSize;
	xRepeat = (float)(width - SystemPanel->spriteSize * 2) / SystemPanel->spriteSize;

	glBegin (GL_QUADS);

	sheetGetCoordinateFVals (SystemPanel->base, 0, 0, &tx, &ty, &tw, &th);
	glTexCoord2f (tx, ty);
	glVertex3f (left, top, zNear);
	glTexCoord2f (tx, ty + th);
	glVertex3f (left, top + sy, zNear);
	glTexCoord2f (tx + tw, ty + th);
	glVertex3f (left + sx, top + sy, zNear);
	glTexCoord2f (tx + tw, ty);
	glVertex3f (left + sx, top, zNear);

	sheetGetCoordinateFVals (SystemPanel->base, 0, 2, &tx, &ty, &tw, &th);
	glTexCoord2f (tx, ty);
	glVertex3f (left, bottom - sy, zNear);
	glTexCoord2f (tx, ty + th);
	glVertex3f (left, bottom, zNear);
	glTexCoord2f (tx + tw, ty + th);
	glVertex3f (left + sx, bottom, zNear);
	glTexCoord2f (tx + tw, ty);
	glVertex3f (left + sx, bottom - sy, zNear);

	sheetGetCoordinateFVals (SystemPanel->base, 2, 0, &tx, &ty, &tw, &th);
	glTexCoord2f (tx, ty);
	glVertex3f (right - sx, top, zNear);
	glTexCoord2f (tx, ty + th);
	glVertex3f (right - sx, top + sy, zNear);
	glTexCoord2f (tx + tw, ty + th);
	glVertex3f (right, top + sy, zNear);
	glTexCoord2f (tx + tw, ty);
	glVertex3f (right, top, zNear);

	sheetGetCoordinateFVals (SystemPanel->base, 2, 2, &tx, &ty, &tw, &th);
	glTexCoord2f (tx, ty);
	glVertex3f (right - sx, bottom - sy, zNear);
	glTexCoord2f (tx, ty + th);
	glVertex3f (right - sx, bottom, zNear);
	glTexCoord2f (tx + tw, ty + th);
	glVertex3f (right, bottom, zNear);
	glTexCoord2f (tx + tw, ty);
	glVertex3f (right, bottom - sy, zNear);

	glEnd ();


	glBindTexture (GL_TEXTURE_2D, textureName (SystemPanel->lrSide));
	glBegin (GL_QUADS);
	glTexCoord2f (0, 0);
	glVertex3f (left, top + sy, zNear);
	glTexCoord2f (0, yRepeat);
	glVertex3f (left, bottom - sy, zNear);
	glTexCoord2f (0.5, yRepeat);
	glVertex3f (left + sx, bottom - sy, zNear);
	glTexCoord2f (0.5, 0);
	glVertex3f (left + sx, top + sy, zNear);

	glTexCoord2f (0.5, 0);
	glVertex3f (right - sx, top + sy, zNear);
	glTexCoord2f (0.5, yRepeat);
	glVertex3f (right - sx, bottom - sy, zNear);
	glTexCoord2f (1.0, yRepeat);
	glVertex3f (right, bottom - sy, zNear);
	glTexCoord2f (1.0, 0);
	glVertex3f (right, top + sy, zNear);

	glEnd ();


	glBindTexture (GL_TEXTURE_2D, textureName (SystemPanel->tbSide));
	glBegin (GL_QUADS);
	glTexCoord2f (0, 0);
	glVertex3f (left + sx, top, zNear);
	glTexCoord2f (0, .5);
	glVertex3f (left + sx, top + sy, zNear);
	glTexCoord2f (xRepeat, .5);
	glVertex3f (right - sx, top + sy, zNear);
	glTexCoord2f (xRepeat, 0);
	glVertex3f (right - sx, top, zNear);

	glTexCoord2f (0, 0.5);
	glVertex3f (left + sx, bottom - sy, zNear);
	glTexCoord2f (0, 1.0);
	glVertex3f (left + sx, bottom, zNear);
	glTexCoord2f (xRepeat, 1.0);
	glVertex3f (right - sx, bottom, zNear);
	glTexCoord2f (xRepeat, 0.5);
	glVertex3f (right - sx, bottom - sy, zNear);
	glEnd ();

	
	glBindTexture (GL_TEXTURE_2D, textureName (SystemPanel->inside));
	glBegin (GL_QUADS);
	glTexCoord2f (0, 0);
	glVertex3f (left + sx, top + sy, zNear);
	glTexCoord2f (0, yRepeat);
	glVertex3f (left + sx, bottom - sy, zNear);
	glTexCoord2f (xRepeat, yRepeat);
	glVertex3f (right - sx, bottom - sy, zNear);
	glTexCoord2f (xRepeat, 0);
	glVertex3f (right - sx, top + sy, zNear);
	glEnd ();
}

static void ui_drawIndex (struct uiMenu * menu)
{
	signed int
		x, y,
		diff = 0;
	float
		top,
		left,
		bottom,
		right,
		topMovement = 0.0,
		zNear = video_getZnear () - 0.001,
		percentage,
		tx, ty, tw, th;
	uiFrame_getXY (menu->frame, &x, &y);
	if ((percentage = timerPercentageToGoal (menu->timer)) >= 1.0)
	{
		//timerPause (menu->timer);
		timerSetGoal (menu->timer, 0.0);
		top = video_pixelYMap (
			y +
			(menu->frame->lineSpacing - SystemPanel->spriteSize) / 2 +
			(menu->frame->lineSpacing * menu->activeIndex)
		);
	}
	else
	{
		diff = menu->lastIndex - menu->activeIndex;
		topMovement = (1 - ((sin (percentage * M_PI - M_PI_2) + 1) / 2.0)) * menu->frame->lineSpacing * diff;
		top = video_pixelYMap (
			y +
			(menu->frame->lineSpacing - SystemPanel->spriteSize) / 2 +
			(menu->frame->lineSpacing * menu->activeIndex) +
			topMovement
		);
	}

	left = video_pixelXMap (x - menu->frame->border - SystemPanel->spriteSize * 1.25);
	bottom = top + video_pixelYOffset (SystemPanel->spriteSize);
	right = left + video_pixelXOffset (SystemPanel->spriteSize);

	glColor3ub (0xff, 0xff, 0xff);
	glBindTexture (GL_TEXTURE_2D, textureName (sheetGetTexture (SystemPanel->base)));
	sheetGetCoordinateFVals (SystemPanel->base, 2, 3, &tx, &ty, &tw, &th);

	glBegin (GL_TRIANGLE_FAN);
	glTexCoord2f (tx, ty);
	glVertex3f (left, top, zNear);
	glTexCoord2f (tx, ty + th);
	glVertex3f (left, bottom, zNear);
	glTexCoord2f (tx + tw, ty + th);
	glVertex3f (right, bottom, zNear);
	glTexCoord2f (tx + tw, ty);
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
