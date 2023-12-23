#ifndef GAME_OBJECTS_H
#define GAME_OBJECTS_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "util.h"

typedef enum {
	GAME_OBJECT_TYPE_NULL,
	GAME_OBJECT_TYPE_ENTITY,
	GAME_OBJECT_TYPE_BODY,
	GAME_OBJECT_TYPE_GFX_SCENE,
	LAST_GAME_OBJECT_TYPE
} GameObjectType;

typedef uint_fast32_t BodyID,
                      EntityID,
                      SceneObjectID,
                      SceneSpriteID,
                      SceneTextID,
                      SceneAnimatedSpriteID,
                      GameObjectID;

typedef struct GameObjectRegistry GameObjectRegistry;
struct GameObjectRegistry {
	bool activated;
	size_t privdata_size;
	size_t data_size;
	void (*clean_cbk)(GameObjectID);
};

typedef GameObjectRegistry GameObjectRegistrar[LAST_GAME_OBJECT_TYPE];

#define MAX_BITS_PER_TYPE 4
#define __GAME_OBJECT_ID_BITS    32
#define __GAME_OBJECT_TYPE_SHIFT (__GAME_OBJECT_ID_BITS - MAX_BITS_PER_TYPE)
#define __GAME_OBJECT_TYPE_MASK  (0xFFFFFFFF << __GAME_OBJECT_TYPE_SHIFT)
#define __GAME_OBJECT_ID_MASK	 (~__GAME_OBJECT_TYPE_MASK)

static inline GameObjectID make_id_descr(GameObjectType type, ObjectID id) 
{
	return ((type << __GAME_OBJECT_TYPE_SHIFT) | id) * !!id;
}

static inline GameObjectType id_type(GameObjectID id_type)
{
	return id_type >> __GAME_OBJECT_TYPE_SHIFT; 
}

static inline ObjectID id(GameObjectID id_type)
{
	return id_type & __GAME_OBJECT_ID_MASK; 
}

void          obj_init(GameObjectRegistrar objs);
void          obj_terminate(void);
void          obj_reset(GameObjectType type);

GameObjectID  obj_new(GameObjectType id_type);
void          obj_del(GameObjectType id);
void          obj_clean(void);
bool	      obj_is_dead(GameObjectID id);
void         *obj_data(GameObjectID id);
void         *obj_privdata(GameObjectID id);
GameObjectID  obj_begin(GameObjectType id_type);
GameObjectID  obj_next(GameObjectID obj);
RelPtr        obj_relptr(GameObjectType type, void *ptr);


#endif
