#include "texture.h"
#include "texture_internal.h"

#include <string.h>
#include <GL/glpng.h>

#include "xph_memory.h"
#include "xph_log.h"
#include "xph_path.h"
#include "dynarr.h"

static unsigned char
	TextureColor[4] = {0x00, 0x00, 0x00, 0x0ff},
	BackgroundColor[4] = {0xff, 0xff, 0xff, 0xff};

void textureSetBackgroundColor (unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	BackgroundColor[0] = r;
	BackgroundColor[1] = g;
	BackgroundColor[2] = b;
	BackgroundColor[3] = a;
}

void textureSetColor (unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	TextureColor[0] = r;
	TextureColor[1] = g;
	TextureColor[2] = b;
	TextureColor[3] = a;
}

unsigned char * textureColorAt (TEXTURE t, VECTOR3 px)
{
	static unsigned char
		oobBlank[4] = {0, 0, 0, 0};
	if (textureOOB (t, px))
		return oobBlank;
	return &t->data[(((t->height - 1) - (signed int)px.y) * t->width + (signed int)px.x) * t->mode];
}

void textureActiveStencil (TEXTURE t, unsigned char r, unsigned char g, unsigned char b)
{
	unsigned char
		check[3] = {r, g, b};
	int
		i = 0,
		max = t->height * t->width;
	//printf ("stenciling on 0x%2.2x%2.2x%2.2x\n", r, g, b);
	if (t->mode < 3)
	{
		ERROR ("Can't stencil texture (%p); only has %d color channel%s", t, t->mode, t->mode == 1 ? "" : "s");
		return;
	}
	while (i < max)
	{
		//printf ("at %d: got 0x%2.2x%2.2x%2.2x\n", i, t->data[i * t->mode], t->data[i * t->mode + 1], t->data[i * t->mode + 2]);
		if (memcmp (&t->data[i * t->mode], check, 3) == 0)
		{
			if (t->mode == 4)
				t->data[i * t->mode + 3] = 0;
			else
				memset (&t->data[i * t->mode], 0, t->mode);
			//printf ("  wrote 0x%2.2x%2.2x%2.2x\n", t->data[i * t->mode], t->data[i * t->mode + 1], t->data[i * t->mode + 2]);
		}
		i++;
	}
}

void textureFloodFill (TEXTURE t, signed int startX, signed int startY)
{
	unsigned char
		replaceColor[4] = {0, 0, 0, 0};
	struct txpx {
		signed int
			x,y;
	} * current,
	  * next;
	VECTOR3
		loc;
	Dynarr
		affectedPixels;

	if (TextureColor[3] == 0x00)
	{
		ERROR ("Can't flood fill with a transparent color.");
		return;
	}

	loc = vectorCreate (startX, startY, 0);
	
	if (textureOOB (t, loc))
	{
		INFO ("Can't centre flood at %d, %d; it's out of bounds of image (%dx%d)", startX, startY, t->width, t->height);
		return;
	}
	affectedPixels = dynarr_create (64, sizeof (struct txpx *));
	memcpy (replaceColor, textureColorAt (t, loc), t->mode);

	current = xph_alloc (sizeof (struct txpx));
	current->x = startX;
	current->y = startY;
	dynarr_queue (affectedPixels, current);

	while (!dynarr_isEmpty (affectedPixels))
	{
		current = *(struct txpx **)dynarr_dequeue (affectedPixels);
		//printf ("got: %d, %d\n", current->x, current->y);
		loc = vectorCreate (current->x, current->y, 0);
		if (textureOOB (t, loc))
		{
		}
		else if (memcmp (replaceColor, textureColorAt (t, loc), t->mode) == 0)
		{
			textureDrawPixel (t, loc);

			next = xph_alloc (sizeof (struct txpx));
			next->x = current->x + 1;
			next->y = current->y;
			dynarr_queue (affectedPixels, next);

			next = xph_alloc (sizeof (struct txpx));
			next->x = current->x;
			next->y = current->y + 1;
			dynarr_queue (affectedPixels, next);

			next = xph_alloc (sizeof (struct txpx));
			next->x = current->x - 1;
			next->y = current->y;
			dynarr_queue (affectedPixels, next);

			next = xph_alloc (sizeof (struct txpx));
			next->x = current->x;
			next->y = current->y - 1;
			dynarr_queue (affectedPixels, next);
		}
		xph_free (current);
	}

	INFO ("FINAL SIZE: %d ENTRIES", dynarr_capacity (affectedPixels));
	dynarr_destroy (affectedPixels);
}

/* about the "((t->height - px->y) * t->width + px->x" math:
 "The first element corresponds to the lower left corner of the texture image. Subsequent elements progress left-to-right through the remaining texels in the lowest row of the texture image, and then in successively higher rows of the texture image. The final element corresponds to the upper right corner of the texture image."
 */
void textureDrawPixel (TEXTURE t, VECTOR3 px)
{
	unsigned char
		* oldColor;
	unsigned char
		alpha,
		minusAlpha;
	/* printf ("drawing at %f/%d, %f/%d (offset: %d of %d) -- (%d - (signed int)%d) * %d + (signed int) %d\n", px.x, (signed int)px.x, px.y, (signed int)px.y, (((t->height - 1) - (signed int)px.y) * t->width + (signed int)px.x), t->height * t->width, t->height, (signed int)px.y, t->width, (signed int)px.x); */
	if (textureOOB (t, px))
		return;
	oldColor = textureColorAt (t, px);
	if (t->mode < 4 || (t->mode == 4 && TextureColor[3] == 0xff))
	{
		memcpy (oldColor, TextureColor, t->mode);
	}
	else
	{
		alpha = TextureColor[3];
		minusAlpha = (0xff - TextureColor[3]);
		//printf ("pre:  #%2.2x%2.2x%2.2x%2.2x\n", oldColor[0], oldColor[1], oldColor[2], oldColor[3]);

		oldColor[0] = (oldColor[0] * minusAlpha + TextureColor[0] * alpha) / 0xff;
		oldColor[1] = (oldColor[1] * minusAlpha + TextureColor[1] * alpha) / 0xff;
		oldColor[2] = (oldColor[2] * minusAlpha + TextureColor[2] * alpha) / 0xff;
		// add the % of the remaining alpha (e.g., if backround alpha is 191 and foreground alpha is 127 then 31 alpha is added since that's 50% of (255 - 191)
		oldColor[3] += (0xff - oldColor[3]) * TextureColor[3] / 0xff;

		//printf ("post: #%2.2x%2.2x%2.2x%2.2x\n", oldColor[0], oldColor[1], oldColor[2], oldColor[3]);
	}
}

void textureDrawLine (TEXTURE t, VECTOR3 start, VECTOR3 end)
{
	VECTOR3
		diff = vectorSubtract (&end, &start),
		norm = vectorNormalize (&diff),
		current = start;
	/* i'm using i/max as a counter since due to floating point rounding error
	 * it's not feasible to use vector_cmp (&current, &end), which is the only
	 * other way i can think of to check for when the line is actually done
	 *  - xph 2011 08 27
	 */
	int
		i = 0,
		max = vectorMagnitude (&diff) + 1;
	/* if start is out of bounds skip ahead until it isn't (if it isn't), and
	 * if it goes out of bounds again then it's not coming back. this is
	 * important if start is out of bounds but end isn't (or if they're both
	 * oob but they cross the image edge at some point); the start of the line
	 * won't be drawn but stopping after the first oob coordiante would be
	 * wrong
	 * (there's a better way to do this: if either value is oob, calculate the
	 * intersection of the line w/ the screen edges and jump to that point
	 * without needing to loop)
	 *  - xph 2011 08 27
	 */
	while (textureOOB (t, current) && i < max)
	{
		current = vectorAdd (&current, &norm);
		i++;
	}
	while (i < max)
	{
		if (textureOOB (t, current))
			break;
		textureDrawPixel (t, current);
		current = vectorAdd (&current, &norm);
		i++;
	}
}

void textureDrawHex (TEXTURE t, VECTOR3 centre, unsigned int size, float angle)
{
	VECTOR3
		axis = vectorCreate (
			cos (angle) * size,
			sin (angle) * size,
			0
		),
	// rotation A is by 60 degrees; rotation B is by 120 degrees
		rotA = vectorCreate (
			cos (angle + M_PI * .333) * size,
			sin (angle + M_PI * .333) * size,
			0
		),
		rotB = vectorCreate (
			cos (angle + M_PI * .666) * size,
			sin (angle + M_PI * .666) * size,
			0
		),
		corners[6];
	int
		i = 0;

	corners[0] = vectorCreate (centre.x + axis.x, centre.y + axis.y, 0);
	corners[1] = vectorCreate (centre.x + rotA.x, centre.y + rotA.y, 0);
	corners[2] = vectorCreate (centre.x + rotB.x, centre.y + rotB.y, 0);
	corners[3] = vectorCreate (centre.x - axis.x, centre.y - axis.y, 0);
	corners[4] = vectorCreate (centre.x - rotA.x, centre.y - rotA.y, 0);
	corners[5] = vectorCreate (centre.x - rotB.x, centre.y - rotB.y, 0);

	while (i < 6)
	{
		textureDrawLine (t, corners[i], corners[(i + 1) % 6]);
		i++;
	}
	//printf ("FLOODING\n");
	textureFloodFill (t, (signed int)centre.x, (signed int)centre.y);
}

void textureCopyChunkFromRaw (TEXTURE t, unsigned int x, unsigned int y, unsigned int width, unsigned int height, const TEXTURE raw, unsigned int rawX, unsigned int rawY)
{
	int
		lines = 0;
	unsigned char
		* to = NULL,
		* from = NULL;
	VECTOR3
		px;
	if (t->mode != raw->mode)
	{
		WARNING ("Can't copy raw data between two textures with different modes (to has %d channels; from has %d)", t->mode, raw->mode);
		return;
	}

	while (lines < height)
	{
		px.x = x;
		px.y = y + lines;
		to = textureColorAt (t, px);
		px.x = rawX;
		px.y = rawY + lines;
		from = textureColorAt (raw, px);

		memcpy (to, from, width * t->mode);
		lines++;
	}

}


bool textureOOB (const TEXTURE t, VECTOR3 coord)
{
	if ((signed int)coord.x < 0 || (signed int)coord.x >= t->width ||
		(signed int)coord.y < 0 || (signed int)coord.y >= t->height)
		return true;
	return false;
}

GLuint textureName (const TEXTURE t)
{
	if (!t)
		return 0;
	return t->name;
}



TEXTURE textureGenBlank (unsigned int width, unsigned int height, unsigned char mode)
{
	TEXTURE
		t = xph_alloc (sizeof (struct xph_texture));
	int
		i = 0,
		max = width * height;
	t->width = width;
	t->height = height;
	if (mode < 1 || mode > 4)
	{
		WARNING ("Invalid mode of %d for texture; using 4 instead.", mode);
		t->mode = 4;
	}
	else
	{
		t->mode = mode;
	}
	t->name = 0;
	t->data = xph_alloc (t->width * t->height * t->mode);
	while (i < max)
	{
		memcpy (&t->data[i * t->mode], BackgroundColor, t->mode);
		i++;
	}

	return t;
}

TEXTURE textureGenFromImage (const char * path)
{
	TEXTURE
		t = xph_alloc (sizeof (struct xph_texture));
	pngRawInfo
		raw;

	pngLoadRaw (absolutePath (path), &raw);

	/* FIXME: this is where we would be doing the verification to make sure
	 * this image is actually the kind of image we can use as a texture (e.g.,
	 * has power-of-two dimensions, isn't paletted [since i don't know how to
	 * handle that yet]) but uhhh it's not done yet
	 *  - xph 2011 08 26
	 */
	t->width = raw.Width;
	t->height = raw.Height;
	t->mode = raw.Components;
	t->name = 0;
	t->data = raw.Data;
	if (raw.Palette)
	{
		free (raw.Palette);
		raw.Palette = NULL;
	}

	return t;
}

void textureDestroy (TEXTURE t)
{
	if (!t)
		return;
	if (t->name)
	{
		glDeleteTextures (1, &t->name);
		t->name = 0;
	}
	if (t->data)
	{
		free (t->data);
		t->data = NULL;
	}
	xph_free (t);
}

GLuint textureBind (TEXTURE t)
{
	int
		format;
	switch (t->mode)
	{
		case 1:
			format = GL_ALPHA;
			break;
		case 2:
			format = GL_LUMINANCE_ALPHA;
			break;
		case 3:
			format = GL_RGB;
			break;
		case 4:
			format = GL_RGBA;
			break;
		default:
			ERROR ("Texture has invalid mode of %d; can't deal; probably won't be able to load data", t->mode);
			format = GL_ALPHA;
			break;
	}
	glGenTextures (1, &t->name);
	glBindTexture (GL_TEXTURE_2D, t->name);
	/* i suspect i'll discover this very quickly when i start actually using
	 * this code to /load/ images, but bad things will happen if the width and
	 * height values aren't powers of two, so if we load images that aren't
	 * powers of two then they've got to be padded in a very specific way
	 *  - xph 2011 08 26
	 */
	glTexImage2D (GL_TEXTURE_2D, 0, t->mode, t->width, t->height, 0, format, GL_UNSIGNED_BYTE, t->data);
	/* to generate mipmaps: increase the second arg to get the n-th mipmap for
	 * the texture. i think if any mipmaps are specified you need to specify
	 * them all, down to 1x1... but that's probably wrong.
	 *  - xph 2011 08 26
	 */
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glBindTexture (GL_TEXTURE_2D, 0);
	return t->name;
}