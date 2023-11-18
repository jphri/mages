#include <stdbool.h>
#include "util.h"
#include "vecmath.h"
#include "entity.h"

typedef struct {
	EntityID next, prev;
	EntityType type;
	bool dead;
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

#define SYS_ID_TYPE EntityID
#define SYS_NODE_TYPE EntityNode
#include "system.h"

void
_sys_int_cleanup(EntityID id) 
{
	if(descr[ent_type(id)].delete)
		descr[ent_type(id)].delete(id);
}

void
ent_init()
{
	_sys_init();
}

void
ent_end()
{
	_sys_deinit();
}

void
ent_update(float delta) 
{
	for(EntityID id = _sys_list;
		id;
		id = _sys_node(id)->next)
	{
		if(descr[ent_type(id)].update)
			descr[ent_type(id)].update(id, delta);
	}
	_sys_cleanup();
}

void
ent_render()
{
	for(EntityID id = _sys_list;
		id;
		id = _sys_node(id)->next)
	{
		if(descr[ent_type(id)].render)
			descr[ent_type(id)].render(id);
	}
}

EntityID 
ent_new(EntityType type)
{
	EntityID new = _sys_new();
	_sys_node(new)->type = type;
	return new;
}

void
ent_del(EntityID id)
{
	_sys_del(id);
}

void *
ent_data(EntityID id)
{
	return &_sys_node(id)->data;
}

EntityType
ent_type(EntityID id)
{
	return _sys_node(id)->type;
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
	default:
		return NULL;
	}
}