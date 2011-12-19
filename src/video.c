#include "video.h"

#include "xph_memory.h"

#define VIDEO_DEFAULT_RESOLUTION	0.05

static VIDEO
	* Video = NULL;


void videoInit ()
{
	if (Video)
		return;
	Video = video_create ();
	video_loadDefaultSettings (Video);
	video_initialize (Video);
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

VIDEO * video_create () {
  VIDEO * v = xph_alloc (sizeof (VIDEO));
  return v;
}

void video_destroy (VIDEO * v) {
  if (v->title != NULL) {
    xph_free (v->title);
  }
  if (v->icon != NULL) {
    xph_free (v->icon);
  }
  xph_free (v);
}


bool video_loadConfigSettings (VIDEO * v, char * configPath) {
  video_loadDefaultSettings (v);
  return false;
}

void video_loadDefaultSettings (VIDEO * v) {
  v->height = 540;
  v->width = 960;
  v->orthographic = false;
  v->doublebuffer = true;
  v->resolution = VIDEO_DEFAULT_RESOLUTION;
  v->title = xph_alloc_name (16, "video->title");
  strncpy (v->title, "beyond 0.0.0.1", 16);
  v->icon = NULL;
  v->near = 20.0;
  v->far = 5000.0;
  v->SDLmode = SDL_OPENGL;
  v->screen = NULL;
	v->renderWireframe = false;
}


void video_enableSDLmodules () {
  //SDL_ShowCursor (SDL_DISABLE);
  //SDL_WM_GrabInput (SDL_GRAB_ON);
}

void video_enableGLmodules ()
{
	//glClearColor (.22, .18, .22, 0.0);
	//glClearColor (0.20, 0.03, 0.12, 0.0); // dark purple
	//glClearColor (0.29, 0.10, 0.19, 0.0); // slightly lighter purple
	glClearColor (0.84, 0.98, 0.68, 0.0); // bright green
	glClearDepth (1.0);

	glEnable (GL_DEPTH_TEST);
	glEnable (GL_CULL_FACE);
	glPolygonMode (GL_FRONT, GL_FILL);
	//glPolygonMode (GL_BACK, GL_LINE);

	glEnable (GL_TEXTURE_2D);
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glShadeModel (GL_FLAT);
	//glEnable (GL_POLYGON_SMOOTH);
}


bool video_initialize (VIDEO * v) {
  SDL_Surface * icon = NULL;
  const SDL_VideoInfo * info = NULL;
  if (v->screen == NULL && SDL_Init (SDL_INIT_VIDEO) < 0) {
    fprintf (stderr, "SDL failed to initialize. The error given was \"%s\".\n", SDL_GetError ());
    return false;
  }
  if (v->icon != NULL) {
    icon = SDL_LoadBMP (v->icon);
    SDL_WM_SetIcon (icon, NULL);
    free (icon);
  }
  if (v->title != NULL) {
    SDL_WM_SetCaption (v->title, NULL);
  }

  info = SDL_GetVideoInfo ();
  if (info == NULL) {
    fprintf (stderr, "Can't get video info: %s; bad things are probably going to happen soon.\n", SDL_GetError());
  } else {
    v->SDLmode |= (info->hw_available ? SDL_HWSURFACE : 0);
/* evidently the documentation lied to me and this does not in fact exist
    if (v->width > info->current_w || v->height > info->current_h) {
      printf ("The resolution we're about to try (%d,%d) is larger than your current resolution (%d,%d)... this might end badly.\n", v->width, v->height, info->current_w, info->current_h);
    }
*/
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
  if (
    v->screen == NULL
      && (v->screen = SDL_SetVideoMode (
            v->width,
            v->height,
            info == NULL ? 16 : info->vfmt->BitsPerPixel,
            v->SDLmode)
         ) == NULL) {
    fprintf (stderr, "SDL failed to start video. The error given was \"%s\".\n", SDL_GetError ());
    return false;
  }
  video_enableGLmodules ();
  video_regenerateDisplay (v);
  return true;
}

void video_regenerateDisplay (VIDEO * v) {
  float
    glWidth = video_getXResolution () / 2.0,
    glHeight = video_getYResolution () / 2.0,
    glNear = v->near,
    glFar = v->far;
  glViewport (0, 0, v->width, v->height);
  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  if (v->orthographic == true) {
    glOrtho (-glWidth, glWidth,
             -glHeight, glHeight,
             glNear, glFar);
  } else {
    glFrustum (-glWidth, glWidth,
               -glHeight, glHeight,
               glNear, glFar);
  }
  //printf ("%s: opengl frustum is %5.3f by %5.3f with a resolution of %5.3f and a near/far value of %5.3f/%5.3f\n", __FUNCTION__, glWidth, glHeight, v->resolution, glNear, glFar);
  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity ();
}

bool video_setResolution (VIDEO * v, float x, float y) {
  if (x <= 0 || y <= 0) {
    return false;
  }
  v->width = x;
  v->height = y;
  video_regenerateDisplay (v);
  return true;
}

bool video_setScaling (VIDEO * v, float scale) {
  if (scale <= 0) {
    return false;
  }
  v->resolution = scale;
  video_regenerateDisplay (v);
  return true;
}

float video_getZnear ()
{
	if (!Video)
		return 0.0;
	return -Video->near;
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
	video_setScaling (Video, VIDEO_DEFAULT_RESOLUTION);
	video_regenerateDisplay (Video);
}

void video_orthoOn ()
{
	if (!Video)
		return;
	Video->orthographic = 1;
	video_setScaling (Video, 1.00);
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