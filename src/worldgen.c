#include "worldgen.h"

#include "bit.h"
#include "component_ground.h"
#include "worldgen_graph.h"
#include "system.h"

#define IMPRINT_BIT 0x01

TEMPLATE WorldTemplate = NULL;
PATTERN WorldPattern = NULL;
static Dynarr
	AllPatterns = NULL;

/***
 * WORLD SHAPES
 */

struct wsHex
{
	enum worldShapeTypes
		type;
	unsigned int
		radius;
};

struct wsHexchunk
{
	enum worldShapeTypes
		type;
	unsigned int
		radius,			// radius of initial hex
		subSteps;		// number of times to generate sub(sub-[sub- etc])hexes
	unsigned char
		subHexes,		// number of subhexes to generate from each hex per step (0 - 12)
		subShrink;		// percentage (out of 256) to shrink radius with each step
};

union worldShapes // WORLDSHAPE
{
	enum worldShapeTypes
		type;
	struct wsHex
		hex;
	struct wsHexchunk
		hexchunk;
};

/***
 * REGION STRUCTS
 */

enum worldRegionTypes
{
	WGR_HEXCHUNK,
};

struct regionHex
{
	enum worldRegionTypes
		type;
	unsigned int
		radius;
	WORLDHEX
		centre;
	struct regionHex
		* subHex[12];
	unsigned char
		subCount;
};

union worldRegion	// WORLDREGION
{
	enum worldRegionTypes
		type;
	struct regionHex
		hex;
};

/***
 * WORLD EFFECTS
 */

struct weElevation
{
	enum worldEffectTypes
		type;
	signed int
		amount;
};

union worldEffects // WORLDEFFECT
{
	enum worldEffectTypes
		type;
	struct weElevation
		elevation;
};

/***
 * STRUCTURES USED EXTERNALLY
 */

struct worldgenTemplate // TEMPLATE
{
	WORLDSHAPE
		shape;
	Dynarr
		features;
};

struct worldgenFeature // FEATURE
{
	char
		* region;
	WORLDEFFECT
		effect;
};

struct worldgenPattern // PATTERN
{
	TEMPLATE
		template;
	WORLDREGION
		region;
	WORLDHEX
		locus;
	GROUNDLIST
		footprint;
	unsigned char
		* footprintBits;
	bool
		expanded;
	// ???
};

struct groundList // GROUNDLIST
{
	int
		i;
	worldPosition
		* grounds;
};


void worldgenAbsHocNihilo ()
{
/*
	// instantiate pattern #1
	WorldTemplate = templateFromSpecification ("\
");
*/
	WORLDHEX
		whx = worldhex ('r', 65535, 0, 0, 8, 0, 4);
	WorldTemplate = templateCreate ();
	templateSetShape (WorldTemplate, worldgenShapeCreate (WGS_HEXCHUNK, 6, 2, 3, 0x7f));
	templateAddFeature (WorldTemplate, "inside", worldgenEffectCreate (WGE_ELEVATION, 8));
	WorldPattern = templateInstantiateNear (WorldTemplate, whx);
	AllPatterns = dynarr_create (8, sizeof (PATTERN));
	dynarr_push (AllPatterns, WorldPattern);

	// register loading function
	system_registerTimedFunction (worldgenExpandWorldPatternGraph, 0xff);
}

void worldgenExpandWorldPatternGraph (TIMER t)
{
	static int
		i = 0;
	PATTERN
		p;
	DEBUG ("in %s", __FUNCTION__);
	
	while (!outOfTime (t))
	{
		p = *(PATTERN *)dynarr_at (AllPatterns, i);
		if (p == NULL)
			break;
		if (p->expanded == FALSE)
			worldgenExpandPatternGraph (p, 1);
		i++;
	}
	system_removeTimedFunction (worldgenExpandWorldPatternGraph);

	groundWorld_placePlayer ();
	systemClearStates();
	systemPushState (STATE_FIRSTPERSONVIEW);
}

void worldgenExpandPatternGraph (PATTERN p, unsigned int depth)
{

	p->expanded = TRUE;
}


bool worldgenUnexpandedPatternsAt (const worldPosition wp)
{
	// TODO: ... go through every pattern and check its footprint? OCTREE TIME. hextree. w/e
	PATTERN
		p;
	int
		i = 0,
		j = 0;
	while ((p = *(PATTERN *)dynarr_at (AllPatterns, i++)) != NULL)
	{
		j = 0;
		while (j < p->footprint->i)
		{
			if (wp_compare (p->footprint->grounds[j], wp) == 0)
			{
				if (p->expanded == FALSE)
					return TRUE;
			}
			j++;
		}
	}
	return FALSE;
}

Dynarr worldgenGetUnimprintedPatternsAt (const worldPosition wp)
{
	// see above re: hextree
	PATTERN
		p;
	Dynarr
		r = dynarr_create (1, sizeof (PATTERN));
	int
		i = 0,
		j = 0;
	while ((p = *(PATTERN *)dynarr_at (AllPatterns, i++)) != NULL)
	{
		j = 0;
		//printf ("blah blah checking %s...\n", wp_print (wp));
		while (j < p->footprint->i)
		{
			//printf ("footprint: %s\n", wp_print (p->footprint->grounds[j]));
			if (wp_compare (p->footprint->grounds[j], wp) == 0 && !(p->footprintBits[j] & IMPRINT_BIT) )
			{
				DEBUG ("%s is within pattern %p's footprint (%d)", wp_print (wp), p, j);
				dynarr_push (r, p);
			}
			j++;
		}
	}
	return r;
}

void worldgenMarkPatternImprinted (PATTERN p, const worldPosition wp)
{
	int
		i = 0;
	while (i < p->footprint->i)
	{
		if (wp_compare (p->footprint->grounds[i], wp) == 0)
		{
			p->footprintBits[i] |= IMPRINT_BIT;
			return;
		}
		i++;
	}
	ERROR ("Can't mark pattern (%p) imprinted: %s isn't in its footprint", p, wp_print (wp));
}

void worldgenMarkAllPatternsUnimprintedAt (const worldPosition wp)
{
	PATTERN
		p;
	int
		i = 0, j;
	while ((p = *(PATTERN *)dynarr_at (AllPatterns, i++)))
	{
		j = 0;
		while (j < p->footprint->i)
		{
			if (wp_compare (p->footprint->grounds[j], wp) == 0)
			{
				p->footprintBits[j] &= ~IMPRINT_BIT;
				break;
			}
			j++;
		}
	}
}

void worldgenImprintGround (TIMER t, EntComponent c)
{
	void
		* g = component_getData (c);
	struct affectedHexes
		* toChange;
	HEX
		hex;
	Entity
		x = component_entityAttached (c);
	worldPosition
		wp = ground_getWorldPos (g);
	Dynarr
		patterns;
	DynIterator
		it;
	PATTERN
		p;
	FEATURE
		feature;
	int
		i, j;
	if (worldgenUnexpandedPatternsAt (wp))
	{
		INFO ("Can't imprint the ground at %s yet (#%d); there are patterns still unexpanded", wp_print (wp), entity_GUID (x));
		return;
	}
	patterns = worldgenGetUnimprintedPatternsAt (wp);
	it = dynIterator_create (patterns);
	while (!dynIterator_done (it))
	{
		p = *(PATTERN *)dynIterator_next (it);

		DEBUG ("IMPRINTING PATTERN %p ON GROUND AT %s", p, wp_print (wp));
		// ...
		toChange = worldgenAffectedHexes (p, wp);
		i = 0;
		while (i < toChange->count)
		{
			hex = ground_getHexAtOffset (g, hex_linearCoord (toChange->r[i], toChange->k[i], toChange->i[i]));
			j = 0;
			while ((feature = *(FEATURE *)dynarr_at (p->template->features, j++)))
			{
				switch (feature->effect->type)
				{
					case WGE_ELEVATION:
						hex->centre = hex->centre + feature->effect->elevation.amount;
					default:
						break;
				}
				j++;
			}
			i++;
		}
		xph_free (toChange->r);
		xph_free (toChange->k);
		xph_free (toChange->i);
		xph_free (toChange);
		toChange = NULL;


		worldgenMarkPatternImprinted (p, wp);
		if (outOfTime (t))
		{
			dynIterator_destroy (it);
			return;
		}
	}
	dynIterator_destroy (it);
}

bool worldgenIsGroundFullyLoaded (const worldPosition wp)
{
	return TRUE;
}

/***
 * TEMPLATE FUNCTIONS
 */

TEMPLATE templateCreate ()
{
	TEMPLATE
		pt = xph_alloc (sizeof (struct worldgenTemplate));
	pt->shape = NULL;
	pt->features = dynarr_create (2, sizeof (WORLDSHAPE));
	return pt;
}

TEMPLATE templateFromSpecification (const char * spec)
{
	TEMPLATE
		pt = templateCreate ();

	return pt;
}

void templateSetShape (TEMPLATE template, WORLDSHAPE shape)
{
	assert (template != NULL);
	template->shape = shape;
}

void templateAddFeature (TEMPLATE template, const char * region, WORLDEFFECT effect)
{
	FEATURE
		f = xph_alloc (sizeof (struct worldgenFeature));
	f->region = xph_alloc (strlen (region) + 1);
	strcpy (f->region, region);
	f->effect = effect;
	dynarr_push (template->features, f);
}


PATTERN templateInstantiateNear (const TEMPLATE template, const WORLDHEX whx)
{
	PATTERN
		p = xph_alloc (sizeof (struct worldgenPattern));
	p->template = template;
	p->locus = whx;
	p->region = worldgenGenerateRegion (p->template->shape, whx);
	p->footprint = worldgenCalculateRegionFootprint (p->region);
	p->footprintBits = xph_alloc (p->footprint->i);
	memset (p->footprintBits, '\0', p->footprint->i);
	p->expanded = FALSE;
	return p;
}

WORLDREGION worldgenGenerateRegion (const WORLDSHAPE shape, const WORLDHEX whx)
{
	WORLDREGION
		r = xph_alloc (sizeof (union worldRegion));
	switch (shape->type)
	{
		case WGS_HEX:
			r->type = WGR_HEXCHUNK;
			r->hex.radius = shape->hex.radius;
			r->hex.centre = whx;
			r->hex.subCount = 0;
			memset (r->hex.subHex, '\0', sizeof (struct regionHex *) * 12);
			break;
		case WGS_HEXCHUNK:
			r->type = WGR_HEXCHUNK;
			r->hex.radius = shape->hex.radius;
			r->hex.centre = whx;
			
			// change this to actual subhex generation:
			r->hex.subCount = 0;
			memset (r->hex.subHex, '\0', sizeof (struct regionHex *) * 12);
			break;
		default:
			ERROR ("Unable to create world region: unknown shape type (%d)", shape->type);
			xph_free (r);
			return NULL;
	}
	return r;
}

GROUNDLIST worldgenCalculateRegionFootprint (const WORLDREGION region)
{
	GROUNDLIST
		gl = xph_alloc (sizeof (struct groundList));
	int
		scale;
	worldPosition
		pos,
		* footprint;
	assert (region != NULL);
	gl->i = 0;
	gl->grounds = NULL;
	switch (region->type)
	{
		case WGR_HEXCHUNK:
			pos = worldhexAsWorldPosition (region->hex.centre);
			scale = region->hex.radius / groundWorld_getGroundRadius () + 2;
			gl->i = hx (scale);
			footprint = wp_adjacentSweep (pos, groundWorld_getPoleRadius (), scale);
			gl->grounds = xph_alloc (sizeof (worldPosition) * gl->i);
			memcpy (gl->grounds, footprint, sizeof (worldPosition) * (gl->i - 1));
			gl->grounds[gl->i - 1] = pos;
			xph_free (footprint);
			footprint = NULL;

//*
			scale = 0;
			while (scale < gl->i)
			{
				printf ("%s (%d/%d)\n", wp_print (gl->grounds[scale]), scale + 1, gl->i);
				scale++;
			}
//*/
			break;
		default:
			break;
	}
	return gl;
}


WORLDSHAPE worldgenShapeCreate (enum worldShapeTypes shape, ...)
{
	WORLDSHAPE
		s = xph_alloc (sizeof (union worldShapes));
	va_list
		args;
	va_start (args, shape);
	s->type = shape;
	switch (shape)
	{
		case WGS_HEX:
			s->hex.radius = va_arg (args, unsigned int);
			break;
		case WGS_HEXCHUNK:
			s->hexchunk.radius = va_arg (args, unsigned int);
			s->hexchunk.subSteps = va_arg (args, unsigned int);
			s->hexchunk.subHexes = (unsigned char)va_arg (args, int);
			s->hexchunk.subShrink = (unsigned char)va_arg (args, int);
			break;
		default:
			ERROR ("Unable to create world shape: unknown type (%d)", shape);
			xph_free (s);
			va_end (args);
			return NULL;
	}
	va_end (args);
	return s;
}

WORLDEFFECT worldgenEffectCreate (enum worldEffectTypes effect, ...)
{
	WORLDEFFECT
		e = xph_alloc (sizeof (union worldEffects));
	va_list
		args;
	va_start (args, effect);
	e->type = effect;
	switch (effect)
	{
		case WGE_ELEVATION:
			e->elevation.amount = va_arg (args, signed int);
			break;
		default:
			ERROR ("Unable to create world effect: unknown type (%d)", effect);
			xph_free (e);
			va_end (args);
			return NULL;
	}
	va_end (args);
	return e;
}




struct affectedHexes * worldgenAffectedHexes (const PATTERN p, const worldPosition wp)
{
	struct affectedHexes
		* h = xph_alloc (sizeof (struct affectedHexes));
	unsigned int
		r = 0,
		k = 0,
		i = 0;
	signed int
		wpx, wpy,
		dx[3], dy[3],
		x, y,
		patternX, patternY;
	int
		radius = groundWorld_getGroundRadius (),
		distance;
	worldPosition
		locus = worldhexAsWorldPosition (p->locus);
	h->count = hx (radius + 1);
	h->r = xph_alloc (sizeof (unsigned int) * h->count);
	h->k = xph_alloc (sizeof (unsigned int) * h->count);
	h->i = xph_alloc (sizeof (unsigned int) * h->count);
	h->count = 0;
	
	/* i don't understand what this code does or how it works and i just wrote
	 * the fucking thing.
	 *  - xph 2011-04-07
	 */
	wp_pos2xy (wp, locus, &wpx, &wpy);
	//DEBUG ("here: %s", wp_print (wp));
	//DEBUG ("locus: %s", wp_print (locus));
	//DEBUG ("Initial worldPosition distance calculation, in ground coordinates: %d, %d", wpx, wpy);
	hexGround_centerDistanceCoord (radius, 0, &dx[0], &dy[0]);
	hexGround_centerDistanceCoord (radius, 1, &dx[1], &dy[1]);
	dx[2] = dx[0] * wpx + dx[1] * wpy;
	dy[2] = dy[0] * wpx + dy[1] * wpy;
	wpx = dx[2];
	wpy = dy[2];
	//DEBUG ("Final distance calculation, in hex coordinates: %d, %d", wpx, wpy);
	while (r <= radius)
	{
		hex_rki2xy (r, k, i, &x, &y);
		switch (p->region->type)
		{
			case WGR_HEXCHUNK:
				worldhexGroundOffset (p->locus, &patternX, &patternY);
				if ((distance = hex_distanceBetween (wpx + x, wpy + y, patternX, patternY)) <= p->region->hex.radius)
				{
					//DEBUG ("TILE AT EFFECTIVE COORDS %d + %d, %d + %d IS WITHIN PATTERN HEX CENTERED AT %d, %d (%d tile%s distant); %d so far in this ground (of %d total)", wpx, x, wpy, y, p->locus->gx, p->locus->gy, distance, distance == 1 ? "" : "s", h->count, hx (radius + 1));
					h->r[h->count] = r;
					h->k[h->count] = k;
					h->i[h->count] = i;
					h->count++;
				}
				break;
			default:
				break;
		}
		hex_nextValidCoord (&r, &k, &i);
	}
	return h;
}
