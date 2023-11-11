#ifndef ID_H
#define ID_H

typedef enum {
	ID_TYPE_NULL,
	ID_TYPE_ENTITY,
	ID_TYPE_BODY,
	LAST_ID_TYPE
} IDType;

#define MAX_BITS_PER_TYPE 4
#define UNSIGNED_INT_BITS (sizeof(unsigned int) * 8)
#define ID_TYPE_SHIFT (UNSIGNED_INT_BITS - MAX_BITS_PER_TYPE)
#define ID_TYPE_MASK  (0xFFFFFFFF << ID_TYPE_SHIFT)
#define ID_MASK       (~ID_TYPE_MASK)

typedef unsigned int BodyID,
					 EntityID,
					 SceneObjectID,
					 SceneSpriteID,
					 SceneTextID,
					 SceneAnimatedSpriteID;

static inline unsigned int make_id_descr(IDType type, unsigned int id) 
{
	return (type << ID_TYPE_SHIFT) | id;
}

static inline IDType id_type(unsigned int id_type)
{
	return id_type >> ID_TYPE_SHIFT; 
}

static inline unsigned int id(unsigned int id_type)
{
	return id_type & ID_MASK; 
}

#endif
