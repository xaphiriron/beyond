#include "video.h"

ENTITY * VideoEntity = NULL;

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
  v->width = 720;
  v->orthographic = FALSE;
  v->doublebuffer = TRUE;
  v->resolution = 1.0;
  v->title = xph_alloc (16, "video->title");
  strncpy (v->title, "beyond 0.0.0.1", 16);
  v->icon = NULL;
  v->near = 10.0;
  v->far = 800.0;
  v->SDLmode = SDL_OPENGL;
  v->screen = NULL;
}


void video_enableSDLmodules () {
}

void video_enableGLmodules () {
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
    glHeight = video_getYResolution () / 2.0;
  glViewport (0, 0, v->width, v->height);
  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  if (v->orthographic == TRUE) {
    glOrtho (-glWidth, glWidth,
             -glHeight, glHeight,
             v->near, v->far);
  } else {
    glFrustum (-glWidth, glWidth,
               -glHeight, glHeight,
               v->near, v->far);
  }
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
  if (VideoEntity == NULL) {
    return -1;
  }
  v = entity_getClassData (VideoEntity, "video");
  return v->resolution * v->width;
}

float video_getYResolution () {
  const VIDEO * v = NULL;
  if (VideoEntity == NULL) {
    return -1;
  }
  v = entity_getClassData (VideoEntity, "video");
  return v->resolution * v->height;
}


int video_handler (ENTITY * e, eMessage msg, void * a, void * b) {
  VIDEO * v = NULL;
  switch (msg) {
    case EM_CLSNAME:
      strncpy (a, "video", 32);
      return EXIT_SUCCESS;
    case EM_CLSINIT:
    case EM_CLSFREE:
      return EXIT_FAILURE;
    case EM_CLSVARS:
      return EXIT_FAILURE;
    case EM_CREATE:
      if (VideoEntity != NULL) {
        entity_destroy (e);
        return EXIT_FAILURE;
      }
      v = video_create ();
      if (a != NULL) {
        video_loadConfigSettings (v, a);
      } else {
        video_loadDefaultSettings (v);
      }
      entity_addClassData (e, "video", v);
      VideoEntity = e;
      return EXIT_SUCCESS;
    default:
      break;
  }
  v = entity_getClassData (e, "video");
  switch (msg) {
    case EM_SHUTDOWN:
    case EM_DESTROY:
      VideoEntity = NULL;
      video_destroy (v);
      entity_rmClassData (e, "video");
      entity_destroy (e);
      return EXIT_SUCCESS;

    case EM_START:
      return video_initialize (v) == TRUE
        ? EXIT_SUCCESS
        : EXIT_FAILURE;

    case EM_PRERENDER:
      /* i really have no clue what video steps need to be taken each frame-- if i'm actually using the buffers to speed up rendering, then i don't actually want to clear the color and depth buffers each frame. but right now i'm not, so i am!
       */
      glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glMatrixMode (GL_MODELVIEW);
      return EXIT_SUCCESS;
    case EM_POSTRENDER:
      glFlush();
      if (v->doublebuffer) {
        SDL_GL_SwapBuffers();
      }
      return EXIT_SUCCESS;

    default:
      return entity_pass ();
  }
  return EXIT_FAILURE;
}