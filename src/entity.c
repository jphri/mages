#include <SDL.h>
#include <stdbool.h>
#include <assert.h>

#include "util.h"
#include "vecmath.h"
#include "entity.h"
#include "game_objects.h"
#include "global.h"

typedef struct {
	EntityType type;
	EntityInterface *interface;
	Entity data;
} EntityObject;

static ObjectPool objects;

void
ent_init(void)
{
	objpool_init(&objects, sizeof(EntityObject), DEFAULT_ALIGNMENT);
}

void
ent_end(void)
{
	objpool_terminate(&objects);
}

void
ent_reset(void)
{
	objpool_reset(&objects);
}

void
ent_update(float delta)
{
	for(EntityObject * obj = objpool_begin(&objects);
		obj;
		obj = objpool_next(obj))
	{
		if(obj->interface->update)
			obj->interface->update(&obj->data, delta);
	}
	objpool_clean(&objects);
}

void
ent_render(void)
{
	for(EntityObject *object = objpool_begin(&objects);
		object;
		object = objpool_next(object))
	{
		if(object->interface->render)
			object->interface->render(&object->data);
	}
}

Entity *
ent_new(EntityType type, EntityInterface *interface)
{
	assert(interface != NULL);

	EntityObject *object = objpool_new(&objects);
	object->type = type;
	object->interface = interface;
	return &object->data;
}

void
ent_del(Entity *e)
{
	EntityObject *obj = CONTAINER_OF(e, EntityObject, data);
	if(obj->interface->die)
		obj->interface->die(e);
	objpool_free(obj);
}

EntityType
ent_type(Entity *e)
{
	return CONTAINER_OF(e, EntityObject, data)->type;
}

bool
ent_implements(Entity *e, ptrdiff_t offset)
{
	EntityObject *obj = CONTAINER_OF(e, EntityObject, data);

	/* beautiful, isn't it? */
	void *fptr = *(void**)((uintptr_t)obj->interface + offset);
	return fptr != NULL;
}

void 
ent_take_damage(Entity *e, float damage, vec2 damage_indicator_pos)
{
	EntityObject *obj = CONTAINER_OF(e, EntityObject, data);

	if(obj->interface->take_damage) {
		obj->interface->take_damage(e, damage);
		ent_damage_number(damage_indicator_pos, damage);
	}
}

Entity *
ent_hover(vec2 mouse_pos)
{
	for(EntityObject *obj = objpool_begin(&objects);
		obj;
		obj = objpool_next(obj))
	{
		if(!obj->interface->mouse_hovered)
			continue;
		
		if(obj->interface->mouse_hovered(&obj->data, mouse_pos))
			return &obj->data;
	}

	return NULL;
}
