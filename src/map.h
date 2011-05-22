#ifndef XPH_MAP_H
#define XPH_MAP_H

#include "bool.h"
#include "vector.h"
#include "dynarr.h"

typedef union hexOrSubdiv * SUBHEX;

typedef struct hexSubdivided * SUBDIV;
typedef struct hexTile * HEX;

typedef struct hexWorldPosition * WORLDHEX;

enum hexOrSubdivType
{
	HS_HEX = 1,
	HS_SUB,
};

enum mapPoleTypes
{
	POLE_ONE = 1,
	POLE_TRI = 3,		// <-- this is the only one that works
	POLE_QUAD = 4,
	POLE_HEX = 6,
};

bool mapSetSpanAndRadius (const unsigned char span, const unsigned char radius);
bool mapGeneratePoles (const enum mapPoleTypes type);

signed int * mapPoleConnections (const char a, const char b);

void mapForceSubdivide (SUBHEX subhex);
void mapForceGrowAtLevelForDistance (SUBHEX subhex, unsigned char absoluteSpan, unsigned int distance);

/***
 * SIMPLE CREATION AND INITIALIZATION FUNCTIONS
 */

SUBHEX mapTileCreate (const SUBHEX parent, signed int x, signed int y);
SUBHEX mapSubhexCreate (const SUBHEX parent, signed int x, signed int y);

/***
 * MAP TRAVERSAL FUNCTIONS
 */

SUBHEX mapGetRelativePosition (const SUBHEX subhex, const signed char relativeSpan, const signed int x, const signed int y);
SUBHEX mapPole (const char poleName);

Dynarr mapAdjacentSubhexes (const SUBHEX subhex, unsigned int distance);
/* returns the subhex reached by taking the centre of {subhex} and moving by the real-space {offset}. may return {subhex}, or possibly a subhex with a higher span level if the precise location isn't generated yet.
 *
 */
SUBHEX mapSubhexAtVectorOffset (const SUBHEX subhex, const VECTOR3 * offset);


VECTOR3 mapDistanceFromSubhexCenter (const unsigned char spanLevel, const signed int x, const signed int y);
bool mapScaleCoordinates (signed char relativeSpan, signed int x, signed int y, signed int * xp, signed int * yp);
/* bool mapBridge (const signed int x, const signed int y, signed int * xp, signed int * yp, signed char * dir); */
signed int * const mapSpanCentres (const unsigned char span);

/***
 * INFORMATIONAL / GETTER FUNCTIONS
 */

unsigned char subhexSpanLevel (const SUBHEX subhex);
char subhexPoleName (const SUBHEX subhex);
SUBHEX subhexData (const SUBHEX subhex, unsigned int offset);
SUBHEX subhexParent (const SUBHEX subhex);
bool subhexLocalCoordinates (const SUBHEX subhex, signed int * xp, signed int * yp);

/* calculates and returns the vector offset between the two subhexes, from
 * {subhex} to {offset}
 */
VECTOR3 subhexVectorOffset (const SUBHEX subhex, const SUBHEX offset);
/* calculates the coordinate offset between {subhex} and {offset}, and sets
 * *spanLevel / *xp / *yp to the result
 */
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