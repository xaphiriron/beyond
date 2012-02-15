/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#include "comp_player.h"

#include "component_input.h"
#include "video.h"

#define DEBUGLEN	2048
char
	debugDisplay[DEBUGLEN];
#include "font.h"
#include "comp_gui.h"
Text
	debugLyrics = NULL;

static void debug_define (EntComponent comp, EntSpeech speech);
static void debug_update (Dynarr entities);
static void debug_open ();
char * systemGenDebugStr ();

void player_input (EntComponent comp, EntSpeech speech);

void player_define (EntComponent comp, EntSpeech speech)
{
	component_registerResponse ("player", "FOCUS_INPUT", player_input);

	component_register ("debug", debug_define);
	entitySystem_register ("debug", debug_update, 0);
}

void player_input (EntComponent comp, EntSpeech speech)
{
	Entity
		worldmap,
		debugInfo;
	inputEvent
		* event = speech->arg;

	if (!event->active)
		return;
	switch (event->code)
	{
		case IR_VIEW_WIREFRAME_SWITCH:
			video_wireframeSwitch ();
			break;
		case IR_WORLDMAP_SWITCH:
			printf ("focusing map\n");
			worldmap = entity_getByName ("WORLDMAP");
			entity_messageGroup ("PlayerAvatarEntities", NULL, "loseFocus", NULL);
			entity_message (worldmap, NULL, "gainFocus", NULL);
			gui_placeOnStack (worldmap);
			break;
		case IR_DEBUG_SWITCH:
			debugInfo = entity_getByName ("DEBUG_OVERLAY");
			if (debugInfo)
				entity_destroy (debugInfo);
			else
				debug_open ();
			break;

		default:
			break;
	}
}

static void debug_draw (EntComponent comp, EntSpeech speech)
{
	fontPrintAlign (ALIGN_LEFT);
	fontPrint (debugDisplay, 0, 0);
	fontTextPrint (debugLyrics);
}

static void debug_define (EntComponent comp, EntSpeech speech)
{
	component_registerResponse ("debug", "guiDraw", debug_draw);
}

static void debug_update (Dynarr entities)
{
	Entity
		debugInfo = entity_getByName ("DEBUG_OVERLAY");
	if (debugInfo)
		systemGenDebugStr ();
}

static void debug_open ()
{
	unsigned int
		width, height;
	Entity
		debug;
	video_getDimensions (&width, &height);
	debug = entity_create ();
	component_instantiate ("gui", debug);
	component_instantiate ("debug", debug);
	entity_refresh (debug);
	entity_name (debug, "DEBUG_OVERLAY");

	gui_placeOnStack (debug);

	systemGenDebugStr ();
	debugLyrics = fontGenerate ("i know\nthe ocean floor was open long ago\nand currents started forming long ago\nwater began circling long ago\nthis ship started sinking long ago", ALIGN_RIGHT, width - 4, 4, width - 8);

	//debugLyrics = fontGenerate ("so make your sirens call\nand sing all you want", ALIGN_RIGHT, width - 4, 4, width - 8);

	//dynarr_assign (uiData->staticText.fragments, 1, uiFragmentCreate ("raise em til your arms tired\nlet em know you here\nthat you struggling surviving\nthat you gonna persevere", ALIGN_RIGHT, -8, 8));
}

#include "map.h"
#include "component_position.h"
#include <ctype.h>
char * systemGenDebugStr ()
{
	signed int
		x = 0,
		y = 0,
		len = 0,
		span = mapGetSpan (),
		radius = mapGetRadius ();
	unsigned int
		height = 0;
	Entity
		player = entity_getByName ("PLAYER"),
		camera = entity_getByName ("CAMERA");
	hexPos
		position = position_get (player);
	SUBHEX
		platter;
	unsigned char
		focus = hexPos_focus (position),
		i = 0;
	char
		buffer[64];
	
	memset (debugDisplay, 0, DEBUGLEN);
	len = snprintf (buffer, 63, "Player Entity: #%d\nCamera Entity: #%d\n\n", entity_GUID (player), entity_GUID (camera));
	strncpy (debugDisplay, buffer, len);


	len += snprintf (buffer, 63, "scale: %d,%d\n\ton %c:\n", span, radius, toupper (subhexPoleName (hexPos_platter (position, span))));
	strncat (debugDisplay, buffer, DEBUGLEN - len);

	i = span;
	while (i > 0)
	{
		i--;
		if (i < focus)
		{
			len += snprintf (buffer, 63, "\t%d: **, **\n", i);
			strncat (debugDisplay, buffer, DEBUGLEN - len);
			continue;
		}
		platter = hexPos_platter (position, i);
		subhexLocalCoordinates (platter, &x, &y);
		len += snprintf (buffer, 63, "\t%d: %d, %d\n", i, x, y);
		strncat (debugDisplay, buffer, DEBUGLEN - len);
	}
	height = position_height (player);
	len += snprintf (buffer, 63, "\nheight: %u\n", height);
	strncat (debugDisplay, buffer, DEBUGLEN - len);

	return debugDisplay;
}