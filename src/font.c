#include "font.h"

struct font_textcache
{
	float
		* vertices,
		* textures;
	int
		glyphs;
};

struct font_char
{
	int
		width,
		height,
		xadvance,
		topBearing;
};

static int next_power (int n)
{
	int
		r = 1;
	while (r < n)
		r <<= 1;
	return r;
}


#define GLYPH_COUNT 256

struct glyph_sheet
{
	GLuint
		name;
	unsigned char
		* data;
	struct font_char
		glyphs[GLYPH_COUNT];
	unsigned int
		height,
		width,
		glyphHeight,
		glyphWidth;
};

static FT_Library
	Library = NULL;
static FT_Face
	Face = NULL;

static struct glyph_sheet
	* GlyphSheet = NULL;

static int
	FontSizeInPx = 0;

static enum textAlignType
	TextAlign = ALIGN_LEFT;

static void fontTexelCoords (unsigned int point, float * x, float * y, float * w, float * h);

void fontLoad (const char * path, int fontSize)
{
	int
		error = 0,
		i, max,
		x, y,
		maxWidth = 0,
		maxHeight = 0,
		spriteX,
		spriteY;
	FT_Bitmap
		* bitmap = NULL;
	unsigned int
		c = 0,
		textureIndex = 0;
	char
		* absPath;
	if (!Library)
	{
		error = FT_Init_FreeType (&Library);
		if (error)
		{
			ERROR ("Some kind of error during freetype initialization");
			return;
		}
	}
	// the 0 is the face index, e.g., normal font instead of bold or italic (and indexes greater than 0 might not exist depending on the font)
	absPath = absolutePath (path);
	error = FT_New_Face (Library, absPath, 0, &Face);
	if (error)
	{
		ERROR ("Couldn't load font at \"%s\"", absPath);
		return;
	}
	// the 0 is the width; it defaults to the same as the height (which is the fontSize << 6); 96,96 is the horiz,vert dpi (which is a guess)
	FT_Set_Char_Size (Face, 0, fontSize << 6, 96, 96);
	FontSizeInPx = (fontSize / 72.) * 96;

	bitmap = &Face->glyph->bitmap;
	while (c < GLYPH_COUNT)
	{
		FT_Load_Char (Face, c, FT_LOAD_RENDER);
		if (bitmap->width > maxWidth)
			maxWidth = bitmap->width;
		if (bitmap->rows > maxHeight)
			maxHeight = bitmap->rows;
		c++;
	}

	// okay despite the usage of GLYPH_COUNT this requires it to be 256 since we're generating a 16x16 grid. this is kind of dumb. - xph 2012 01 11
	GlyphSheet = xph_alloc (sizeof (struct glyph_sheet));
	GlyphSheet->glyphWidth = maxWidth;
	GlyphSheet->glyphHeight = maxHeight;
	GlyphSheet->width = next_power (GlyphSheet->glyphWidth * 16);
	GlyphSheet->height = next_power (GlyphSheet->glyphHeight * 16);
	GlyphSheet->data = xph_alloc (GlyphSheet->width * GlyphSheet->height * 2);
	memset (GlyphSheet->data, 0x7f, GlyphSheet->width * GlyphSheet->height * 2);

	c = 0;
	while (c < GLYPH_COUNT)
	{
		FT_Load_Char (Face, c, FT_LOAD_RENDER);
		GlyphSheet->glyphs[c].width = bitmap->width;
		GlyphSheet->glyphs[c].height = bitmap->rows;
		GlyphSheet->glyphs[c].topBearing = Face->glyph->metrics.horiBearingY >> 6;
		GlyphSheet->glyphs[c].xadvance = Face->glyph->advance.x >> 6;

		// we're using a 2-component GL_LUMINANCE_ALPHA texture (and setting the luminance to 0xff always) because even though you might assume you could just use a GL_ALPHA texture evidently you can't. or, more likely, setting that to work correctly is slightly more complex than just changing the way the data is set and parsed (i guess??) - xph 2012 01 08

		assert (bitmap->pixel_mode == FT_PIXEL_MODE_GRAY);
		i = 0;
		max = bitmap->rows * bitmap->width;
		while (i < max)
		{
			// see previous note about how max(Width|Height) and total(Width|Height) are used - xph 2012 01 11
			spriteX = GlyphSheet->glyphWidth * (c % 16);
			spriteY = GlyphSheet->glyphHeight * (c / 16);
			x = spriteX + (i % bitmap->width);
			y = spriteY + (i / bitmap->width);
			textureIndex = (((GlyphSheet->height - y - 1) * GlyphSheet->width) + x) * 2;
			assert (textureIndex < GlyphSheet->height * GlyphSheet->width * 2);
			memset (&GlyphSheet->data [textureIndex], 0xff, 1);
			memset (&GlyphSheet->data [textureIndex + 1], bitmap->buffer[i], 1);
			i++;
		}

		c++;
	}

	glGenTextures (1, &GlyphSheet->name);
	assert (GlyphSheet->name != 0);
	glBindTexture (GL_TEXTURE_2D, GlyphSheet->name);
	glTexImage2D (GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, GlyphSheet->width, GlyphSheet->height, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, GlyphSheet->data);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

void fontUnload ()
{
	if (!Face)
		return;
	glDeleteTextures (1, &GlyphSheet->name);
	xph_free (GlyphSheet->data);
	xph_free (GlyphSheet);
	FontSizeInPx = 0;
	FT_Done_Face (Face);
	Face = NULL;
}

int fontLineHeight ()
{
	return FontSizeInPx;
}

enum textAlignType fontPrintAlign (enum textAlignType align)
{
	switch (align)
	{
		case ALIGN_LEFT:
		case ALIGN_RIGHT:
/*
		case ALIGN_CENTRE:
		case ALIGN_JUSTIFY:
*/
			TextAlign = align;
			break;
		default:
			WARNING ("Can't set text alignment: Invalid type (%d)", align);
			break;
	}
	return TextAlign;
}


Text fontGenerate (const char * text, enum textAlignType align, int x, int y, int w)
{
	Text
		words = xph_alloc (sizeof (struct font_textcache));

	unsigned int
		width, height;
	int
		i = 0,
		lineAdvance = 0,
		tabAdvance = 0;
	unsigned char
		c;
	float
		glX, glY, glW, glH,
		gltX, gltY, gltW, gltH,
		glTB,
		zNear = video_getZnear () - 0.001;

	words->vertices = xph_alloc (sizeof (float) * 4 * 3 * strlen (text));
	words->textures = xph_alloc (sizeof (float) * 4 * 2 * strlen (text));
	words->glyphs = 0;

	video_getDimensions (&width, &height);
	if (x < 0)
		glX = video_xMap (width + x);
	else
		glX = video_xMap (x);
	if (y < 0)
		glY = video_yMap (height + (y + FontSizeInPx));
	else
		glY = video_yMap (y + FontSizeInPx);

	// this is obnoxious but a right-aligned line has to be stepped through backwards and it's just easier to write it its own traversal code rather than try to make the standard loop go backwards or forwards depending - xph 2012 01 08
	if (align == ALIGN_RIGHT)
	{
		const char
			* nextLine = text;
		bool
			doneReading = false;
		nextLine = strchr (nextLine + 1, '\n');
		if (nextLine)
			i = nextLine - text;
		else
			i = strlen (text);
		c = text[i];
		while (!doneReading)
		{
			c = text[--i];
			switch (c)
			{
				case '\n':
					glX = video_xMap (x);
					glY += video_yOffset (FontSizeInPx);
					lineAdvance = 0;

					// can't just break since we're in a switch, but since hitting this means we're at the end of the text; you can just return - xph 2012 01 08
					if (!nextLine)
					{
						doneReading = true;
						break;
					}

					nextLine = strchr (nextLine + 1, '\n');
					if (nextLine)
						i = nextLine - text;
					else
						i = strlen (text);
					continue;
				case ' ':
					glX -= video_xOffset (GlyphSheet->glyphs[c].xadvance);
					lineAdvance -= GlyphSheet->glyphs[c].xadvance;
					continue;
			}
			if (doneReading)
				break;
			glW = video_xOffset (GlyphSheet->glyphs[c].width);
			glH = video_yOffset (GlyphSheet->glyphs[c].height);
			glTB = video_yOffset (-GlyphSheet->glyphs[c].topBearing);

			glX -= video_xOffset (GlyphSheet->glyphs[c].xadvance);
			lineAdvance -= GlyphSheet->glyphs[c].xadvance;
			fontTexelCoords (c, &gltX, &gltY, &gltW, &gltH);

			words->vertices[words->glyphs * 3 + 0] = glX;
			words->vertices[words->glyphs * 3 + 1] = glY + glTB + glH;
			words->vertices[words->glyphs * 3 + 2] = zNear;
			words->textures[words->glyphs * 2 + 0] = gltX;
			words->textures[words->glyphs * 2 + 1] = gltY + gltH;
			words->glyphs++;
			words->vertices[words->glyphs * 3 + 0] = glX + glW;
			words->vertices[words->glyphs * 3 + 1] = glY + glTB + glH;
			words->vertices[words->glyphs * 3 + 2] = zNear;
			words->textures[words->glyphs * 2 + 0] = gltX + gltW;
			words->textures[words->glyphs * 2 + 1] = gltY + gltH;
			words->glyphs++;
			words->vertices[words->glyphs * 3 + 0] = glX + glW;
			words->vertices[words->glyphs * 3 + 1] = glY + glTB;
			words->vertices[words->glyphs * 3 + 2] = zNear;
			words->textures[words->glyphs * 2 + 0] = gltX + gltW;
			words->textures[words->glyphs * 2 + 1] = gltY;
			words->glyphs++;
			words->vertices[words->glyphs * 3 + 0] = glX;
			words->vertices[words->glyphs * 3 + 1] = glY + glTB;
			words->vertices[words->glyphs * 3 + 2] = zNear;
			words->textures[words->glyphs * 2 + 0] = gltX;
			words->textures[words->glyphs * 2 + 1] = gltY;
			words->glyphs++;

			if (i == 0)
			{
				glX = video_xMap (x);
				glY += video_yOffset (FontSizeInPx);
				lineAdvance = 0;

				if (!nextLine)
					break;
				nextLine = strchr (nextLine + 1, '\n');
				if (nextLine)
					i = nextLine - text;
				else
					i = strlen (text);
				continue;
			}
		}
	}
	else
	{
		while ((c = text[i++]))
		{
			switch (c)
			{
				case '\n':
					glX = video_xMap (x);
					glY += video_yOffset (FontSizeInPx);
					lineAdvance = 0;
					continue;
				case '\t':
					tabAdvance = (GlyphSheet->glyphs[' '].xadvance * 8) - lineAdvance % (GlyphSheet->glyphs[' '].xadvance * 8);
					lineAdvance += tabAdvance;
					glX += video_xOffset (tabAdvance);
					continue;
				case ' ':
					glX += video_xOffset (GlyphSheet->glyphs[c].xadvance);
					lineAdvance += GlyphSheet->glyphs[c].xadvance;
					continue;
				default:
					break;
			}
			glW = video_xOffset (GlyphSheet->glyphs[c].width);
			glH = video_yOffset (GlyphSheet->glyphs[c].height);
			glTB = video_yOffset (-GlyphSheet->glyphs[c].topBearing);
			fontTexelCoords (c, &gltX, &gltY, &gltW, &gltH);

			words->vertices[words->glyphs * 3 + 0] = glX;
			words->vertices[words->glyphs * 3 + 1] = glY + glTB + glH;
			words->vertices[words->glyphs * 3 + 2] = zNear;
			words->textures[words->glyphs * 2 + 0] = gltX;
			words->textures[words->glyphs * 2 + 1] = gltY + gltH;
			words->glyphs++;
			words->vertices[words->glyphs * 3 + 0] = glX + glW;
			words->vertices[words->glyphs * 3 + 1] = glY + glTB + glH;
			words->vertices[words->glyphs * 3 + 2] = zNear;
			words->textures[words->glyphs * 2 + 0] = gltX + gltW;
			words->textures[words->glyphs * 2 + 1] = gltY + gltH;
			words->glyphs++;
			words->vertices[words->glyphs * 3 + 0] = glX + glW;
			words->vertices[words->glyphs * 3 + 1] = glY + glTB;
			words->vertices[words->glyphs * 3 + 2] = zNear;
			words->textures[words->glyphs * 2 + 0] = gltX + gltW;
			words->textures[words->glyphs * 2 + 1] = gltY;
			words->glyphs++;
			words->vertices[words->glyphs * 3 + 0] = glX;
			words->vertices[words->glyphs * 3 + 1] = glY + glTB;
			words->vertices[words->glyphs * 3 + 2] = zNear;
			words->textures[words->glyphs * 2 + 0] = gltX;
			words->textures[words->glyphs * 2 + 1] = gltY;
			words->glyphs++;

			glX += video_xOffset (GlyphSheet->glyphs[c].xadvance);
			lineAdvance += GlyphSheet->glyphs[c].xadvance;
		}
	}

	return words;
}

void fontDestroyText (Text t)
{
	xph_free (t->vertices);
	xph_free (t->textures);
	xph_free (t);
}

/*
{
	// * 4 since there are four points per quad; * 3/2 since there are 3/2 values per point (3 space, 2 texture)
	float
		* glyphQuads = xph_alloc (sizeof (float) * 4 * 3 * strlen (text)),
		* glyphTexs = xph_alloc (sizeof (float) * 4 * 2 * strlen (text));
	int
		quadsUsed = 0;
	GLuint
		TextBuffer;

	// generate buffer
	glGenBuffer (1, &TextBuffer);

	// activate buffer
	glBindBuffer (GL_ARRAY_BUFFER, TextBuffer);

	// store data in buffer
	glBufferData (GL_ARRAY_BUFFER, quadsUsed, glyphQuads, GL_STATIC_DRAW);

	// draw buffer
	glEnableVertexAttribArray (0);
	glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glVertexPointer (3, GL_FLOAT, 0, glyphQuads);
	glTexCoordPointer (2, GL_FLOAT, 0, glyphTexs);
	glDrawArrays (GL_QUADS, 0, quadsUsed);
}
*/


static void fontTexelCoords (unsigned int point, float * x, float * y, float * w, float * h)
{
	assert (point < GLYPH_COUNT);
	assert (x && y && w && h);

	*x = (float)GlyphSheet->glyphWidth * (point % 16) / GlyphSheet->width;
	*y = GlyphSheet->height - ((float)GlyphSheet->glyphHeight * (point / 16) / GlyphSheet->height);
	*w = (float)GlyphSheet->glyphs[point].width / GlyphSheet->width;
	*h = -(float)GlyphSheet->glyphs[point].height / GlyphSheet->height;
}

// TODO: a 'width' argument to allow for centred or justified text (+ automatic word-wrapping?)
void fontPrint (const char * text, int x, int y)
{
	Text
		words;
	if (text[0] == 0)
		return;
	words = fontGenerate (text, TextAlign, x, y, 0);
	if (words->glyphs != 0)
		fontTextPrint (words);
	fontDestroyText (words);
	return;
}

void fontTextPrint (Text t)
{
	glEnableClientState (GL_VERTEX_ARRAY);
	glEnableClientState (GL_TEXTURE_COORD_ARRAY);

	glBindTexture (GL_TEXTURE_2D, GlyphSheet->name);

	glVertexPointer (3, GL_FLOAT, 0, t->vertices);
	glTexCoordPointer (2, GL_FLOAT, 0, t->textures);
	glDrawArrays (GL_QUADS, 0, t->glyphs);

	glDisableClientState (GL_VERTEX_ARRAY);
	glDisableClientState (GL_TEXTURE_COORD_ARRAY);

	glBindTexture (GL_TEXTURE_2D, 0);
}
