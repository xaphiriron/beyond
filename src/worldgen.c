#include "worldgen.h"

#include "bit.h"
#include "system.h"

#include "hex_utility.h"
#include "map.h"
#include "map_internal.h"

GRAPH
	WorldGraph = NULL;

struct worldgenPattern
{
	
};

/*
struct worldgenArch // ARCH
{
	SUBHEX
		centred;
	


	PATTERN
		pattern;
	bool
		expanded;
	GRAPH
		subpatterns;	// ???
	// ???
};
*/

struct worldgenArch // ARCH
{
	SUBHEX
		centre;

	VECTOR3
		position;
	unsigned int
		height;
	signed int
		x, y;

};

void worldgenAbsHocNihilo ()
{
	FUNCOPEN ();
/*
	PATTERN
		firstPattern = patternCreate ();	// from patterns file
*/
	
	mapSetSpanAndRadius (7, 8);
	mapGeneratePoles (POLE_TRI);

	FUNCCLOSE ();
}

void worldgenFinalizeCreation ()
{
	FUNCOPEN ();

	systemCreatePlayer ();

	systemClearStates();
	systemPushState (STATE_FREEVIEW);
	FUNCCLOSE ();
}

void worldgenExpandWorldGraph (TIMER t)
{
	static SUBHEX
		pole = NULL,
		base = NULL;
	SUBHEX
		active = NULL;
	RELATIVEHEX
		rel = NULL;
	FUNCOPEN ();

	loadSetGoal (2);

	if (!pole)
		pole = mapPole ('r');

	if (!base)
	{
		mapForceGrowAtLevelForDistance (pole, 1, 3);
		rel = mapRelativeSubhexWithCoordinateOffset (pole, -6, 0, 0);
		base = mapRelativeTarget (rel);
		mapRelativeDestroy (rel);
	}

	loadSetLoaded (1);

	active = base;

	while (1)
	{
		// all this is just worldgen scratchpad stuff
/*
		worldgenCreateArch (NULL, active);
		active = mapHexAtCoordinateAuto (subhexParent (base), 1, 0);
		worldgenCreateArch (NULL, active);
		active = mapHexAtCoordinateAuto (subhexParent (base), 0, 1);
		worldgenCreateArch (NULL, active);
		active = mapHexAtCoordinateAuto (subhexParent (base), -1, 1);
		worldgenCreateArch (NULL, active);
		active = mapHexAtCoordinateAuto (subhexParent (base), -1, 0);
		worldgenCreateArch (NULL, active);
		active = mapHexAtCoordinateAuto (subhexParent (base), 0, -1);
		worldgenCreateArch (NULL, active);
		active = mapHexAtCoordinateAuto (subhexParent (base), 1, -1);
		worldgenCreateArch (NULL, active);
*/

		mapDataAdd (base, "height", 16);

		active = mapHexAtCoordinateAuto (subhexParent (base), 1, 0);
		mapDataAdd (active, "height", 32);

		active = mapHexAtCoordinateAuto (subhexParent (base), 0, 1);
		mapDataAdd (active, "height", 16);

		active = mapHexAtCoordinateAuto (subhexParent (base), -1, 1);
		mapDataAdd (active, "height", 16);

		active = mapHexAtCoordinateAuto (subhexParent (base), -1, 0);
		mapDataAdd (active, "height", 8);

		active = mapHexAtCoordinateAuto (subhexParent (base), 0, -1);
		mapDataAdd (active, "height", 36);

		active = mapHexAtCoordinateAuto (subhexParent (base), 1, -1);
		mapDataAdd (active, "height", 12);


		active = mapHexAtCoordinateAuto (subhexParent (base), -1, 2);
		mapDataAdd (active, "height", 16);

		break;
		if (outOfTime (t))
			return;
	}

	loadSetLoaded (2);

	FUNCCLOSE ();
}

ARCH worldgenBuildArch (ARCH parent, unsigned int subid, PATTERN pattern)
{
	return NULL;
}

ARCH worldgenCreateArch (PATTERN pattern, SUBHEX base)
{
	ARCH
		arch = xph_alloc (sizeof (struct worldgenArch));
	unsigned int
		r, k, i;

	memset (arch, '\0', sizeof (struct worldgenArch));

	arch->centre = base;

	// do the pattern setup here!!!
	r = rand () % MapRadius;
	k = rand () % 6;
	i = rand () % MapRadius - 1;
	hex_rki2xy (r, k, i, &arch->x, &arch->y);
	arch->height = (rand () & 0x1f) + 5;

	mapArchSet (base, arch);
	return arch;
}

void worldgenExpandArchGraph (ARCH p, unsigned int depth)
{
}

/***
 * PATTERN FUNCTIONS
 */

PATTERN patternCreate ()
{
	PATTERN
		p = xph_alloc (sizeof (struct worldgenPattern));
	return p;
}

/***
 * IMPRINTING FUNCTIONS
 */

void worldgenImprintMapData (SUBHEX at)
{
	unsigned int
		height = mapDataGet (at, "height"),
		adjHeight[6],
		r = 0, i = 0;
	signed int
		* centres = mapSpanCentres (1),
		dir = 0,
		nextDir = 1,
		x, y;
	float
		b[4] = {0, 0, 0, 0};
	VECTOR3
		p,
		adjCoords[6];
	SUBHEX
		hex = NULL;
	if (subhexSpanLevel (at) != 1)
		return;

	hex = mapHexAtCoordinateAuto (at, 0, 0);
	hex->hex.centre = height;

	subhexLocalCoordinates (at, &x, &y);
	while (dir < 6)
	{
		adjCoords[dir] = hex_xyCoord2Space (centres[dir * 2], centres[dir * 2 + 1]);
		adjHeight[dir] = mapDataGet (mapHexAtCoordinateAuto (subhexParent (at), x + XY[dir][X], y + XY[dir][Y]), "height");
		dir++;
	}

	dir = 0;
	nextDir = 5;
	while (dir < 6)
	{
		r = 1;
		i = 0;
		while (r <= MapRadius)
		{
			hex_rki2xy (r, dir, i, &x, &y);
			hex = mapHexAtCoordinateAuto (at, x, y);
			if (hex == NULL)
				goto increment;
			p = hex_coord2space (r, dir, i);
			/* the full method for calculating the barycentric coordinates of a point :p {0} with three vertices (in this case, 0,0 implicitly, :adjCoords[dir], and :adjCoords[nextDir] {1,2,3}) is the first calculate a normalizing factor, bc, by way of
			* b0 = (2.x - 1.x) * (3.y - 1.y) - (3.x - 1.x) * (2.y - 1.y);
			* in this case since the '1' is 0,0 this simplifies things substantially
			* from that the coordinates themselves are calculated:
			* b1 = ((2.x - 0.x) * (y.3 - 0.y) - (3.x - 0.x) * (2.y -0.y)) / bc;
			* b2 = ((3.x - 0.x) * (y.1 - 0.y) - (1.x - 0.x) * (3.y -0.y)) / bc;
			* b3 = ((1.x - 0.x) * (y.2 - 0.y) - (2.x - 0.x) * (1.y -0.y)) / bc;
			* which, again, can be simplified on the values involving 1, since
			* it's always 0,0. remember also that the world plane is the x,z
			* plane; that's why we're using z here instead of y. to interpolate
			* any map data point, multiply the coordinate values with their respective map point value, so that:
			* value at :p = b1 * [map value of the subhex corresponding to 1] + b2 * [same] + b3 * same;
			 */
			b[0] = adjCoords[dir].x * adjCoords[nextDir].z - adjCoords[nextDir].x * adjCoords[dir].z;
			b[1] = ((adjCoords[dir].x - p.x) * (adjCoords[nextDir].z - p.z) -
					(adjCoords[nextDir].x - p.x) * (adjCoords[dir].z - p.z)) / b[0];
			b[2] = ((adjCoords[nextDir].x - p.x) * -p.z -
					-p.x * (adjCoords[nextDir].z - p.z)) / b[0];
			b[3] = (-p.x * (adjCoords[dir].z - p.z) -
					(adjCoords[dir].x - p.x) * -p.z) / b[0];
			hex->hex.centre = b[1] * height + b[2] * adjHeight[dir] + b[3] * adjHeight[nextDir];
			if ((signed int)hex->hex.centre < 0)
				hex->hex.centre = 0;
			//WARNING ("final height: %d, from %f * %d + %f * %d + %f * %d", hex->hex.centre, b[1], height, b[2], adjHeight[dir], b[3], adjHeight[nextDir]);
			
			increment:
			i++;
			if (i == r)
			{
				r++;
				i = 0;
			}
		}
		dir++;
		nextDir = (dir + 5) % 6;
	}
}

void worldgenImprintAllArches (SUBHEX at)
{
	ARCH
		arch = NULL;
	SUBHEX
		hex = NULL,
		closer = NULL;
	int
		offset = 0;
	signed int
		hx, hy,
		cx, cy,
		height;
	unsigned int
		r, k, i;
	if (subhexSpanLevel (at) < 1)
	{
		WARNING ("Can't imprint platter (%p) with span level %d.", at, subhexSpanLevel (at));
		return;
	}
	if (at->sub.imprinted == TRUE)
		return;
	height = 2;
	while ((arch = mapArchGet (at, offset++)) != NULL)
	{
		hex = mapHexAtCoordinateAuto (at, arch->x, arch->y);
		if (hex->hex.centre < arch->height)
			hex->hex.centre = arch->height;
		r = 1;
		k = i = 0;
		while (r * 2 < arch->height)
		{
			hex_rki2xy (r, k, i, &hx, &hy);
			hx += arch->x;
			hy += arch->y;
			hex = mapHexAtCoordinateAuto (at, hx, hy);
			if (hex == NULL)
			{
				hex_nextValidCoord (&r, &k, &i);
				continue;
			}
			hex_stepLineToOrigin (hx - arch->x,hy - arch->y, 1, &cx, &cy);
			closer = mapHexAtCoordinateAuto (at, arch->x + cx, arch->y + cy);
			DEBUG ("from %d, %d got 'closer' hex %d, %d", hx, hy, arch->x + cx, arch->y + cy);
			if (closer == NULL)
			{
				// ?!?
				hex_nextValidCoord (&r, &k, &i);
				continue;
			}
			if (height > closer->hex.centre || hex->hex.centre >= closer->hex.centre - height)
			{
				hex_nextValidCoord (&r, &k, &i);
				continue;
			}
			if (i == 0)
			{
				SETCORNER (hex->hex.corners, k, -1);
				SETCORNER (hex->hex.corners, (k + 1) % 6, 0);
				SETCORNER (hex->hex.corners, (k + 2) % 6, 1);
				SETCORNER (hex->hex.corners, (k + 3) % 6, 1);
				SETCORNER (hex->hex.corners, (k + 4) % 6, 0);
				SETCORNER (hex->hex.corners, (k + 5) % 6, -1);
			}
			else
			{
				SETCORNER (hex->hex.corners, k, -1);
				SETCORNER (hex->hex.corners, (k + 1) % 6, 0);
				SETCORNER (hex->hex.corners, (k + 2) % 6, 1);
				SETCORNER (hex->hex.corners, (k + 3) % 6, 2);
				SETCORNER (hex->hex.corners, (k + 4) % 6, 1);
				SETCORNER (hex->hex.corners, (k + 5) % 6, 0);
			}
			hex->hex.centre = closer->hex.centre - height;
			hex_nextValidCoord (&r, &k, &i);
		}
		hex = mapHexAtCoordinateAuto (at, arch->x, arch->y);
		hex->hex.centre = arch->height - 1;
		hex->hex.corners[0] = 0;
		hex->hex.corners[1] = 0;
		hex->hex.corners[2] = 0;
	}
	at->sub.imprinted = TRUE;
	mapBakeHexes (at);

}