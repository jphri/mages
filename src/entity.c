#include <stdbool.h>
#include "util.h"
#include "vecmath.h"
#include "entity.h"
#include "game_objects.h"

typedef struct {
	EntityType type;
} EntityPrivData;

typedef struct {
	EntityType type;
	union {
		#define MAC_ENTITY(NAME) \
		NAME##_struct __##NAME##_struct;
			ENTITY_LIST
		#undef MAC_ENTITY
	} data;
} EntityNode;

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

#define PRIVDATA(ID) ((EntityPrivData*)obj_privdata(ID))

void
_sys_int_cleanup(GameObjectID id)
{
	if(descr[ent_type(id)].delete)
		descr[ent_type(id)].delete(id);
}

void
ent_init(void)
{
}

void
ent_end(void)
{
}

GameObjectRegistry
ent_object_descr(void)
{
	return (GameObjectRegistry) {
		.activated = true,
		.clean_cbk = _sys_int_cleanup,
		.privdata_size = sizeof(EntityPrivData),
		.data_size = sizeof(EntityNode)
	};
}

void
ent_reset(void)
{
	obj_reset(GAME_OBJECT_TYPE_ENTITY);
}

void
ent_update(float delta)
{
	for(EntityID id = obj_begin(GAME_OBJECT_TYPE_ENTITY);
		id;
		id = obj_next(id))
	{
		if(descr[ent_type(id)].update)
			descr[ent_type(id)].update(id, delta);
	}
}

void
ent_render(void)
{
	for(EntityID id = obj_begin(GAME_OBJECT_TYPE_ENTITY);
		id;
		id = obj_next(id))
	{
		if(descr[ent_type(id)].render)
			descr[ent_type(id)].render(id);
	}
}

EntityID
ent_new(EntityType type)
{
	EntityID new = obj_new(GAME_OBJECT_TYPE_ENTITY);
	PRIVDATA(new)->type = type;
	return new;
}

void
ent_del(EntityID id)
{
	obj_del(id);
}

void *
ent_data(EntityID id)
{
	return obj_data(id);
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
