#include "video.h"

Object * VideoObject = NULL;

VIDEO * video_create () {
  VIDEO * v = xph_alloc (sizeof (VIDEO), "VIDEO");
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
  return FALSE;
}

void video_loadDefaultSettings (VIDEO * v) {
  v->height = 540;
  v->width = 960;
  v->orthographic = FALSE;
  v->doublebuffer = TRUE;
  v->resolution = 0.03;
  v->title = xph_alloc (16, "video->title");
  strncpy (v->title, "beyond 0.0.0.1", 16);
  v->icon = NULL;
  v->near = 10.0;
  v->far = 5000.0;
  v->SDLmode = SDL_OPENGL;
  v->screen = NULL;
}


void video_enableSDLmodules () {
}

void video_enableGLmodules () {
  glClearColor (0.0, 0.0, 0.0, 0.0);
  glClearDepth (1.0);
  glEnable (GL_DEPTH_TEST);
  glEnable (GL_CULL_FACE);
  glPolygonMode (GL_FRONT, GL_FILL);
  //glPolygonMode (GL_BACK, GL_LINE);
}


bool video_initialize (VIDEO * v) {
  SDL_Surface * icon = NULL;
  const SDL_VideoInfo * info = NULL;
  if (v->screen == NULL && SDL_Init (SDL_INIT_VIDEO) < 0) {
    fprintf (stderr, "SDL failed to initialize. The error given was \"%s\".\n", SDL_GetError ());
    return FALSE;
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
  SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, v->doublebuffer == TRUE ? 1 : 0);
  if (
    v->screen == NULL
      && (v->screen = SDL_SetVideoMode (
            v->width,
            v->height,
            info == NULL ? 16 : info->vfmt->BitsPerPixel,
            v->SDLmode)
         ) == NULL) {
    fprintf (stderr, "SDL failed to start video. The error given was \"%s\".\n", SDL_GetError ());
    return FALSE;
  }
  video_enableGLmodules ();
  video_regenerateDisplay (v);
  return TRUE;
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
  if (v->orthographic == TRUE) {
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
    return FALSE;
  }
  v->width = x;
  v->height = y;
  video_regenerateDisplay (v);
  return TRUE;
}

bool video_setScaling (VIDEO * v, float scale) {
  if (scale <= 0) {
    return FALSE;
  }
  v->resolution = scale;
  video_regenerateDisplay (v);
  return TRUE;
}

float video_getXResolution () {
  const VIDEO * v = NULL;
  if (VideoObject == NULL) {
    return -1;
  }
  v = obj_getClassData (VideoObject, "video");
  return v->resolution * v->width;
}

float video_getYResolution () {
  const VIDEO * v = NULL;
  if (VideoObject == NULL) {
    return -1;
  }
  v = obj_getClassData (VideoObject, "video");
  return v->resolution * v->height;
}


int video_handler (Object * o, objMsg msg, void * a, void * b) {
  VIDEO * v = NULL;
  switch (msg) {
    case OM_CLSNAME:
      strncpy (a, "video", 32);
      return EXIT_SUCCESS;
    case OM_CLSINIT:
    case OM_CLSFREE:
      return EXIT_FAILURE;
    case OM_CLSVARS:
      return EXIT_FAILURE;
    case OM_CREATE:
      if (VideoObject != NULL) {
        obj_destroy (o);
        return EXIT_FAILURE;
      }
      v = video_create ();
      if (a != NULL) {
        video_loadConfigSettings (v, a);
      } else {
        video_loadDefaultSettings (v);
      }
      obj_addClassData (o, "video", v);
      VideoObject = o;
      return EXIT_SUCCESS;
    default:
      break;
  }
  v = obj_getClassData (o, "video");
  switch (msg) {
    case OM_SHUTDOWN:
    case OM_DESTROY:
      VideoObject = NULL;
      video_destroy (v);
      obj_rmClassData (o, "video");
      obj_destroy (o);
      return EXIT_SUCCESS;

    case OM_START:
      return video_initialize (v) == TRUE
        ? EXIT_SUCCESS
        : EXIT_FAILURE;

    case OM_PRERENDER:
      /* i really have no clue what video steps need to be taken each frame-- if i'm actually using the buffers to speed up rendering, then i don't actually want to clear the color and depth buffers each frame. but right now i'm not, so i am!
       */
      glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glMatrixMode (GL_MODELVIEW);
      return EXIT_SUCCESS;
    case OM_POSTRENDER:
      glFlush();
      if (v->doublebuffer) {
        SDL_GL_SwapBuffers();
      }
      return EXIT_SUCCESS;

    default:
      return obj_pass ();
  }
  return EXIT_FAILURE;
}