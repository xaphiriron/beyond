#include "hex_draw.h"

static float rgb[3] = {1.0, 1.0, 1.0};

void hex_setDrawColor (float red, float green, float blue) {
  rgb[0] = red;
  rgb[1] = green;
  rgb[2] = blue;
}

void hex_draw (const Hex hex, const Entity camera, const CameraGroundLabel label) {
  float
    ra = hex->top - hex->topA,
    rb = hex->top - hex->topB,
    d = hex->top + ra,
    e = hex->top + rb,
    // I don't know why these two work, but they do.
    c = hex->top + (ra + (hex->top - e)),
    f = hex->top + ((hex->top - d) + rb),
//     avg = (hex->topA + hex->topB + c + d + e + f) / 6.0,
    corners[6] = {hex->topA, hex->topB, c, d, e, f};
  int
    i = 0,
    j = 1,
    edge;/*,
	// that's 30 degrees in radians there
    cameraRot = (int)(((camera_getHeading (camera) + 0.52359877559829882) / (M_PI * 2) + 0.5) * 6) % 6*/
  bool
    light = FALSE;
  VECTOR3
    labelOffset = label_getOriginOffset (label),
    hexOffset = hex_coordOffset (hex->r, hex->k, hex->i),
    pos = vectorAdd (&labelOffset, &hexOffset);
  //printf ("\tlabel offset: %5.2f, %5.2f, %5.2f\n", labelOffset.x, labelOffset.y, labelOffset.z);
  //printf ("\thex offset:  %5.2f, %5.2f, %5.2f\n", hexOffset.x, hexOffset.y, hexOffset.z);
  if ((hex->i % 2) ^ !(hex->r % 2)) {
    light = TRUE;
  }
  if (light == TRUE) {
    glColor3f (rgb[0] * 1.05, rgb[1] * 1.05, rgb[2] * 1.05);
  } else {
    glColor3f (rgb[0], rgb[1], rgb[2]);
  }
  glBegin (GL_TRIANGLE_FAN);
  glVertex3f (pos.x + H[0][0], pos.y + corners[0], pos.z + H[0][1]);
  glVertex3f (pos.x + H[5][0], pos.y + corners[5], pos.z + H[5][1]);
  glVertex3f (pos.x + H[4][0], pos.y + corners[4], pos.z + H[4][1]);
  glVertex3f (pos.x + H[3][0], pos.y + corners[3], pos.z + H[3][1]);
  glVertex3f (pos.x + H[2][0], pos.y + corners[2], pos.z + H[2][1]);
  glVertex3f (pos.x + H[1][0], pos.y + corners[1], pos.z + H[1][1]);
  glVertex3f (pos.x + H[0][0], pos.y + corners[0], pos.z + H[0][1]);
  glEnd ();

/* Draw surface normals. (as of right now, normals are wrong when the ground they are on is rotated.)
  glColor3f (0, 0, 0);
  glBegin (GL_LINES);
  glVertex3f (pos.x, pos.y + avg, pos.z);
  glVertex3f (pos.x + hex->topNormal.x * 32, pos.y + avg + hex->topNormal.y * 32, pos.z + hex->topNormal.z * 32);
  glEnd ();
 */

  if (light == TRUE) {
    glColor3f (rgb[0] * 0.95, rgb[1] * 0.95, rgb[2] * 0.95);
  } else {
    glColor3f (rgb[0] * 0.9, rgb[1] * 0.9, rgb[2] * 0.9);
  }
  while (i < 6) {
	edge = i;
	if (hex->edgeDepth[edge * 2] >= corners[edge] && hex->edgeDepth[edge * 2 + 1] >= corners[j])
	{
		i++;
		j = (i + 1) % 6;
		continue;
	}
    glBegin (GL_TRIANGLE_STRIP);
    glVertex3f (pos.x + H[i][0], pos.y + corners[edge], pos.z + H[i][1]);
    glVertex3f (pos.x + H[j][0], pos.y + corners[j], pos.z + H[j][1]);
    glVertex3f (pos.x + H[i][0], pos.y + hex->edgeDepth[edge * 2], pos.z + H[i][1]);
    glVertex3f (pos.x + H[j][0], pos.y + hex->edgeDepth[edge * 2 + 1], pos.z + H[j][1]);
    glEnd ();
    i++;
    j = (i + 1) % 6;
  }
}
