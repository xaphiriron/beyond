#include "font.h"

#include <GL/glpng.h>
#include "texture.h"
#include "video.h"
#include "sprite.h"
#include "xph_log.h"

static SPRITESHEET
	SystemFont = NULL;
void loadFont (const char * path)
{
	if (SystemFont != NULL)
	{
		WARNING ("Resetting system font (with \"%s\")", path);
		sheetDestroy (SystemFont);
		SystemFont = NULL;
	}

	// TODO: ugh fix this so it works when the orientation is set correctly - xph 2011 09 27
	pngSetStandardOrientation (0);
	SystemFont = sheetCreate (path, SHEET_MULTISIZE);
}

int systemLineHeight ()
{
	if (SystemFont == NULL)
		return -1;
	return sheetGetHeightRange (SystemFont);
}

static enum textAlignType
	TextAlign = ALIGN_LEFT;
enum textAlignType textAlign (enum textAlignType align)
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

void drawLine (const char * line, signed int x, signed int y)
{
	float
		glX = video_pixelXMap (x),
		glY = video_pixelYMap (y),
		glLetterSpacing = video_pixelXOffset (1),// how many px of space between letters
		glLineSpacing = 0,// how many px of space between lines; set below
		glLetterWidth = 0,
		glLetterHeight = 0,
		glLetterHeightOffset = 0,
		glNear = video_getZnear (),
		startingLine = glX,
		tx, ty, tw, th;
	signed int
		lx, ly,
		lw, lh,
		lf, lg,
		sheetWidth,
		sheetHeight;
	const char
		* c = line,
		* nextLine = line;
	SPRITE
		letter;
	sheetGetTextureSize (SystemFont, &sheetWidth, &sheetHeight);
	glY += video_pixelYOffset (sheetGetGBaselineOffset (SystemFont));
	glLineSpacing = video_pixelYOffset (sheetGetHeightRange (SystemFont));

	glBindTexture (GL_TEXTURE_2D, textureName (sheetGetTexture (SystemFont)));
	glBegin (GL_QUADS);

	if (TextAlign == ALIGN_LEFT)
		c = line;
	else if (TextAlign == ALIGN_RIGHT)
	{
		c = (nextLine = strchr (nextLine + 1, '\n')) == NULL
			? line + strlen (line)
			: nextLine;
		c--;
	}

	while (
		(TextAlign == ALIGN_LEFT && *c != 0) ||
		(TextAlign == ALIGN_RIGHT && c >= line)
	)
	{
		letter = sheetGetSpriteViaOffset (SystemFont, *c);

		if (TextAlign == ALIGN_LEFT && *c == '\n')
		{
			glX = startingLine;
			glY += glLineSpacing;
			c++;
			continue;
		}
		//DEBUG ("got sprite %p (for '%c')", letter, *c);
		spriteGetXY (letter, &lx, &ly);
		spriteGetWH (letter, &lw, &lh);
		spriteGetFG (letter, &lf, &lg);
		tx = (float)lx / sheetWidth;
		ty = (float)ly / sheetHeight;
		tw = (float)lw / sheetWidth;
		th = (float)lh / sheetHeight;
		glLetterHeightOffset = -video_pixelYOffset (lg);
		glLetterWidth = video_pixelXOffset (lw);
		glLetterHeight = video_pixelYOffset (lh);
		
		//DEBUG ("height offset of %c: %d", *c, lg);

		if (TextAlign == ALIGN_RIGHT)
			glX -= (glLetterWidth + glLetterSpacing);

		glTexCoord2f (tx, ty);
		glVertex3f (glX, glY + glLetterHeightOffset, glNear);
		glTexCoord2f (tx, ty + th);
		glVertex3f (glX, glY + glLetterHeightOffset + glLetterHeight, glNear);
		glTexCoord2f (tx + tw, ty + th);
		glVertex3f (glX + glLetterWidth, glY + glLetterHeightOffset + glLetterHeight, glNear);
		glTexCoord2f (tx + tw, ty);
		glVertex3f (glX + glLetterWidth, glY + glLetterHeightOffset, glNear);

		if (TextAlign == ALIGN_LEFT)
		{
			glX += (glLetterWidth + glLetterSpacing);
			c++;
		}
		else if (TextAlign == ALIGN_RIGHT)
		{
			if (*c == '\n' || c == line)
			{
				if (nextLine == NULL)
					break;
				glX = startingLine;
				glY += glLineSpacing;
				c = (nextLine = strchr (nextLine + 1, '\n')) == NULL
					? line + strlen (line)
					: nextLine;
			}
			c--;
		}
	}
	glEnd ();
}
