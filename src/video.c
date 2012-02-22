/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#include "video.h"

#include "xph_memory.h"
#include "xph_log.h"
#include "system.h"
#include "ogdl/ogdl.h"

#define VIDEO_DEFAULT_RESOLUTION	0.05

struct video
{
	float
		zNear,
		zFar,
		resolution;
	char
		* title,
		* icon;
	unsigned int
		height,
		width;
	bool
		orthographic,
		doublebuffer;

	unsigned int
		SDLmode;
	SDL_Surface
		* screen;

	bool
		renderWireframe;

};

typedef struct video VIDEO;

static VIDEO
	* Video = NULL;


static VIDEO * video_create ();
static void video_destroy (VIDEO * v);

static void video_loadDefaultSettings (VIDEO * v);
static void video_loadDefaults ();
static void video_loadConfig (const Graph config);

static void video_enableSDLmodules ();
static void video_enableGLmodules ();

static bool video_initialize (VIDEO *);
static void video_regenerateDisplay (VIDEO *);


void videoInit ()
{
	if (Video)
		return;
	Video = video_create ();
	if (System->config)
		video_loadConfig (System->config);
	else
		video_loadDefaults ();
	video_loadDefaultSettings (Video);
	if (!video_initialize (Video))
	{
		ERROR ("Failure while loading video; quitting");
		System->quit = true;
	}
};

void videoDestroy ()
{
	video_destroy (Video);
	Video = NULL;

	SDL_Quit ();
};

void videoPrerender ()
{
	/* i really have no clue what video steps need to be taken each frame-- if i'm actually using the buffers to speed up rendering, then i don't actually want to clear the color and depth buffers each frame. but right now i'm not, so i am!
	 */
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();
}

void videoPostrender ()
{
	glFlush();
	if (Video->doublebuffer)
		SDL_GL_SwapBuffers();
}

static VIDEO * video_create ()
{
	VIDEO
		* v = xph_alloc (sizeof (VIDEO));
	return v;
}

static void video_destroy (VIDEO * v)
{
	if (v->title != NULL)
		xph_free (v->title);
	if (v->icon != NULL)
		xph_free (v->icon);
	xph_free (v);
}

static void video_loadDefaults ()
{
	Graph
		fakeConfig = Graph_new ("__root__"),
		video,
		size;

	Graph_set (fakeConfig, ".video", NULL);
	video = Graph_get (fakeConfig, "video");

	Graph_set (video, ".fullscreen", Graph_new ("off"));
	Graph_set (video, ".size", NULL);

	size = Graph_get (video, "size");
	Graph_set (size, ".960", NULL);
	Graph_set (size, ".540", NULL);

	video_loadConfig (fakeConfig);
	Graph_free (fakeConfig);
}

static void video_loadConfig (const Graph config)
{
	Graph
		fullscreenNode,
		widthNode,
		heightNode;
	signed int
		width,
		height;
	Graph_print (config);

	Video->SDLmode = SDL_OPENGL;
	fullscreenNode = Graph_get (config, ".video.fullscreen.[0]");
	if (fullscreenNode)
	{
		if (strcmp (fullscreenNode->name, "on") == 0)
			Video->SDLmode = SDL_OPENGL | SDL_FULLSCREEN;
		else if (strcmp (fullscreenNode->name, "off") == 0)
			Video->SDLmode = SDL_OPENGL;
		else
			ERROR ("For fullscreen: expected on|off");
	}

	widthNode = Graph_get (config, ".video.size.[0]");
	heightNode = Graph_get (config, ".video.size.[1]");
	if (widthNode && heightNode)
	{
		width = atoi (widthNode->name);
		height = atoi (heightNode->name);
		if (width <= 0 || height <= 0)
		{
			ERROR ("Video resolution of %d x %d not possible", width, height);
		}
		else
		{
			Video->width = width;
			Video->height = height;
		}
	}
	else
	{
		WARNING ("Video display resolution not specified; defaulting to 960 x 540");
		Video->width = 960;
		Video->height = 540;
	}
	
	

}

static void video_loadDefaultSettings (VIDEO * v)
{
	char
		title[] = "beyond 0.1";
	v->orthographic = false;
	v->doublebuffer = true;
	v->resolution = VIDEO_DEFAULT_RESOLUTION;
	v->title = xph_alloc (strlen (title) + 1);
	strncpy (v->title, title, strlen (title) + 1);
	v->icon = NULL;
	v->zNear = 20.0;
	v->zFar = 10000.0;
	v->screen = NULL;
	v->renderWireframe = false;
}


static void video_enableSDLmodules ()
{
	SDL_version
		sdlversion;
	SDL_VERSION (&sdlversion);
	INFO ("Compiled with SDL version %d.%d.%d\n", sdlversion.major, sdlversion.minor, sdlversion.patch);

	SDL_EnableUNICODE (true);
	//SDL_ShowCursor (SDL_DISABLE);
	//SDL_WM_GrabInput (SDL_GRAB_ON);
}

static void video_enableGLmodules ()
{
	const unsigned char
		* glVersion,
		* glExtensions;
	glVersion = glGetString (GL_VERSION);
	glExtensions = glGetString (GL_EXTENSIONS);
	INFO ("Using GL version %s with extensions:", glVersion);
	INFO ("\t%s", glExtensions);

	glClearColor (0.1215, 0.0588, 0.1843, 0.0); // dark purple
	glClearDepth (1.0);

	glEnable (GL_DEPTH_TEST);
	glEnable (GL_CULL_FACE);
	glPolygonMode (GL_FRONT, GL_FILL);

	glEnable (GL_TEXTURE_2D);
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glShadeModel (GL_FLAT);
	//glEnable (GL_POLYGON_SMOOTH);
}


static bool video_initialize (VIDEO * v)
{
	SDL_Surface
		* icon = NULL;
	const SDL_VideoInfo
		* info = NULL;
	const char
		* sdlError;
	int
		glError;
	if (!v->screen && SDL_Init (SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) < 0)
	{
		if ((sdlError = SDL_GetError ())[0] != 0)
			ERROR ("SDL failed to initialize. The error given was \"%s\".", sdlError);
		else
			ERROR ("SDL failed to initialize. There was no error given.");
		return false;
	}
	else if ((sdlError = SDL_GetError ())[0] != 0)
	{
		WARNING ("SDL generated a non-fatal error while initializing: %s", sdlError);
	}
	SDL_ClearError ();
	if (v->icon)
	{
		icon = SDL_LoadBMP (v->icon);
		SDL_WM_SetIcon (icon, NULL);
		free (icon);
	}
	if (v->title)
	{
		SDL_WM_SetCaption (v->title, NULL);
	}

	info = SDL_GetVideoInfo ();
	sdlError = SDL_GetError ();
	if (!info)
	{
		if (sdlError[0] != 0)
			ERROR ("Can't get SDL video information. The error given was \"%s\".", sdlError);
		else
			ERROR ("Can't get SDL video information. There was no error given.");
		SDL_ClearError ();
	}
	else
	{
		v->SDLmode |= (info->hw_available ? SDL_HWSURFACE : 0);
	}
	video_enableSDLmodules ();

  /* TODO: the perennial refrain here is that colour depth should be tested
   * and stored in the config, instead of dictated by fiat here in the source.
   */
	SDL_GL_SetAttribute (SDL_GL_RED_SIZE, 5);
	SDL_GL_SetAttribute (SDL_GL_GREEN_SIZE, 5);
	SDL_GL_SetAttribute (SDL_GL_BLUE_SIZE, 5);
	SDL_GL_SetAttribute (SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, v->doublebuffer == true ? 1 : 0);
	if (v->screen == NULL)
	{
		v->screen = SDL_SetVideoMode (v->width, v->height, info == NULL ? 0 : info->vfmt->BitsPerPixel, v->SDLmode);
		sdlError = SDL_GetError ();
		
		if (v->screen == NULL)
		{
			if (sdlError[0] != 0)
				ERROR ("SDL failed to start video. The error given was \"%s\".", sdlError);
			else
				ERROR ("SDL failed to start video. There was no error given.");
			return false;
		}
		else if (!(v->screen->flags & SDL_OPENGL))
		{
			if (sdlError[0] != 0)
				ERROR ("SDL was unable to create an OpenGL context. The error given was \"%s\"", sdlError);
			else
				ERROR ("SDL was unable to create an OpenGL context. There was no error given.");
			return false;
		}
		else if (sdlError[0] != 0)
		{
			WARNING ("SDL generated a non-fatal error while setting the video mode: %s", sdlError);
		}
		SDL_ClearError ();

		while ((glError = glGetError ()) != GL_NO_ERROR)
		{
			ERROR ("GL generated the error code on creation: %d\n", glError);
			// if gl has no context it'll generate GL_INVALID_OPERATION in an infinite loop. this should never trigger since we catch the no-gl-context case above, but, you know, just in case - xph 02 22 2012
			if (glError == GL_INVALID_OPERATION)
				break;
		}
	}
	video_enableGLmodules ();
	video_regenerateDisplay (v);
	return true;
}

static void video_regenerateDisplay (VIDEO * v)
{
	float
		glWidth = video_getXResolution () / 2.0,
		glHeight = video_getYResolution () / 2.0;
	glViewport (0, 0, v->width, v->height);
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	if (v->orthographic == true)
		glOrtho (-glWidth, glWidth, -glHeight, glHeight, v->zNear, v->zFar);
	else
		glFrustum (-glWidth, glWidth, -glHeight, glHeight, v->zNear, v->zFar);
	//printf ("%s: opengl frustum is %5.3f by %5.3f with a resolution of %5.3f and a near/far value of %5.3f/%5.3f\n", __FUNCTION__, glWidth, glHeight, v->resolution, v->zNear, v->zFar);
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();
}

float video_getZnear ()
{
	if (!Video)
		return 0.0;
	return -Video->zNear;
}

float video_getXResolution ()
{
	if (!Video)
		return -1;
	return Video->resolution * Video->width;
}

float video_getYResolution ()
{
	if (!Video)
		return -1;
	return Video->resolution * Video->height;
}

bool video_setScaling (float scale)
{
	if (!Video)
		return false;
	if (scale <= 0)
		return false;
	Video->resolution = scale;
	video_regenerateDisplay (Video);
	return true;
}

bool video_setDimensions (signed int width, signed int height)
{
	if (width <= 0 || height <= 0)
		return false;
	if (!Video)
		return false;
	Video->width = width;
	Video->height = height;
	video_regenerateDisplay (Video);
	return true;
}

bool video_getDimensions (unsigned int * width, unsigned int * height)
{
	if (Video == NULL || width == NULL || height == NULL)
		return false;
	*width = Video->width;
	*height = Video->height;
	return true;
}

inline float video_pixelYMap (int y)
{
	if (!Video)
		return 0.0;
	return (Video->height / 2.0 - y) * Video->resolution;
}

inline float video_pixelXMap (int x)
{
	if (!Video)
		return 0.0;
	return (x - Video->width / 2.0) * Video->resolution;
}

inline float video_pixelXOffset (signed int x)
{
	if (!Video)
		return 0.0;
	return x * Video->resolution;
}

inline float video_pixelYOffset (signed int y)
{
	if (!Video)
		return 0.0;
	return y * -Video->resolution;
}

void video_orthoOff ()
{
	if (!Video)
		return;
	Video->orthographic = 0;
	video_setScaling (VIDEO_DEFAULT_RESOLUTION);
	video_regenerateDisplay (Video);
}

void video_orthoOn ()
{
	if (!Video)
		return;
	Video->orthographic = 1;
	video_setScaling (1.00);
	video_regenerateDisplay (Video);
}

void video_wireframeSwitch ()
{
	if (!Video)
		return;
	Video->renderWireframe ^= 1;
	if (Video->renderWireframe)
		glPolygonMode (GL_FRONT, GL_LINE);
	else
		glPolygonMode (GL_FRONT, GL_FILL);
}