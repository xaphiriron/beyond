#ifndef XPH_MAP_H
#define XPH_MAP_H

#include "bool.h"
#include "vector.h"
#include "dynarr.h"

typedef union hexOrSubdiv * SUBHEX;

typedef struct hexSubdivided * SUBDIV;
typedef struct hexTile * HEX;

typedef struct hexWorldPosition * WORLDHEX;
typedef struct hexRelativePosition * RELATIVEHEX;

enum hexOrSubdivType
{
	HS_HEX = 1,
	HS_SUB,
};

enum mapPoleTypes
{
// 	POLE_ONE = 1,
	POLE_TRI = 3,		// <-- this is the only one that works
// 	POLE_QUAD = 4,
// 	POLE_HEX = 6,
};

bool mapSetSpanAndRadius (const unsigned char span, const unsigned char radius);
bool mapGeneratePoles (const enum mapPoleTypes type);
bool worldExists ();

void worldDestroy ();

void mapForceSubdivide (SUBHEX subhex);
void mapForceGrowAtLevelForDistance (SUBHEX subhex, unsigned char absoluteSpan, unsigned int distance);

/***
 * SIMPLE CREATION AND INITIALIZATION FUNCTIONS
 */

SUBHEX mapHexCreate (SUBHEX parent, signed int x, signed int y);
SUBHEX mapSubdivCreate (SUBHEX parent, signed int x, signed int y);

void subhexDestroy (SUBHEX subhex);

/***
 * MAP TRAVERSAL FUNCTIONS
 */

SUBHEX mapPole (const char poleName);

/* FIXME: the two functions below both calculate pole-level connections (i.e., how the poles are connected to each other; which edge of each of them abuts which other edge) but they do it in two different ways, which could lead to havoc if one of them is altered but not the other
 * - xph 2011-06-02
 */
/* given two poles, returns an array of the directions they touch in, terminated with -1
 */
signed int * mapPoleConnections (const char a, const char b);
/* given a starting pole and an x, y coordinate, returns the pole reached
 */
char mapPoleTraversal (const char p, signed int x, signed int y);

/* returns a dynarr with hx (distance) entries, each of which is a RELATIVEHEX denoting a subhex position radiating out from subhex
 */
Dynarr mapAdjacentSubhexes (const SUBHEX subhex, unsigned int distance);

/* all these functions may return values with target subhexes of lower fidelity (so the span of the original subhex argument is < the span of the target subhex); this can only traverse across currently-loaded subhexes
 *
 * there's also a design flaw (of sorts) in all of these -- once generated, there's no attempt made to keep the generated origin/target pointers up to date, and as a /result/ they can become dangling pointers very easily. the distinct point is that it's not safe to keep referring to a relativehex's subhex values if there has been a subhex loading/unloading tick since it was generated, which means in practice IT IS A VERY BAD IDEA TO CACHE RELATIVEHEX RESULTS unless the code is intimately tied together with the loading code and can remove unloaded values without attempting to dereference them.
 */
RELATIVEHEX mapRelativeSubhexWithVectorOffset (const SUBHEX subhex, const VECTOR3 * offset);
RELATIVEHEX mapRelativeSubhexWithCoordinateOffset (const SUBHEX subhex, const signed char relativeSpan, const signed int x, const signed int y);
RELATIVEHEX mapRelativeSubhexWithSubhex (const SUBHEX subhex, const SUBHEX target);

SUBHEX mapRelativeTarget (const RELATIVEHEX relativePosition);
VECTOR3 mapRelativeDistance (const RELATIVEHEX relPos);
/* do the coordinate values match up with the span of the target; i.e., has the best-match subhex actually been loaded */
bool isPerfectFidelity (const RELATIVEHEX relPos);
void mapRelativeDestroy (RELATIVEHEX rel);


VECTOR3 mapDistanceFromSubhexCentre (const unsigned char spanLevel, const signed int x, const signed int y);
bool mapVectorOverrunsPlatter (const unsigned char span, const VECTOR3 * vector);
bool mapScaleCoordinates (signed char relativeSpan, signed int x, signed int y, signed int * xp, signed int * yp, signed int * xRemainder, signed int * yRemainder);
/* bool mapBridge (const signed int x, const signed int y, signed int * xp, signed int * yp, signed char * dir); */
signed int * const mapSpanCentres (const unsigned char span);

/***
 * INFORMATIONAL / GETTER FUNCTIONS
 */

unsigned char subhexSpanLevel (const SUBHEX subhex);
char subhexPoleName (const SUBHEX subhex);
SUBHEX subhexData (const SUBHEX subhex, signed int x, signed int y);
SUBHEX subhexParent (const SUBHEX subhex);
bool subhexLocalCoordinates (const SUBHEX subhex, signed int * xp, signed int * yp);

bool subhexCoordinateOffset (const SUBHEX subhex, const SUBHEX offset, unsigned char * spanLevel, signed int * xp, signed int * yp);

WORLDHEX subhexGeneratePosition (const SUBHEX subhex);

WORLDHEX worldhexDuplicate (const WORLDHEX whx);
const char * const worldhexPrint (const WORLDHEX whx);
void worldhexDestroy (WORLDHEX whx);

SUBHEX worldhexSubhex (const WORLDHEX whx);
char worldhexPole (const WORLDHEX whx);

/***
 * RENDERING FUNCTIONS
 */

void worldSetRenderCacheCentre (SUBHEX origin);
void mapDraw ();
void subhexDraw (const SUBDIV sub, const VECTOR3 offset);
void hexDraw (const HEX hex, const VECTOR3 centreOffset);

#endif /* XPH_MAP_H */