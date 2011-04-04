#include "hex_draw.h"

static float rgb[3] = {0.75, 0.7, 0.65};

void hex_setDrawColor (float red, float green, float blue)
{
	rgb[0] = red;
	rgb[1] = green;
	rgb[2] = blue;
}

void hex_draw (const Hex hex, const Entity camera) {
	unsigned int
		corners[6] = {
			FULLHEIGHT (hex, 0),
			FULLHEIGHT (hex, 1),
			FULLHEIGHT (hex, 2),
			FULLHEIGHT (hex, 3),
			FULLHEIGHT (hex, 4),
			FULLHEIGHT (hex, 5)
		};
  int
	i, j;
  bool
    light = FALSE;
  VECTOR3
    pos = hex_coord2space (hex->r, hex->k, hex->i);
  if ((hex->k & 0x01) ^ (hex->i & 0x01) ^ (hex->r & 0x01)) {
    light = TRUE;
  }
  if (light == TRUE) {
    glColor3f (rgb[0] * 1.5, rgb[1] * 1.5, rgb[2] * 1.5);
  } else {
    glColor3f (rgb[0] * 0.75, rgb[1] * 0.75, rgb[2] * 0.75);
  }
  glBegin (GL_TRIANGLE_FAN);
	glVertex3f (pos.x, hex->centre * HEX_SIZE_4, pos.z);
  glVertex3f (pos.x + H[0][0], corners[0] * HEX_SIZE_4, pos.z + H[0][1]);
  glVertex3f (pos.x + H[5][0], corners[5] * HEX_SIZE_4, pos.z + H[5][1]);
  glVertex3f (pos.x + H[4][0], corners[4] * HEX_SIZE_4, pos.z + H[4][1]);
  glVertex3f (pos.x + H[3][0], corners[3] * HEX_SIZE_4, pos.z + H[3][1]);
  glVertex3f (pos.x + H[2][0], corners[2] * HEX_SIZE_4, pos.z + H[2][1]);
  glVertex3f (pos.x + H[1][0], corners[1] * HEX_SIZE_4, pos.z + H[1][1]);
  glVertex3f (pos.x + H[0][0], corners[0] * HEX_SIZE_4, pos.z + H[0][1]);
  glEnd ();

  if (light == TRUE) {
    glColor3f (rgb[0] * 1.4, rgb[1] * 1.4, rgb[2] * 1.4);
  } else {
    glColor3f (rgb[0] * 0.7, rgb[1] * 0.7, rgb[2] * 0.7);
  }
	i = 0;
	j = 1;
	while (i < 6)
	{
		if (hex->edgeBase[i * 2] >= corners[i] &&
			hex->edgeBase[i * 2 + 1] >= corners[j])
		{
			i++;
			j = (i + 1) % 6;
			continue;
		}
		glBegin (GL_TRIANGLE_STRIP);
		glVertex3f (pos.x + H[i][0], corners[i] * HEX_SIZE_4, pos.z + H[i][1]);
		glVertex3f (pos.x + H[j][0], corners[j] * HEX_SIZE_4, pos.z + H[j][1]);
		glVertex3f (pos.x + H[i][0], (hex->edgeBase[i * 2]) * HEX_SIZE_4, pos.z + H[i][1]);
		glVertex3f (pos.x + H[j][0], (hex->edgeBase[i * 2 + 1]) * HEX_SIZE_4, pos.z + H[j][1]);
		glEnd ();
		i++;
		j = (i + 1) % 6;
	}
/*
  while (i < 6) {
    glBegin (GL_TRIANGLE_STRIP);
    glVertex3f (pos.x + H[i][0], pos.y + corners[i], pos.z + H[i][1]);
    glVertex3f (pos.x + H[j][0], pos.y + corners[j], pos.z + H[j][1]);
    glVertex3f (pos.x + H[i][0], pos.y, pos.z + H[i][1]);
    glVertex3f (pos.x + H[j][0], pos.y, pos.z + H[j][1]);
    glEnd ();
    i++;
    j = (i + 1) % 6;
  }
*/
}

void hex_drawFiller (const CameraGroundLabel label, signed int scale)
{
	signed int
		rs = scale * 2;
	VECTOR3
		o = label_getOriginOffset (label);
	glColor3f (rgb[0], rgb[1], rgb[2]);
	glBegin (GL_TRIANGLE_FAN);
	glVertex3f (o.x + H[0][1] * rs, o.y, o.z + H[0][0] * rs);
	glVertex3f (o.x + H[1][1] * rs, o.y, o.z + H[1][0] * rs);
	glVertex3f (o.x + H[2][1] * rs, o.y, o.z + H[2][0] * rs);
	glVertex3f (o.x + H[3][1] * rs, o.y, o.z + H[3][0] * rs);
	glVertex3f (o.x + H[4][1] * rs, o.y, o.z + H[4][0] * rs);
	glVertex3f (o.x + H[5][1] * rs, o.y, o.z + H[5][0] * rs);
	glVertex3f (o.x + H[0][1] * rs, o.y, o.z + H[0][0] * rs);
	glEnd ();
}
