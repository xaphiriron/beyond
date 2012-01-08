#include "font.h"

static int next_power (int n)
{
	int
		r = 1;
	while (r < n)
		r <<= 1;
	return r;
}

struct font_char
{
	int
		width,
		height,
		xadvance,
		topBearing;
	float
		texWidth,
		texHeight;
	unsigned char
		* data;
};

static FT_Library
	Library = NULL;
static FT_Face
	Face = NULL;
static GLuint
	GlyphNames[128];
static struct font_char
	Glyphs[128];
static int
	FontSizeInPx = 0;

static enum textAlignType
	TextAlign = ALIGN_LEFT;

void fontLoad (const char * path, int fontSize)
{
	int
		error = 0,
		width,
		height,
		i, max,
		x, y;
	FT_Bitmap
		* bitmap = NULL;
	unsigned char
		c = 0,
		* textureData;
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

	glGenTextures (128, GlyphNames);
	bitmap = &Face->glyph->bitmap;
	while (c < 128)
	{
		FT_Load_Char (Face, c, FT_LOAD_RENDER);
		width = next_power (bitmap->width);
		height = next_power (bitmap->rows);
		Glyphs[c].width = bitmap->width;
		Glyphs[c].height = bitmap->rows;
		Glyphs[c].texWidth = (float)bitmap->width / width;
		Glyphs[c].texHeight = (float)bitmap->rows / height;

		Glyphs[c].topBearing = Face->glyph->metrics.horiBearingY >> 6;
		Glyphs[c].xadvance = Face->glyph->advance.x >> 6;

		// we're using a 2-component GL_LUMINANCE_ALPHA texture (and setting the luminance to 0xff always) because even though you might assume you could just use a GL_ALPHA texture evidently you can't. or, more likely, setting that to work correctly is slightly more complex than just changing the way the data is set and parsed (i guess??) - xph 2012 01 08
		Glyphs[c].data = xph_alloc (width * height * 2);
		textureData = Glyphs[c].data;
		memset (textureData, 0x7f, width * height * 2);
		assert (bitmap->pixel_mode == FT_PIXEL_MODE_GRAY);
		i = 0;
		max = bitmap->rows * bitmap->width;
		while (i < max)
		{
			x = i % bitmap->width;
			y = i / bitmap->width;
			// set texel to pure white
			memset (&textureData [(((height - (height - bitmap->rows)- 1) - y) * width + x) * 2], 0xff, 1);
			// set texel transparency to the correct alpha
			memset (&textureData [(((height - (height - bitmap->rows)- 1) - y) * width + x) * 2 + 1], bitmap->buffer [i], 1);
			i++;
		}

		assert (GlyphNames[c] != 0);
		glBindTexture (GL_TEXTURE_2D, GlyphNames[c]);
		glTexImage2D (GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, width, height, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, textureData);
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		c++;
	}
}

void fontUnload ()
{
	int
		i = 0;
	if (!Face)
		return;
	FontSizeInPx = 0;
	while (i < 128)
	{
		xph_free (Glyphs[i].data);
		i++;
	}
	memset (Glyphs, 0, sizeof (struct font_char) * 128);
	glDeleteTextures (128, GlyphNames);
	memset (GlyphNames, 0, sizeof (GLuint) * 128);
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


// TODO: a 'width' argument to allow for centred or justified text (+ automatic word-wrapping?)
void fontPrint (const char * text, int x, int y)
{
	unsigned int
		width, height;
	int
		i = 0,
		lineAdvance = 0,
		tabAdvance = 0;
	unsigned char
		c;
	float
		glX, glY,
		glW, glH,
		glTB,
		zNear = video_getZnear () - 0.001;
	video_getDimensions (&width, &height);
	if (x < 0)
		glX = video_xMap (width + x);
	else
		glX = video_xMap (x);
	if (y < 0)
		glY = video_yMap (height + (y + FontSizeInPx));
	else
		glY = video_yMap (y + FontSizeInPx);

	if (TextAlign == ALIGN_RIGHT)
	{
		// this is obnoxious but a right-aligned line has to be stepped through backwards and it's just easier to write it its own traversal code rather than try to make the standard loop go backwards or forwards depending - xph 2012 01 08
		const char
			* nextLine = text;
		nextLine = strchr (nextLine + 1, '\n');
		if (nextLine)
			i = nextLine - text;
		else
			i = strlen (text);
		c = text[i];
		while ((c = text[--i]))
		{
			switch (c)
			{
				case '\n':
					glX = video_xMap (x);
					glY += video_yOffset (FontSizeInPx);
					lineAdvance = 0;

					// can't just break since we're in a switch, but since hitting this means we're at the end of the text youcan just return - xph 2012 01 08
					if (!nextLine)
						return;
					nextLine = strchr (nextLine + 1, '\n');
					if (nextLine)
						i = nextLine - text;
					else
						i = strlen (text);
					continue;
				case ' ':
					glX -= video_xOffset (Glyphs[c].xadvance);
					lineAdvance -= Glyphs[c].xadvance;
					continue;
			}
			glW = video_xOffset (Glyphs[c].width);
			glH = video_yOffset (Glyphs[c].height);
			glTB = video_yOffset (-Glyphs[c].topBearing);

			glX -= video_xOffset (Glyphs[c].xadvance);
			lineAdvance -= Glyphs[c].xadvance;

			glBindTexture (GL_TEXTURE_2D, GlyphNames[c]);
			glBegin (GL_QUADS);
			glTexCoord2f (0, 0);
			glVertex3f (glX, glY + glTB + glH, zNear);
			glTexCoord2f (Glyphs[c].texWidth, 0);
			glVertex3f (glX + glW, glY + glTB + glH, zNear);
			glTexCoord2f (Glyphs[c].texWidth, Glyphs[c].texHeight);
			glVertex3f (glX + glW, glY + glTB, zNear);
			glTexCoord2f (0, Glyphs[c].texHeight);
			glVertex3f (glX, glY + glTB, zNear);
			glEnd ();

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
		printf ("\n");
		return;
	}

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
				tabAdvance = (Glyphs[' '].xadvance * 8) - lineAdvance % (Glyphs[' '].xadvance * 8);
				lineAdvance += tabAdvance;
				glX += video_xOffset (tabAdvance);
				continue;
			case ' ':
				glX += video_xOffset (Glyphs[c].xadvance);
				lineAdvance += Glyphs[c].xadvance;
				continue;
			default:
				break;
		}
		glW = video_xOffset (Glyphs[c].width);
		glH = video_yOffset (Glyphs[c].height);
		glTB = video_yOffset (-Glyphs[c].topBearing);

		glBindTexture (GL_TEXTURE_2D, GlyphNames[c]);
		glBegin (GL_QUADS);
		glTexCoord2f (0, 0);
		glVertex3f (glX, glY + glTB + glH, zNear);
		glTexCoord2f (Glyphs[c].texWidth, 0);
		glVertex3f (glX + glW, glY + glTB + glH, zNear);
		glTexCoord2f (Glyphs[c].texWidth, Glyphs[c].texHeight);
		glVertex3f (glX + glW, glY + glTB, zNear);
		glTexCoord2f (0, Glyphs[c].texHeight);
		glVertex3f (glX, glY + glTB, zNear);
		glEnd ();

		glX += video_xOffset (Glyphs[c].xadvance);
		lineAdvance += Glyphs[c].xadvance;
	}
}
