#include "comp_plant.h"

#include "component_position.h"

void plant_create (EntComponent comp, EntSpeech speech);
void plant_destroy (EntComponent comp, EntSpeech speech);

void plant_define (EntComponent comp, EntSpeech speech)
{
	component_registerResponse ("plant", "__create", plant_create);
	component_registerResponse ("plant", "__destroy", plant_destroy);


}

void plant_create (EntComponent comp, EntSpeech speech)
{
	Plant
		plant = xph_alloc (sizeof (struct xph_plant));

	plant->expansion = lsystem_create ();
	lsystem_addProduction (plant->expansion, 'R', "R[-R][-'R][-''R]");

	plant->body = xph_alloc (2);
	plant->body[0] = 'R';

	plant->updateFrequency = 512;
	plant->lastUpdated = rand () & ((1 << 8) - 1);

	plant->fauxShape = shape_makeHollowPoly (7, 32);
	shape_setColor (plant->fauxShape, 0x99, 0x00, 0xff, 0xff);

	component_setData (comp, plant);
}

void plant_destroy (EntComponent comp, EntSpeech speech)
{
	Plant
		plant = component_getData (comp);

	shape_destroy (plant->fauxShape);

	lsystem_destroy (plant->expansion);

	xph_free (plant->body);
	xph_free (plant);

	component_clearData (comp);
}

void plantUpdate_system (Dynarr entities)
{
	Entity
		plantEntity;
	Plant
		plant;
	int
		i = 0;
	char
		* newBody;
	while ((plantEntity = *(Entity *)dynarr_at (entities, i++)))
	{
		plant = component_getData (entity_getAs (plantEntity, "plant"));
		if (++plant->lastUpdated >= plant->updateFrequency)
		{
			newBody = lsystem_iterate (plant->body, plant->expansion, 1);
			xph_free (plant->body);
			plant->body = newBody;
			plant->lastUpdated = 0;
		}
	}
}

void plantRender_system (Dynarr entities)
{
	Entity
		plantEntity;
	Plant
		plant;
	int
		i = 0;

	VECTOR3
		render;

	while ((plantEntity = *(Entity *)dynarr_at (entities, i++)))
	{
		plant = component_getData (entity_getAs (plantEntity, "plant"));
		render = position_renderCoords (plantEntity);
		//printf ("rendering #%d as a plant (at %.2f, %.2f, %.2f)\n", entity_GUID (plantEntity), render.x, render.y, render.z);
		glBindTexture (GL_TEXTURE_2D, 0);
		shape_draw (plant->fauxShape, &render);
	}
}