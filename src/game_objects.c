#include <stdio.h>
#include <stdlib.h>

#include "game_objects.h"
#include "entity.h"
#include "physics.h"
#include "graphics.h"

#include "util.h"

static GameObjectRegistry *object_registry;
static ObjectAllocator *object_allocators;

static void internal_clean_callback(ObjectAllocator *alloc, ObjectID obj_id) 
{
	int type = alloc - object_allocators;
	object_registry[type].clean_cbk(make_id_descr(type, obj_id));
}

static inline ObjectAllocator *get_alloc(GameObjectID id)
{
	return &object_allocators[id_type(id)];
}

void
obj_init(GameObjectRegistrar registrar)
{
	object_registry = malloc(sizeof(registrar[0]) * LAST_GAME_OBJECT_TYPE);
	memcpy(object_registry, registrar, sizeof(object_registry[0]) * LAST_GAME_OBJECT_TYPE);
	object_allocators = malloc(sizeof(ObjectAllocator) * LAST_GAME_OBJECT_TYPE);

	for(int i = 0; i < LAST_GAME_OBJECT_TYPE; i++) {
		if(object_registry[i].activated) {
			objalloc_init(&object_allocators[i], object_registry[i].data_size + object_registry[i].privdata_size);
			if(object_registry[i].clean_cbk)
				object_allocators[i].clean_cbk = internal_clean_callback;
			else
				object_allocators[i].clean_cbk = NULL;
		}
	}

}

void
obj_terminate(void)
{
	for(int i = 0; i < LAST_GAME_OBJECT_TYPE; i++) {
		if(object_registry[i].activated) {
			objalloc_end(&object_allocators[i]);
		}
	}
}

void
obj_reset(GameObjectType type)
{
	objalloc_reset(&object_allocators[type]);
}

GameObjectID
obj_new(GameObjectType id_type)
{
	ObjectID obj_id = objalloc_alloc(&object_allocators[id_type]);
	return make_id_descr(id_type, obj_id);
}

void
obj_del(GameObjectType game_id)
{
	ObjectAllocator *alloc = get_alloc(game_id);
	
	ObjectID obj_id = id(game_id);
	objalloc_free(alloc, obj_id);
}


void
obj_clean(void)
{
	for(int i = 0; i < LAST_GAME_OBJECT_TYPE; i++) {
		if(object_registry[i].activated)
			objalloc_clean(&object_allocators[i]);
	}
}

bool
obj_is_dead(GameObjectID game_id)
{
	ObjectAllocator *alloc = get_alloc(game_id);
	return objalloc_is_dead(alloc, id(game_id));
}

void *
obj_data(GameObjectID game_id)
{
	ObjectAllocator *alloc = get_alloc(game_id);
	return (unsigned char *)objalloc_data(alloc, id(game_id)) + object_registry[id_type(game_id)].privdata_size;
}

void *
obj_privdata(GameObjectID game_id)
{
	ObjectAllocator *alloc = get_alloc(game_id);
	return (unsigned char *)objalloc_data(alloc, id(game_id));
}

GameObjectID
obj_begin(GameObjectType id_type)
{
	ObjectAllocator *alloc = &object_allocators[id_type];
	return make_id_descr(id_type, objalloc_begin(alloc));
}

GameObjectID
obj_next(GameObjectID obj)
{
	ObjectAllocator *alloc = get_alloc(obj);
	return make_id_descr(id_type(obj), objalloc_next(alloc, id(obj)));
}

RelPtr
obj_relptr(GameObjectType type, void *ptr)
{
	return (RelPtr) {
		.base_pointer = &object_allocators[type].data_buffer.data,
		.offset = (unsigned char*)ptr - (unsigned char*)object_allocators[type].data_buffer.data
	};
}
