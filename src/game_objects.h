#ifndef GAME_OBJECTS_H
#define GAME_OBJECTS_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "util.h"

//typedef enum {
//	GAME_OBJECT_TYPE_NULL,
//	GAME_OBJECT_TYPE_ENTITY,
//	GAME_OBJECT_TYPE_BODY,
//	GAME_OBJECT_TYPE_GFX_SCENE,
//	LAST_GAME_OBJECT_TYPE
//} GameObjectType;
//
//typedef uint_least32_t BodyID,
//                       EntityID,
//                       SceneObjectID,
//                       SceneSpriteID,
//                       SceneTextID,
//                       SceneAnimatedSpriteID,
//                       GameObjectID;
//
//#define __MAX_BITS_PER_TYPE      4
//#define __GAME_OBJECT_ID_BITS    (sizeof(GameObjectID) * 8)
//#define __GAME_OBJECT_TYPE_SHIFT (__GAME_OBJECT_ID_BITS - __MAX_BITS_PER_TYPE)
//#define __GAME_OBJECT_ID_MASK    (((GameObjectID)0x01 << __GAME_OBJECT_TYPE_SHIFT) - 1)
//#define __GAME_OBJECT_TYPE_MASK	 (~__GAME_OBJECT_ID_MASK)
//
//static inline GameObjectID make_gobj_id(GameObjectType type, ObjectID id) 
//{
//	return (((GameObjectID)type << __GAME_OBJECT_TYPE_SHIFT) | id) * !!id;
//}
//
//static inline GameObjectType gobj_type(GameObjectID id_type)
//{
//	return id_type >> __GAME_OBJECT_TYPE_SHIFT; 
//}
//
//static inline ObjectID gobj_id(GameObjectID id_type)
//{
//	return id_type & __GAME_OBJECT_ID_MASK; 
//}
//
#endif
