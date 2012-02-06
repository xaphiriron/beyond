#include "component_input.h"

#include "video.h"
#include "system.h"
#include "ogdl/ogdl.h"
#include "xph_path.h"

#include "graph_common.h"

struct input
{
	SDL_Event
		event;

	Dynarr
		controlMap,			// (Keycode *) which keys are mapped to
							// which commands.
		focusedEntities;

	bool
		active,
		textMode;
};

typedef union xph_key
{
	enum xph_keytype
	{
		XPH_SDL,
		XPH_EXTRA,
	}
		type;
	struct xph_sdlkey
	{
		enum xph_keytype
			type;
		SDLKey
			key;
	}
		sdl;
	struct xph_extrakey
	{
		enum xph_keytype
			type;
		enum xph_extrakeys
		{
			XKEY_CTRL,
			XKEY_SHIFT,
			XKEY_ALT,
			XKEY_SUPER,
			XKEY_META,
		}
			key;
	}
		extra;
} Keycode;

static INPUT
	Input = NULL;

static Keycode * key_create (const char * name);
static bool key_match (const Keycode * key, SDLKey sym);

static void input_loadConfig (const Graph config);
static void input_loadDefaults ();


struct input * input_create ()
{
	struct input * i = xph_alloc (sizeof (struct input));
	i->controlMap = dynarr_create (1, sizeof (Keycode *));
	i->focusedEntities = dynarr_create (2, sizeof (Entity *));

	i->active = true;
	i->textMode = true;

	return i;
}

void input_destroy (struct input * i)
{
	dynarr_map (i->controlMap, xph_free);
	dynarr_destroy (i->controlMap);

	dynarr_destroy (i->focusedEntities);
	xph_free (i);
}

bool input_addEntity (Entity e, enum input_control_types t)
{
	assert (Input != NULL);
	// THIS FUNCTION ISN'T NEEDED ANYMORE; INSTANTIATING AN INPUT COMPONENT AUTOMATICALLY ADDS AN ENTITY TO THE INPUT LIST
	return true;
}

bool input_rmEntity (Entity e, enum input_control_types t)
{
	assert (Input != NULL);
	dynarr_remove_condense (Input->focusedEntities, e);
	return true;
}

void input_sendAction (enum input_responses action)
{
	/* FIXME: this action will always be considered active. this isn't the best
	 * idea and i suspect it's possible to break the input system by sending
	 * e.g., a movement start action since it could only be deactivated by
	 * player input
	 *  - xph 2012 01 25
	 */
	struct input_event
		event;
	event.code = action;
	event.event = NULL;
	event.active = true;
	input_sendGameEventMessage (&event);
}

void input_sendGameEventMessage (const struct input_event * ie)
{
	int
		i = 0;
	Entity
		e = NULL;
	struct xph_input
		* inputData;
	Dynarr
		targetCache = NULL;
	// CATCH AND HANDLE EVENTS THAT HAVE SYSTEM-WIDE REPERCUSSIONS
	//DEBUG ("GOT INPUTEVENT TYPE %d", ie->ir);
	if (!Input->active)
		return;
	// this switch makes me really sad; there's got to be a better way to trigger game events than by doing things this way - xph 2012 01 31
	switch (ie->code)
	{
		case IR_QUIT:
			system_message (OM_SHUTDOWN, NULL, NULL);
			break;
		case IR_WORLDGEN:
			system_message (OM_FORCEWORLDGEN, NULL, NULL);
			break;
		case IR_OPTIONS:
			system_message (OM_OPTIONS, NULL, NULL);
			break;
		default:
			break;
	}
	// SEND MESSAGES OFF TO WORLD ENTITIES

	targetCache = dynarr_create (4, sizeof (Entity));
	i = 0;
	while ((e = *(Entity *)dynarr_at (Input->focusedEntities, i++)) != NULL)
	{
		inputData = component_getData (entity_getAs (e, "input"));
		if (!inputData->hasFocus)
			continue;
		dynarr_push (targetCache, e);
	}
	i = 0;
	while ((e = *(Entity *)dynarr_at (targetCache, i++)))
	{
		entity_message (e, NULL, "FOCUS_INPUT", (void *)ie);
	}
	dynarr_clear (targetCache);
	dynarr_destroy (targetCache);
}

bool input_hasFocus (Entity e)
{
	struct xph_input
		* input = component_getData (entity_getAs (e, "input"));
	if (!input)
		return false;
	return input->hasFocus;
}

static void input_classDestroy (EntComponent comp, EntSpeech speech);

static void input_componentCreate (EntComponent comp, EntSpeech speech);
static void input_componentDestroy (EntComponent comp, EntSpeech speech);

static void input_gainFocus (EntComponent comp, EntSpeech speech);
static void input_loseFocus (EntComponent comp, EntSpeech speech);

void input_define (EntComponent inputComponent, EntSpeech speech)
{
	component_registerResponse ("input", "__classDestroy", input_classDestroy);

	component_registerResponse ("input", "__create", input_componentCreate);
	component_registerResponse ("input", "__destroy", input_componentDestroy);

	component_registerResponse ("input", "gainFocus", input_gainFocus);
	component_registerResponse ("input", "loseFocus", input_loseFocus);

	Input = input_create ();
	if (System->config)
		input_loadConfig (System->config);
	else
		input_loadDefaults ();
}

static void input_classDestroy (EntComponent comp, EntSpeech speech)
{
	input_destroy (Input);
	Input = NULL;
}

static void input_componentCreate (EntComponent comp, EntSpeech speech)
{
	Entity
		this = component_entityAttached (comp);
	struct xph_input
		* input = xph_alloc (sizeof (struct xph_input));

	component_setData (comp, input);

	dynarr_push (Input->focusedEntities, this);
}

static void input_componentDestroy (EntComponent comp, EntSpeech speech)
{
	Entity
		this = component_entityAttached (comp);
	struct xph_input
		* input = component_getData (comp);
	xph_free (input);

	dynarr_remove_condense (Input->focusedEntities, this);
}

static void input_gainFocus (EntComponent comp, EntSpeech speech)
{
	struct xph_input
		* input = component_getData (comp);
	input->hasFocus = true;
}

static void input_loseFocus (EntComponent comp, EntSpeech speech)
{
	struct xph_input
		* input = component_getData (comp);
	input->hasFocus = false;
}


void input_system (Dynarr entities)
{
	int
		i = 0;
	inputEvent
		event;

	//DEBUG ("INPUT UPDATE HAPPENING NOW\n", NULL);
	while (SDL_PollEvent (&Input->event))
	{
		event.code = IR_NOTHING;
		event.event = &Input->event;
		event.active = true;
		switch (Input->event.type)
		{
			case SDL_MOUSEMOTION:
				event.code = IR_MOUSEMOVE;
				input_sendGameEventMessage (&event);
				break;
			case SDL_MOUSEBUTTONDOWN:
				event.code = IR_MOUSECLICK;
				input_sendGameEventMessage (&event);
				break;
			case SDL_MOUSEBUTTONUP:
				event.code = IR_MOUSECLICK;
				event.active = false;
				input_sendGameEventMessage (&event);
				break;

			case SDL_KEYDOWN:
				i = 0;
				while (i < IR_FINAL)
				{
					if (key_match (*(Keycode **)dynarr_at (Input->controlMap, i), Input->event.key.keysym.sym))
					{
						event.event = &Input->event;
						event.code = i;
						event.active = true;
						input_sendGameEventMessage (&event);
					}
					i++;
				}
				if (Input->textMode)
				{
					event.event = &Input->event;
					event.code = IR_TEXT;
					event.active = true;
					input_sendGameEventMessage (&event);
				}
				break;
			case SDL_KEYUP:
				i = 0;
				while (i < IR_FINAL)
				{
					if (key_match (*(Keycode **)dynarr_at (Input->controlMap, i), Input->event.key.keysym.sym))
					{
						event.event = &Input->event;
						event.code = i;
						event.active = false;
						input_sendGameEventMessage (&event);
					}
					i++;
				}
				break;
			case SDL_ACTIVEEVENT:
				if (Input->event.active.state & SDL_APPINPUTFOCUS)
				{
					if (Input->event.active.gain)
					{
						Input->active = true;
						if (systemState () == STATE_FREEVIEW)
							SDL_ShowCursor (SDL_DISABLE);
					}
					else
					{
						Input->active = false;
						if (systemState () == STATE_FREEVIEW)
							SDL_ShowCursor (SDL_ENABLE);
					}
				}
				break;
			case SDL_QUIT:
				system_message (OM_SHUTDOWN, NULL, NULL);
				break;
			default:
				if ((SDL_GetAppState () & (SDL_APPMOUSEFOCUS | SDL_APPINPUTFOCUS)) == (SDL_APPMOUSEFOCUS | SDL_APPINPUTFOCUS))
				{
					event.code = IR_NOTHING;
					input_sendGameEventMessage (&event);
				}
				break;
		}
	}
}


static void input_loadConfig (const Graph config)
{
	// this requires the same indexing as the inputCodes enum in component_input.h
	static char
		* inputCodePaths [] =
	{
		"", // IR_NOTHING
		".keys.world.forward", // IR_FREEMOVE_MOVE_FORWARD
		".keys.world.backward", // IR_FREEMOVE_MOVE_BACKWARD
		".keys.world.left", // IR_FREEMOVE_MOVE_LEFT
		".keys.world.right", // IR_FREEMOVE_MOVE_RIGHT
		".keys.world.automove", // IR_FREEMOVE_AUTOMOVE
		"", // IR_MOUSEMOVE
		"", // IR_MOUSECLICK
		"", // IR_TEXT
		".keys.world.camera_mode", // IR_CAMERA_MODE_SWITCH
		".keys.wireframe_mode", // IR_VIEW_WIREFRAME_SWITCH
		".keys.worldmap", // IR_WORLDMAP_SWITCH
		".keys.position_debug", // IR_DEBUG_SWITCH
		".keys.ui.up", // IR_UI_MENU_INDEX_UP
		".keys.ui.down", // IR_UI_MENU_INDEX_DOWN
		".keys.ui.cancel", // IR_UI_CANCEL
		".keys.ui.confirm", // IR_UI_CONFIRM
		".keys.ui.mode", // IR_UI_MODE_SWITCH
		".keys.arch_test", // IR_WORLD_PLACEARCH
		"", // IR_WORLDGEN
		"", // IR_OPTIONS
		".keys.quit", // IR_QUIT
		"", // IR_FINAL
	};

	int
		i = 0;
	Graph
		node,
		keybind;
	Keycode
		* key = NULL;

	Graph_print (config);
	while (i < IR_FINAL)
	{
		if (inputCodePaths[i][0] == 0 || !(node = Graph_get (config, inputCodePaths[i])))
		{
			i++;
			continue;
		}
		keybind = Graph_get (node, "[0]");
		if (!keybind)
		{
			ERROR ("Key path \"%s\" has no binding; can't use", inputCodePaths[i]);
			i++;
			continue;
		}
		printf ("%s: %s\n", inputCodePaths[i], keybind->name);
		key = key_create (keybind->name);
		if (!key)
		{
			ERROR ("Unknown key \"%s\"; can't use", keybind->name);
			i++;
			continue;
		}
		dynarr_assign (Input->controlMap, i, key);
		i++;
	}
}

static void input_loadDefaults ()
{
	Graph
		fakeConfig = Graph_new ("__root__"),
		keys,
		world,
		ui;

	Graph_set (fakeConfig, ".", Graph_new ("keys"));
	keys = Graph_get (fakeConfig, "keys");

	Graph_set (keys, ".quit", Graph_new ("Escape"));
	Graph_set (keys, ".worldmap", Graph_new ("/"));
	Graph_set (keys, ".wireframe_mode", Graph_new ("W"));
	Graph_set (keys, ".position_debug", Graph_new ("F3"));
	Graph_set (keys, ".arch_test", Graph_new ("Z"));

	Graph_set (keys, ".world", NULL);
	world = Graph_get (keys, "world");
	Graph_set (world, ".left", Graph_new ("Left"));
	Graph_set (world, ".right", Graph_new ("Right"));
	Graph_set (world, ".forward", Graph_new ("Up"));
	Graph_set (world, ".backward", Graph_new ("Down"));
	Graph_set (world, ".automove", Graph_new ("Q"));
	Graph_set (world, ".camera_mode", Graph_new ("Tab"));

	Graph_set (keys, ".ui", NULL);
	ui = Graph_get (keys, "ui");
	Graph_set (ui, ".up", Graph_new ("Up"));
	Graph_set (ui, ".down", Graph_new ("Down"));
	Graph_set (ui, ".confirm", Graph_new ("Enter"));
	Graph_set (ui, ".cancel", Graph_new ("Space"));
	Graph_set (ui, ".mode", Graph_new ("Tab"));

	input_loadConfig (fakeConfig);
	Graph_free (fakeConfig);
}


static Keycode * key_create (const char * name)
{
	const char
	* inputKeys [] =
	{
		"!",
		"\"",
		"#",
		"$",
		"%",
		"&",
		"(",
		")",
		"*",
		"+",
		",",
		"-",
		".",
		"/",
		"0",
		"1",
		"2",
		"3",
		"4",
		"5",
		"6",
		"7",
		"8",
		"9",
		":",
		";",
		"<",
		"=",
		">",
		"?",
		"@",
		"[",
		"\\",
		"]",
		"^",
		"_",
		"`",
		"A",
		"B",
		"Backspace",
		"Creak",
		"C",
		"Caps lock",
		"Compose",
		"D",
		"Delete",
		"Down",
		"E",
		"End",
		"Enter",
		"Escape",
		"Euro",
		"F",
		"F1",
		"F10",
		"F11",
		"F12",
		"F13",
		"F14",
		"F15",
		"F2",
		"F3",
		"F4",
		"F5",
		"F6",
		"F7",
		"F8",
		"F9",
		"G",
		"H",
		"Help",
		"Home",
		"I",
		"Insert",
		"J",
		"K",
		"L",
		"Left",
		"Left Alt",
		"Left Ctrl",
		"Left Meta",
		"Left Shift",
		"Left Super",
		"M",
		"Menu",
		"Mode",
		"N",
		"Num Lock",
		"Numpad 0",
		"Numpad 1",
		"Numpad 2",
		"Numpad 3",
		"Numpad 4",
		"Numpad 5",
		"Numpad 6",
		"Numpad 7",
		"Numpad 8",
		"Numpad 9",
		"Numpad Divide",
		"Numpad Enter",
		"Numpad Equals",
		"Numpad Minus",
		"Numpad Multiply",
		"Numpad Period",
		"Numpad Plus",
		"O",
		"P",
		"Page Down",
		"Page Up",
		"Pause",
		"Power",
		"Print",
		"Print Screen",
		"Q",
		"R",
		"Return",
		"Right",
		"Right Alt",
		"Right Ctrl",
		"Right Meta",
		"Right Shift",
		"Right Super",
		"S",
		"Scroll Lock",
		"Space",
		"Sysrq",
		"T",
		"Tab",
		"U",
		"Undo",
		"Up",
		"V",
		"W",
		"X",
		"Y",
		"Z",

		""
	},
	* inputModKeys [] =
	{
		"Alt",
		"Ctrl",
		"Meta",
		"Shift",
		"Super",

		""
	};
	const int
	keyIDs [] =
	{
		SDLK_EXCLAIM,
		SDLK_QUOTEDBL,
		SDLK_HASH,
		SDLK_DOLLAR,
		SDLK_5,		// this would be SDLK_PERCENT but it doesn't exist; all the key codes that implicitly require shift + [key] to be generated are of dubious value since they should probably be generated by a shift + [key] config
		SDLK_AMPERSAND,
		SDLK_LEFTPAREN,
		SDLK_RIGHTPAREN,
		SDLK_ASTERISK,
		SDLK_PLUS,
		SDLK_COMMA,
		SDLK_MINUS,
		SDLK_PERIOD,
		SDLK_SLASH,
		SDLK_0,
		SDLK_1,
		SDLK_2,
		SDLK_3,
		SDLK_4,
		SDLK_5,
		SDLK_6,
		SDLK_7,
		SDLK_8,
		SDLK_9,
		SDLK_COLON,
		SDLK_SEMICOLON,
		SDLK_LESS,
		SDLK_EQUALS,
		SDLK_GREATER,
		SDLK_QUESTION,
		SDLK_AT,
		SDLK_LEFTBRACKET,
		SDLK_BACKSLASH,
		SDLK_RIGHTBRACKET,
		SDLK_CARET,
		SDLK_UNDERSCORE,
		SDLK_BACKQUOTE,
		SDLK_a,
		SDLK_b,
		SDLK_BACKSPACE,
		SDLK_BREAK,
		SDLK_c,
		SDLK_CAPSLOCK,
		SDLK_COMPOSE,
		SDLK_d,
		SDLK_DELETE,
		SDLK_DOWN,
		SDLK_e,
		SDLK_END,
		SDLK_RETURN,
		SDLK_ESCAPE,
		SDLK_EURO,
		SDLK_f,
		SDLK_F1,
		SDLK_F10,
		SDLK_F11,
		SDLK_F12,
		SDLK_F13,
		SDLK_F14,
		SDLK_F15,
		SDLK_F2,
		SDLK_F3,
		SDLK_F4,
		SDLK_F5,
		SDLK_F6,
		SDLK_F7,
		SDLK_F8,
		SDLK_F9,
		SDLK_g,
		SDLK_h,
		SDLK_HELP,
		SDLK_HOME,
		SDLK_i,
		SDLK_INSERT,
		SDLK_j,
		SDLK_k,
		SDLK_l,
		SDLK_LEFT,
		SDLK_LALT,
		SDLK_LCTRL,
		SDLK_LMETA,
		SDLK_LSHIFT,
		SDLK_LSUPER,
		SDLK_m,
		SDLK_MENU,
		SDLK_MODE,
		SDLK_n,
		SDLK_NUMLOCK,
		SDLK_KP0,
		SDLK_KP1,
		SDLK_KP2,
		SDLK_KP3,
		SDLK_KP4,
		SDLK_KP5,
		SDLK_KP6,
		SDLK_KP7,
		SDLK_KP8,
		SDLK_KP9,
		SDLK_KP_DIVIDE,
		SDLK_KP_ENTER,
		SDLK_KP_EQUALS,
		SDLK_KP_MINUS,
		SDLK_KP_MULTIPLY,
		SDLK_KP_PERIOD,
		SDLK_KP_PLUS,
		SDLK_o,
		SDLK_p,
		SDLK_PAGEDOWN,
		SDLK_PAGEUP,
		SDLK_PAUSE,
		SDLK_POWER,
		SDLK_PRINT,
		SDLK_SYSREQ,
		SDLK_q,
		SDLK_r,
		SDLK_RETURN,
		SDLK_RIGHT,
		SDLK_RALT,
		SDLK_RCTRL,
		SDLK_RMETA,
		SDLK_RSHIFT,
		SDLK_RSUPER,
		SDLK_s,
		SDLK_SCROLLOCK,
		SDLK_SPACE,
		SDLK_SYSREQ,
		SDLK_t,
		SDLK_TAB,
		SDLK_u,
		SDLK_UNDO,
		SDLK_UP,
		SDLK_v,
		SDLK_w,
		SDLK_x,
		SDLK_y,
		SDLK_z
	},
	modIDs[] =
	{
		XKEY_ALT,
		XKEY_CTRL,
		XKEY_META,
		XKEY_SHIFT,
		XKEY_SUPER
	};
	int
		i = 0;
	Keycode
		* r = NULL;
	if ((i = arg_match (inputModKeys, name)) >= 0)
	{
		r = xph_alloc (sizeof (Keycode));
		r->type = XPH_EXTRA;
		r->extra.key = modIDs[i];
		return r;
	}
	if ((i = arg_match (inputKeys, name)) >= 0)
	{
		r = xph_alloc (sizeof (Keycode));
		r->type = XPH_SDL;
		r->sdl.key = keyIDs[i];
		return r;
	}
	return NULL;
}

static bool key_match (const Keycode * key, SDLKey sym)
{
	if (key == NULL)
		return false;
	if (key->type == XPH_SDL)
		return key->sdl.key == sym;
	switch (key->extra.key)
	{
		case XKEY_ALT:
			return sym == SDLK_LALT || sym == SDLK_RALT;
		case XKEY_CTRL:
			return sym == SDLK_LCTRL || sym == SDLK_RCTRL;
		case XKEY_META:
			return sym == SDLK_LMETA || sym == SDLK_RMETA;
		case XKEY_SHIFT:
			return sym == SDLK_LSHIFT || sym == SDLK_RSHIFT;
		case XKEY_SUPER:
			return sym == SDLK_LSUPER || sym == SDLK_RSUPER;
		default:
			break;
	}
	return false;
}
