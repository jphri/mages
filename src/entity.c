#include <SDL.h>
#include <stdbool.h>
#include "util.h"
#include "vecmath.h"
#include "entity.h"
#include "game_objects.h"
#include "global.h"

typedef struct {
	EntityType type;
	union {
		#define MAC_ENTITY(NAME) \
		NAME##_struct __##NAME##_struct;
			ENTITY_LIST
		#undef MAC_ENTITY
	} data;
} EntityObject;

static struct {
	void (*update)(EntityID id, float delta);
	void (*render)(EntityID id);
	void (*delete)(EntityID id);
} descr[LAST_ENTITY] =
{
	#define MAC_ENTITY(NAME) \
		[NAME] = { NAME##_update, NAME##_render, NAME##_del },
		ENTITY_LIST
	#undef MAC_ENTITY
};

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
		if(descr[ent_type(id)].update)
			descr[ent_type(id)].update(id, delta);
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
		if(descr[ent_type(id)].render)
			descr[ent_type(id)].render(id);
	}
}

EntityID
ent_new(EntityType type)
{
	EntityID new = objalloc_alloc(&objects);
	PRIVDATA(new)->type = type;
	return new;
}

void
ent_del(EntityID id)
{
	if(descr[ent_type(id)].delete)
		descr[ent_type(id)].delete(id);
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

void *
ent_component(EntityID id, EntityComponent comp)
{
	unsigned int int_type = ent_type(id) | comp << 8;

	#define DEFINE_COMPONENT_FOR(TYPE, COMPONENT, MEMBER_NAME) \
		case (TYPE | (COMPONENT << 8)): return &ENT_DATA(TYPE, id)->MEMBER_NAME; break

	switch(int_type) {
	DEFINE_COMPONENT_FOR(ENTITY_DUMMY, ENTITY_COMP_MOB, mob);
	DEFINE_COMPONENT_FOR(ENTITY_FIREBALL, ENTITY_COMP_DAMAGE, damage);

	DEFINE_COMPONENT_FOR(ENTITY_PLAYER, ENTITY_COMP_BODY, body);
	DEFINE_COMPONENT_FOR(ENTITY_DUMMY, ENTITY_COMP_BODY, body);
	DEFINE_COMPONENT_FOR(ENTITY_FIREBALL, ENTITY_COMP_BODY, body);

	DEFINE_COMPONENT_FOR(ENTITY_PARTICLE, ENTITY_COMP_BODY, body);
	default:
		return NULL;
	}
}

RelPtr
ent_relptr(void *ptr)
{
	return (RelPtr){ &objects.data_buffer.data, (unsigned char *)ptr - (unsigned char *)objects.data_buffer.data };
}
