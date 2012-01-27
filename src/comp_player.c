#include "comp_player.h"

#include "component_input.h"
#include "video.h"

void player_input (EntComponent comp, EntSpeech speech);

void player_define (EntComponent comp, EntSpeech speech)
{
	component_registerResponse ("player", "FOCUS_INPUT", player_input);
}

void player_input (EntComponent comp, EntSpeech speech)
{
	Entity
		worldmap;
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
			break;
		case IR_DEBUG_SWITCH:
			entitySystem_message ("ui", NULL, "DEBUGOVERLAY_SWITCH", NULL);
			break;

		default:
			break;
	}
}
