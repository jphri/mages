#include "util.h"
#include "vecmath.h"
#include "entity.h"

typedef struct {
	EntityID next, prev;
	EntityType type;
	union {
		EntityPlayer player;
	} data;
} EntityNode;

static struct {
	void (*update)(EntityID id, float delta);
	void (*render)(EntityID id);
	void (*delete)(EntityID id);
} descr[LAST_ENTITY] =
{
	[ENTITY_PLAYER] = { .update = ent_player_update, .render = ent_player_render, .delete = ent_player_del }
};

#define SYS_ID_TYPE EntityID
#define SYS_NODE_TYPE EntityNode
#include "system.h"

static ArrayBuffer should_die_buffer;

void
ent_init()
{
	_sys_init();
	arrbuf_init(&should_die_buffer);
}

void
ent_end()
{
	_sys_deinit();
}

void
ent_update(float delta) 
{
	arrbuf_clear(&should_die_buffer);

	for(EntityID id = _sys_list;
		id;
		id = _sys_node(id)->next)
	{
		if(descr[ent_type(id)].update)
			descr[ent_type(id)].update(id, delta);
	}

	EntityID *should_die = should_die_buffer.data;
	for(int i = 0; 
		i < arrbuf_length(&should_die_buffer, sizeof(EntityID)); 
		i++)
	{
		if(descr[ent_type(should_die[i])].delete)
			descr[ent_type(should_die[i])].delete(should_die[i]);
		_sys_del(should_die[i]);
	}
}

void
ent_render()
{
	for(EntityID id = _sys_list;
		id;
		id = _sys_node(id)->next)
	{
		if(descr[ent_type(id)].update)
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
	arrbuf_insert(&should_die_buffer, sizeof(id), &id);
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
