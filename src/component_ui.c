#include "component_ui.h"

#include "video.h"
#include "font.h"

#include "system.h"

struct uiTextFragment
{
	char
		* text;
	signed int
		xAlign,
		yAlign;
	enum textAlignType
		align;
};

struct uiStaticText
{
	enum uiPanelTypes
		type;

	Dynarr
		fragments;
};

union uiPanels
{
	enum uiPanelTypes
		type;
	struct uiStaticText
		staticText;
};

static unsigned int
	height,
	width;

static UIPANEL uiPanel_createEmpty ();
static void uiPanel_destroy (UIPANEL ui);

static struct uiTextFragment * uiFragmentCreate (const char * text, enum textAlignType align, signed int x, signed int y);
static void uiFragmentDestroy (void * v);


void uiDrawCursor ()
{
	unsigned int
		height = 0,
		width = 0,
		centerHeight,
		centerWidth,
		halfTexHeight,
		halfTexWidth;
	float
		zNear,
		top, bottom,
		left, right;
/*
	if (t == NULL)
		return;
*/
	if (!video_getDimensions (&width, &height))
		return;
	zNear = video_getZnear () - 0.001;
	centerHeight = height / 2;
	centerWidth = width / 2;
	halfTexHeight = 2;//texture_pxHeight (t) / 2;
	halfTexWidth = 2;//texture_pxWidth (t) / 2;
	top = video_pixelYMap (centerHeight + halfTexHeight);
	bottom = video_pixelYMap (centerHeight - halfTexHeight);
	left = video_pixelXMap (centerWidth - halfTexWidth);
	right = video_pixelXMap (centerWidth + halfTexWidth);
	//printf ("top: %7.2f; bottom: %7.2f; left: %7.2f; right: %7.2f\n", top, bottom, left, right);
	glPushMatrix ();
	glLoadIdentity ();
	glColor3f (1.0, 1.0, 1.0);
	//glBindTexture (GL_TEXTURE_2D, texture_glID (t));
	glBegin (GL_TRIANGLE_STRIP);
	glVertex3f (top, left, zNear);
	glVertex3f (bottom, left, zNear);
	glVertex3f (top, right, zNear);
	glVertex3f (bottom, right, zNear);
	glEnd ();
	glPopMatrix ();
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

	if (uiData->type != UI_NONE)
	{
		WARNING ("Entity #%d already has a UI type of %d; overwriting", entity_GUID (uiEntity), uiData->type);
	}

	uiData->type = type;
	switch (type)
	{
		case UI_STATICTEXT:
			uiData->staticText.fragments = dynarr_create (3, sizeof (struct uiTextFragment *));

		case UI_DEBUG_OVERLAY:
			uiData->staticText.fragments = dynarr_create (3, sizeof (struct uiTextFragment *));

			dynarr_assign (uiData->staticText.fragments, 0, uiFragmentCreate (systemGenDebugStr (), ALIGN_LEFT, 8, 8));

			dynarr_assign (uiData->staticText.fragments, 1, uiFragmentCreate ("raise em til your arms tired\nlet em know you here\nthat you struggling surviving\nthat you gonna persevere", ALIGN_RIGHT, -8, 8));
			break;

		case UI_WORLDMAP:
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
		case UI_STATICTEXT:
			while ((fragment = *(struct uiTextFragment **)dynarr_at (uiData->staticText.fragments, i++)) != NULL)
			{
				textAlign (fragment->align);
				drawLine (fragment->text, fragment->xAlign, fragment->yAlign);
			}
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
		default:
			break;
	}

	xph_free (ui);
}


static struct uiTextFragment * uiFragmentCreate (const char * text, enum textAlignType align, signed int x, signed int y)
{
	struct uiTextFragment
		* uit = xph_alloc (sizeof (struct uiTextFragment));
	uit->text = xph_alloc (strlen (text) + 1);
	strcpy (uit->text, text);
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
	xph_free (uit->text);
	xph_free (uit);
}