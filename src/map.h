#ifndef XPH_MAP_H
#define XPH_MAP_H

#include <stdbool.h>
#include "vector.h"
#include "dynarr.h"
#include "texture.h"

#include "matspec.h"

#include "entity.h"

void mapLoad_system (Dynarr entities);


/***
 * MAP STUFF: not really sure how much of this stuff needs to be externally
 * visible.
 */

typedef union hexOrSubdiv * SUBHEX;

typedef struct hexSubdivided * SUBDIV;
typedef struct hexColumn * HEX;
typedef struct hexStep * HEXSTEP;

typedef struct hexRelativePosition * RELATIVEHEX;

// for the love of god use hexPos instead of relativehex or w/e; once everything uses it i can remove the relativehex code entirely and simplify things - xph 2011 12 04
typedef struct xph_world_hex_position * hexPos;

#include "worldgen.h"

enum hexOrSubdivType
{
	HS_HEX = 1,
	HS_SUB,
};

bool mapSetSpanAndRadius (const unsigned char span, const unsigned char radius);
unsigned char mapGetSpan ();
unsigned char mapGetRadius ();
bool mapGeneratePoles ();
bool worldExists ();

void worldDestroy ();

void mapForceSubdivide (SUBHEX subhex);
void mapForceGrowAtLevelForDistance (SUBHEX subhex, unsigned char absoluteSpan, unsigned int distance);
void mapForceGrowChildAt (SUBHEX subhex, signed int x, signed int y);

void mapLoadAround (hexPos pos);

bool hexPos_forceLoadTo (hexPos pos, unsigned char span);

/***
 * SIMPLE CREATION AND INITIALIZATION FUNCTIONS
 */

SUBHEX mapHexCreate (SUBHEX parent, signed int x, signed int y);
SUBHEX mapSubdivCreate (SUBHEX parent, signed int x, signed int y);

void subhexDestroy (SUBHEX subhex);

HEXSTEP hexSetBase (HEX hex, unsigned int height, MATSPEC material);
HEXSTEP hexCreateStep (HEX hex, unsigned int height, MATSPEC material);

HEXSTEP hexGroundStepNear (const HEX hex, unsigned int height);
void stepShiftHeight (HEX hex, HEXSTEP step, signed int shift);
void stepTransmute (HEX hex, HEXSTEP step, MATSPEC material, int penetration);

/***
 * LOCATIONS
 */

hexPos map_randomPos ();
hexPos map_randomPositionNear (const hexPos base, int range);
hexPos map_copy (const hexPos original);
hexPos map_at (const SUBHEX at);
hexPos map_from (const SUBHEX at, short relativeSpan, int x, int y);

void map_freePos (hexPos pos);

SUBHEX hexPos_platter (const hexPos pos, unsigned char focus);
unsigned char hexPos_focus (const hexPos pos);

SUBHEX map_posFocusedPlatter (const hexPos pos);
SUBHEX map_posBestMatchPlatter (const hexPos pos);
void map_posSwitchFocus (hexPos pos, unsigned char focus);
void map_posUpdateWith (hexPos pos, const SUBHEX div);

/* returns a dynarr of hexPos focused at the level of the subhex given, hitting all coordinates within a radius given by distance */
Dynarr map_posAround (const SUBHEX subhex, unsigned int distance);

/***
 * MAP TRAVERSAL FUNCTIONS
 */

/* returns TRUE if finish can be set to a span-1 subhex; FALSE otherwise
 *  (including on error, in which finish will be set NULL and newPosition will
 *  be set to the origin)
 */
bool mapMove (const SUBHEX start, const VECTOR3 * const position, SUBHEX * finish, VECTOR3 * newPosition);

SUBHEX mapPole (const char poleName);

/* FIXME: the two functions below both calculate pole-level connections (i.e., how the poles are connected to each other; which edge of each of them abuts which other edge) but they do it in two different ways, which could lead to havoc if one of them is altered but not the other
 * - xph 2011-06-02
 */
/* given two poles, returns an array of the directions they touch in, terminated with -1
 */
const signed int * mapPoleConnections (const char a, const char b);
/* given a starting pole and an x, y coordinate, returns the pole reached
 */
char mapPoleTraversal (const char p, signed int x, signed int y);


/* all these functions may return values with target subhexes of lower fidelity (so the span of the original subhex argument is < the span of the target subhex); this can only traverse across currently-loaded subhexes
 *
 * there's also a design flaw (of sorts) in all of these -- once generated, there's no attempt made to keep the generated origin/target pointers up to date, and as a /result/ they can become dangling pointers very easily. the distinct point is that it's not safe to keep referring to a relativehex's subhex values if there has been a subhex loading/unloading tick since it was generated, which means in practice IT IS A VERY BAD IDEA TO CACHE RELATIVEHEX RESULTS unless the code is intimately tied together with the loading code and can remove unloaded values without attempting to dereference them.
 */
RELATIVEHEX mapRelativeSubhexWithVectorOffset (const SUBHEX subhex, const VECTOR3 * offset);
RELATIVEHEX mapRelativeSubhexWithCoordinateOffset (const SUBHEX subhex, const signed char relativeSpan, const signed int x, const signed int y);

/* this does exactly what mapRelativeSubhexWithCoordinateOffset does only it returns the target or NULL if no perfect target and automatically destroys the relativehex */
SUBHEX mapHexAtCoordinateAuto (const SUBHEX subhex, const signed short relativeSpan, const signed int x, const signed int y);

SUBHEX mapRelativeTarget (const RELATIVEHEX relativePosition);
SUBHEX mapRelativeSpanTarget (const RELATIVEHEX relativePosition, unsigned char span);
VECTOR3 mapRelativeDistance (const RELATIVEHEX relPos);
/* do the coordinate values match up with the span of the target; i.e., has the best-match subhex actually been loaded */
bool isPerfectFidelity (const RELATIVEHEX relPos);
void mapRelativeDestroy (RELATIVEHEX rel);

VECTOR3 mapDistanceFromSubhexCentre (const unsigned char spanLevel, const signed int x, const signed int y);
bool mapScaleCoordinates (signed char relativeSpan, signed int x, signed int y, signed int * xp, signed int * yp, signed int * xRemainder, signed int * yRemainder);
/* bool mapBridge (const signed int x, const signed int y, signed int * xp, signed int * yp, signed char * dir); */
signed int * const mapSpanCentres (const unsigned char span);

/***
 * MAP DATA LAYER FUNCTIONS
 */

void mapDataSet (SUBHEX at, const char * type, signed int amount);
signed int mapDataAdd (SUBHEX at, const char * type, signed int amount);
signed int mapDataGet (SUBHEX at, const char * type);
const Dynarr mapDataTypes (SUBHEX at);

void subhexAddArch (SUBHEX at, Entity arch);
void subhexRemoveArch (SUBHEX at, Entity arch);
const Dynarr subhexGetArches (const SUBHEX at);

// REMOVE: this function is only going to be used for the fake re-imprinting; it's going to be replaced by something less terrible once map loading is more full-fledged
void subhexResetLoadStateForNewArch (SUBHEX at);

/***
 * COLLISION
 */

union collide_marker
{
	enum collide_hit
	{
		HIT_NOTHING,	// pointed out into space
		HIT_SURFACE,	// hit surface of hex
		HIT_UNDERSIDE,	// hit underside of hex
		HIT_JOIN,
		HIT_OTHER
	} type;
	struct
	{
		enum collide_hit
			type;
		HEX
			hex;
		HEXSTEP
			step;
	} hex;
	struct
	{
		enum collide_hit
			type;
		HEX
			hex;
		HEXSTEP
			step;
		unsigned int
			dir,
			index,
			heightHit;
	} join;
};

union collide_marker map_lineCollide (const SUBHEX base, const VECTOR3 * local, const VECTOR3 * ray);

/***
 * INFORMATIONAL / GETTER FUNCTIONS
 */

unsigned char subhexSpanLevel (const SUBHEX subhex);
char subhexPoleName (const SUBHEX subhex);
SUBHEX subhexData (const SUBHEX subhex, signed int x, signed int y);
SUBHEX subhexParent (const SUBHEX subhex);
bool subhexLocalCoordinates (const SUBHEX subhex, signed int * xp, signed int * yp);
bool subhexPartlyLoaded (const SUBHEX subhex);

// what was the idea of this...???
//bool subhexCoordinateOffset (const SUBHEX subhex, const SUBHEX offset, unsigned char * spanLevel, signed int * xp, signed int * yp);

VECTOR3 mapDistanceBetween (const SUBHEX a, const SUBHEX b);
VECTOR3 mapDistance (const hexPos a, const hexPos b);

/***
 * RENDERING FUNCTIONS
 */

void mapBakeEdgeHexes (SUBHEX subhex, unsigned int dir);
void mapBakeHexes (SUBHEX subhex);

void worldSetRenderCacheCentre (SUBHEX origin);
VECTOR3 renderOriginDistance (const SUBHEX hex);
void mapDraw (const float const * matrix);
void subhexDraw (const SUBDIV sub, const VECTOR3 offset);
void hexDraw (const HEX hex, const VECTOR3 centreOffset);

enum map_draw_types
{
	DRAW_NORMAL,
	DRAW_HIGHLIGHT,
};

void drawHexSurface (const struct hexColumn * const hex, const HEXSTEP step, const VECTOR3 * const render, enum map_draw_types style);
void drawHexUnderside (const struct hexColumn * const hex, const HEXSTEP step, MATSPEC material, const VECTOR3 * const render, enum map_draw_types style);
void drawHexEdge (const struct hexColumn * const hex, const HEXSTEP step, unsigned int high1, unsigned int high2, unsigned int low1, unsigned int low2, int direction, const VECTOR3 * const render, enum map_draw_types style);

#endif /* XPH_MAP_H */