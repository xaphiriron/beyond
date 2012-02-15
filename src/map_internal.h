/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#ifndef XPH_MAP_INTERNAL_H
#define XPH_MAP_INTERNAL_H

#include "matspec.h"

struct hexSubdivided
{
	enum hexOrSubdivType
		type;
	unsigned char
		span;
	SUBHEX
		parent;	// the subdiv this subdiv is a part of (with span + 1), or NULL if this subdiv is a pole
	signed int
		x, y;	// the x,y offset of this subdiv within its parent, or 0,0 if this subdiv is a pole
	/* data is hex iff span == 1; data is subs iff span > 1; in either case
	 * the value may be NULL if the world hasn't been generated down to that
	 * level yet. furthermore if it's subs it's possible for only some of the
	 * subdivs to be non-NULL.
	 * In either case, if data isn't NULL to begin with it stores hx (mapRadius + 1) values.
     */
	SUBHEX
		* data;

	struct mapData
		* mapInfo;
	Dynarr
		arches;
	bool
		imprintable,	// true if all adjacent subdivs are loaded (so data val
						// interpolation works)
		loaded;			// true if all hexes have had all applicable patterns
						// imprinted (and the patterns haven't changed since)
};

struct hex_render_info
{
	int
		dummy;
};

struct hex_occupant
{
	Entity
		occupant;
	struct hexStep
		* over;
};

struct hexColumn
{
	enum hexOrSubdivType
		type;
	SUBHEX
		parent;
	signed int
		x, y;
	unsigned char
		light;

	Dynarr
		steps,
		occupants;
	struct hexColumn
		* adjacent[6];

	struct hex_render_info
		info;
};

struct step_render_info
{
	const VECTOR3
		* jit[6];
	bool
		undersideVisible,
		surfaceVisible;
	Dynarr
		visibleJoin[6];
};

struct hexStep
{
	unsigned int
		height;
	unsigned char
		corners[3];

	MATSPEC
		material;

	struct step_render_info
		info;
	/* actual hex data goes here */
};


struct mapData
{
	Dynarr
		entries;
};

struct mapDataEntry
{
	char
		type[32];
	signed int
		value;
};

union hexOrSubdiv		// SUBHEX is union hexOrSubdiv *
{
	enum hexOrSubdivType
		type;
	struct hexSubdivided
		sub;
	struct hexColumn
		hex;
};

struct hexRelativePosition // RELATIVEHEX
{
	/* both these values have (maxSpan - minSpan) + 1 indices allocated
	 */
	signed int
		* x,
		* y;
	unsigned char
		minSpan,
		maxSpan;
	VECTOR3
		distance;	// TODO: blah blah but consider the overflow
	/* TODO: i suspect either of these subhexes could easily become dangling pointers if a hexRelativePosition struct is kept for more than one tick, since they could be unloaded any time the subhex unloading code is called (this creates the question of why these values would be calculated to begin with, so... think about that for a while)
	 * - xph 2011-05-27
	 */
	SUBHEX
		origin,		// i.e., from origin x,y steps away is the subhex target
		target;
};

struct xph_world_hex_position
{
	signed int
		* x,
		* y;
	// platter[MapSpan] will always be loaded; it's a pole
	SUBHEX
		* platter;
	// focus is the last level the x,y values should be considered meaningful (i.e., 0,0 values past that point (at an index smaller than) aren't specific references to the centre of the platter above)
	unsigned char
		focus;
	VECTOR3
		from;
};

extern SUBHEX
	* Poles;
extern char
	* PoleNames;
extern int
	PoleCount;
extern unsigned char
	MapRadius,
	MapSpan;


int __v, __n;
#define GETCORNER(p, n)		(__n = (n), __v = (__n % 2 ? p[__n/2] & 0x0f : (p[__n/2] & 0xf0) >> 4), (__v & 8) ? 0 - ((__v & 7)) : __v)
#define SETCORNER(p, n, v)	(__n = (n), __v = (v), __v = (__v < 0 ? ((~__v & 15) + 1) | 8 : __v & 7), p[__n/2] = (__n % 2 ? (p[__n/2] & 0xf0) | __v : (p[__n/2] & 0x0f) | (__v << 4)))

#define FULLHEIGHT(base, i)		(__v = GETCORNER (base->corners, i), (__v < 0 && base->height < abs (__v)) ? 0 : ((__v > 0 && base->height > (UINT_MAX - __v)) ? UINT_MAX : base->height + __v))

/* keep these cast as float no matter what they are; since hex heights are
 * unsigned ints we really don't want to force a signed int * unsigned int
 * multiplication when we render the hexes (or whatever); just accept that
 * we're going to lose precision when we do it. so don't do it until you
 * render.
 *  -xph 2011-04-03
 */
#define HEX_SIZE	(float)30
#define HEX_SIZE_4	(float)7.5

#endif /* XPH_MAP_INTERNAL_H */