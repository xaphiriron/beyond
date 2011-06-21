#include "texture.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "bool.h"
#include "path.h"

#include "xph_log.h"
#include "xph_memory.h"

#include "dynarr.h"

static Dynarr
	TextureList = NULL;

static int textureCmp (const void * a, const void * b);
static int textureCmpPath (const void * p, const void * t);
static void texturesInit ();

static void texturesInit () {
  if (TextureList != NULL) {
    WARNING ("texturesInit called with a valid texture store; destroying textures, even if it might be a bad idea.", NULL);
    texturesDestroy ();
  }
  TextureList = dynarr_create (16, sizeof (struct texture *));
}

void texturesDestroy () {
//   struct texture * t = NULL;
  assert (TextureList != NULL);
	dynarr_wipe (TextureList, (void(*)(void *))textureUnload);
/* -- holdover from the phantomnation import; the above line ought to accomplish the same thing - xph 2011 06 14
  while (TextureList->offset > 0) {
    t = listIndex (TextureList, 0);
    t->references = 0;
    listRemove (TextureList, t);
    textureUnload (t);
  }
*/
  dynarr_destroy (TextureList);
  TextureList = NULL;
}

struct texture * textureLoad (const char * relPath, int mipmap, int trans, GLint wrap, GLint minfilter, GLint magfilter) {
  struct texture
    * match = NULL,
    * new = NULL;
  char * absPath = NULL;
	FUNCOPEN ();
  if (TextureList == NULL) {
    texturesInit ();
  }
  absPath = absolutePath (relPath);
	match = *(struct texture **)dynarr_search (TextureList, textureCmpPath, absPath);
	DEBUG ("searched loaded textures; match: %p", match);
  /*match = bsearch (absPath, TextureList->items, TextureList->offset, sizeof (struct texture *), (int (*)(const void *, const void *))textureCmpPath);*/
  if (match != NULL) {
    if (match->mipmap != mipmap) {
      WARNING ("loading a copy of texture \"%s\" (with %d reference%s) that requests an un-implemented mipmap change.", match->path, match->references, match->references == 1 ? "" : "s");
    }
    if (match->trans != trans) {
      WARNING ("loading a copy of texture \"%s\" (with %d reference%s) that requests an un-implemented transparency change.", match->path, match->references, match->references == 1 ? "" : "s");
    }
    glBindTexture (GL_TEXTURE_2D, match->id);
    if (match->wrap != wrap) {
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
      match->wrap = wrap;
    }
    if (match->minfilter != minfilter) {
      glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minfilter);
      match->minfilter = minfilter;
    }
    if (match->magfilter != magfilter) {
      glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magfilter);
      match->magfilter = magfilter;
    }
    glBindTexture (GL_TEXTURE_2D, 0);
    match->references++;
	FUNCCLOSE ();
    return match;
  }
  new = xph_alloc (sizeof (struct texture));
	DEBUG ("creating new texture @ %p", new);
  new->references = 1;
  new->path = xph_alloc (strlen (absPath) + 1);
	strcpy (new->path, absPath);
	DEBUG ("stored path as '%s'", new->path);
  new->info = xph_alloc (sizeof (pngInfo));
  if ((mipmap < 0 || mipmap > 3) && mipmap != PNG_NOMIPMAP && mipmap != PNG_BUILDMIPMAPS && mipmap != PNG_SIMPLEMIPMAPS) {
    DEBUG ("loading new texture \"%s\" with an invalid mipmap arg of %d.", new->path, mipmap);
    mipmap = PNG_NOMIPMAP;
  }
  new->mipmap = mipmap;
  if (wrap != GL_CLAMP && wrap != GL_REPEAT) {
    DEBUG ("loading new texture \"%s\" with an invalid wrap arg of %d.", new->path, wrap);
    wrap = GL_CLAMP;
  }
  new->wrap = wrap;
  new->trans = trans;
  new->minfilter = minfilter;
  new->magfilter = magfilter;
  DEBUG ("path: %s", new->path);
	new->id = 0;
	new->id = pngBind (new->path, new->mipmap, new->trans, new->info, new->wrap, new->minfilter, new->magfilter);
	if (new->id == 0)
	{
		ERROR ("tried to load a texture from \"%s\", which failed.", new->path);
		xph_free (new->info);
		xph_free (new->path);
		xph_free (new);
		FUNCCLOSE ();
		return NULL;
	}
	DEBUG ("bound png successfully", NULL);
	dynarr_push (TextureList, new);
	dynarr_sort (TextureList, textureCmp);
  /*qsort (TextureList->items, TextureList->offset, sizeof (struct texture *), (int (*)(const void *, const void *))textureCmp);*/
	DEBUG ("loaded texture at '%s' successfully", relPath);
	FUNCCLOSE ();
	return new;
}

void textureUnload (struct texture * t) {
/*
  int i = 0;
  bool shift = FALSE;*/
  assert (TextureList != NULL);
  if (0 >= --t->references) {
	dynarr_remove_condense (TextureList, t);
/* -- holdover from phantomnation import; the above line ought to accomplish the same thing - xph 2011 06 14
    while (i < TextureList->offset) {
      if (shift == TRUE || listIndex (TextureList, i) == t) {
        *(void **)((char *)TextureList->items + TextureList->size * i) =
          (i+1 >= TextureList->offset)
            ? NULL
            : *(void **)((char *)TextureList->items + TextureList->size * (i + 1));
        shift = TRUE;
      }
      i++;
    }
    TextureList->offset--;
*/
    glDeleteTextures (1, &t->id);
    xph_free (t->info);
    xph_free (t->path);
    xph_free (t);
  }
  return /*t == NULL ? 0 : t->references*/;
}

static int textureCmp (const void * a, const void * b) {
	DEBUG ("%s: cmp '%s' vs. '%s'", __FUNCTION__, (*(struct texture **)a)->path, (*(struct texture **)b)->path);
  return strcmp ((*(struct texture **)a)->path, (*(struct texture **)b)->path);
}

static int textureCmpPath (const void * p, const void * t) {
	DEBUG ("%s: cmp '%s' vs. '%s'", __FUNCTION__, p, (*(struct texture **)t)->path);
  return strcmp (p, (*(struct texture **)t)->path);
}
