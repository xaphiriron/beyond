#include "worldgen.h"

PATTERNTEMPLATE WorldTemplate = NULL;
PATTERN WorldPattern = NULL;

struct worldgenFeature // FEATURE
{
	WORLDSHAPE
		* shape;
	WORLDEFFECT
		* effect;
};

struct worldgenPattern // PATTERN
{
	// ???
};

struct worldTile // WORLDTILE
{
	unsigned int
		pr, pi;
	signed int
		gx, gy;
	unsigned char
		bits;	// the pole bits and pk-val
};

struct groundList // GROUNDLIST
{
	int
		i;
	worldPosition
		** grounds;
};

union worldShapes // WORLDSHAPE
{
	enum worldShapeTypes type;
};

union worldEffects // WORLDEFFECT
{
	enum worldEffectTypes type;
};

void worldgenAbsHocNihilo ()
{
	WorldTemplate = patternTemplateFromSpecification ("\
");
	WorldPattern = patternFromTemplate (WorldTemplate);
	// instantiate pattern #1
	// register loading function
	system_registerTimedFunction (worldgenExpandWorldPatternGraph, 0xff);
}

void worldgenExpandWorldPatternGraph (TIMER t)
{
	DEBUG ("in %s", __FUNCTION__);
	while (!outOfTime (t))
	{
	// bredth-first traversal of pattern #1; when the first unexpanded pattern is found, call worldgenExpandPatternGraph (p, 1) on it; keep going until the depth threshhold is reached.
		break;
	}
	system_removeTimedFunction (worldgenExpandWorldPatternGraph);
	groundWorld_placePlayer ();
	system_setState (STATE_FIRSTPERSONVIEW);
}

void worldgenExpandPatternGraph (PATTERN p, unsigned int depth)
{

}


bool worldgenUnexpandedPatternsAt (const worldPosition wp)
{
	return FALSE;
}

Dynarr worldgenGetUnimprintedPatternsAt (const worldPosition wp)
{
	return dynarr_create (1, sizeof (char *));
}

void worldgenMarkPatternImprinted (const PATTERN p, const worldPosition wp)
{
}

void worldgenImprintGround (TIMER t, Component c)
{
	void
		* g = component_getData (c);
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
		// ...
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





PATTERN patternFromTemplate (const PATTERNTEMPLATE pt)
{
	return NULL;
}

PATTERNTEMPLATE patternTemplateFromSpecification (const char * spec)
{
	return NULL;
}