#include <stdbool.h>
#include "util.h"
#include "vecmath.h"
#include "entity.h"

typedef struct {
	EntityID next, prev;
	EntityType type;
	bool dead;
	union {
		EntityPlayer player;
		EntityDummy dummy;
		EntityFireball fireball;
	} data;
} EntityNode;

static struct {
	void (*update)(EntityID id, float delta);
	void (*render)(EntityID id);
	void (*delete)(EntityID id);
} descr[LAST_ENTITY] =
{
	[ENTITY_PLAYER] = { .update = ent_player_update, .render = ent_player_render, .delete = ent_player_del },
	[ENTITY_DUMMY] = { .update = ent_dummy_update, .render = ent_dummy_render, .delete = ent_dummy_del },
	[ENTITY_FIREBALL] = { .update = ent_fireball_update, .delete = ent_fireball_del }
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
