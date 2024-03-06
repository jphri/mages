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
	union {
		#define MAC_ENTITY(NAME) \
		NAME##_struct __##NAME##_struct;
			ENTITY_LIST
		#undef MAC_ENTITY
	} data;
} EntityObject;

#define PRIVDATA(ID) ((EntityObject*)objalloc_data(&objects, ID))
static ObjectAllocator objects;

void
_sys_int_cleanup(GameObjectID id)
{
	(void)id;
}

void
ent_init(void)
{
	objalloc_init_allocator(&objects, sizeof(EntityObject), cache_aligned_allocator());
}

void
ent_end(void)
{
	objalloc_end(&objects);
}

void
ent_reset(void)
{
	objalloc_reset(&objects);
}

void
ent_update(float delta)
{
	for(EntityID id = objalloc_begin(&objects);
		id;
		id = objalloc_next(&objects, id))
	{
		if(PRIVDATA(id)->interface->update)
			PRIVDATA(id)->interface->update(id, delta);
	}
	objalloc_clean(&objects);
}

void
ent_render(void)
{
	for(EntityID id = objalloc_begin(&objects);
		id;
		id = objalloc_next(&objects, id))
	{
		if(PRIVDATA(id)->interface->render)
			PRIVDATA(id)->interface->render(id);
	}
}

EntityID
ent_new(EntityType type, EntityInterface *interface)
{
	assert(interface != NULL);

	EntityID new = objalloc_alloc(&objects);
	PRIVDATA(new)->type = type;
	PRIVDATA(new)->interface = interface;
	return new;
}

void
ent_del(EntityID id)
{
	if(PRIVDATA(id)->interface->die)
		PRIVDATA(id)->interface->die(id);
	objalloc_free(&objects, id);
}

void *
ent_data(EntityID id)
{
	return &PRIVDATA(id)->data;
}

EntityType
ent_type(EntityID id)
{
	return PRIVDATA(id)->type;
}

bool
ent_implements(EntityID id, ptrdiff_t offset)
{
	/* beautiful, isn't it? */
	void *fptr = *(void**)((uintptr_t)PRIVDATA(id)->interface + offset);
	return fptr != NULL;
}

void 
ent_take_damage(EntityID id, float damage, vec2 damage_indicator_pos)
{
	if(PRIVDATA(id)->interface->take_damage) {
		PRIVDATA(id)->interface->take_damage(id, damage);
		ent_damage_number(damage_indicator_pos, damage);
	}
}

RelPtr
ent_relptr(void *ptr)
{
	return (RelPtr){ &objects.data_buffer.data, (unsigned char *)ptr - (unsigned char *)objects.data_buffer.data };
}
