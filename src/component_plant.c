#include "component_plant.h"

struct plant_data
{
	char
		seed[4];
	Dynarr
		buds;
	SHAPE
		flower,
		leaf;
	LINES3
		stems,
		roots;
	enum plant_lifecycle
	{
		PLANT_GROW,
		PLANT_BLOSSOM,
		PLANT_WILT,
		PLANT_FRUIT,
		PLANT_FALLOW,
	} stage;
};

struct plant_buds
{
	VECTOR3
		offsetFromCore,
		facing;
};

void plant_generateSeed (struct plant_data * pd)
{
	int
		i = 0;
	while (i < 4)
	{
		pd->seed[i] = rand () % 256;
		i++;
	}
}



void plant_generateRandom (Entity e)
{
	struct plant_data
		* pd = component_getData (entity_getAs (e, "plant"));
	if (pd == NULL)
	{
		component_instantiateOnEntity ("plant", e);
		pd = component_getData (entity_getAs (e, "plant"));
	}
	plant_generateSeed (pd);
	pd->flower = shape_makeCross ();
	shape_setColor (pd->flower, 31, 0, 63, 255);
}

void plant_draw (Entity e, CameraGroundLabel label)
{
	struct plant_data
		* pd = component_getData (entity_getAs (e, "plant"));
	VECTOR3
		offset = label_getOriginOffset (label),
		loc = position_getLocalOffset (e),
		total = vectorAdd (&offset, &loc);
	if (pd == NULL)
		return;

	glPushMatrix ();
	glTranslatef (total.x, total.y + 40, total.z);
	shape_draw (pd->flower);
	glPopMatrix ();
}

static Dynarr comp_entdata = NULL;
int component_plant (Object * obj, objMsg msg, void * a, void * b)
{
	Entity
		e;
	struct plant_data
		* pd;
	DynIterator
		it;
	struct comp_message
		* c_msg;
	switch (msg)
	{
		case OM_CLSNAME:
			strncpy (a, "plant", 32);
			return EXIT_SUCCESS;
		case OM_CLSINIT:
			comp_entdata = dynarr_create (16, sizeof (Entity));
			return EXIT_SUCCESS;
		case OM_CLSFREE:
			dynarr_destroy (comp_entdata);
			comp_entdata = NULL;
			return EXIT_SUCCESS;
		case OM_CLSVARS:
		case OM_CREATE:
			return EXIT_FAILURE;
		default:
			break;
	}
	switch (msg)
	{
		case OM_SHUTDOWN:
		case OM_DESTROY:
			obj_destroy (obj);
			return EXIT_SUCCESS;

		case OM_COMPONENT_INIT_DATA:
			e = (Entity)b;
			pd = xph_alloc (sizeof (struct plant_data));
			*(struct plant_data **)a = pd;
			dynarr_push (comp_entdata, e);
			return EXIT_SUCCESS;

		case OM_COMPONENT_DESTROY_DATA:
			e = (Entity)b;
			pd = *(struct plant_data **)a;
			shape_destroy (pd->flower);
			xph_free (pd);
			dynarr_remove_condense (comp_entdata, e);
			return EXIT_SUCCESS;

		case OM_UPDATE:
			it = dynIterator_create (comp_entdata);
			while (!dynIterator_done (it))
			{
				e = *(Entity *)dynIterator_next (it);
				pd = component_getData (entity_getAs (e, "plant"));
			}
			dynIterator_destroy (it);
			return EXIT_FAILURE;

		case OM_POSTUPDATE:
			it = dynIterator_create (comp_entdata);
			while (!dynIterator_done (it))
			{
				e = *(Entity *)dynIterator_next (it);
				pd = component_getData (entity_getAs (e, "plant"));
			}
			dynIterator_destroy (it);
			return EXIT_FAILURE;

		case OM_COMPONENT_RECEIVE_MESSAGE:
			c_msg = a;
			e = component_entityAttached (c_msg->to);
			if (strcmp (c_msg->message, "RENDER") == 0)
			{
				plant_draw (e, b);
			}
			return EXIT_FAILURE;
		case OM_SYSTEM_RECEIVE_MESSAGE:
			return EXIT_FAILURE;

		default:
			return obj_pass ();
	}
	return 0;
}