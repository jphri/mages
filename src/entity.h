#ifndef ENTITY_H
#define ENTITY_H

#include "id.h"

#define ENTITY_LIST             \
	MAC_ENTITY(ENTITY_PLAYER)   \
	MAC_ENTITY(ENTITY_DUMMY)    \
	MAC_ENTITY(ENTITY_FIREBALL) \
	MAC_ENTITY(ENTITY_DAMAGE_NUMBER)

typedef enum {
	ENTITY_COMP_MOB,
	ENTITY_COMP_DAMAGE,
} EntityComponent;

typedef enum {
	ENTITY_NULL,
	#define MAC_ENTITY(NAME) NAME,
		ENTITY_LIST
	#undef MAC_ENTITY
	LAST_ENTITY
} EntityType;

#define ENTITY_STRUCT(NAME) \
typedef struct NAME##_struct NAME##_struct; \
struct NAME##_struct

typedef struct {
	float health, health_max;
} EntityMob;

typedef struct {
	float damage;
} EntityDamage;

ENTITY_STRUCT(ENTITY_PLAYER) {
	BodyID body;
	SceneSpriteID sprite;
	int fired;
	bool moving;
	EntityMob mob;
};

ENTITY_STRUCT(ENTITY_DUMMY) {
	BodyID body;
	SceneSpriteID sprite;
	EntityMob mob;
};

ENTITY_STRUCT(ENTITY_FIREBALL) {
	BodyID body;
	SceneSpriteID sprite;
	EntityID caster;
	float time;
	
	EntityDamage damage;
};

ENTITY_STRUCT(ENTITY_DAMAGE_NUMBER) {
	vec2 position;
	float time, max_time;
	char damage_str[32];
	unsigned int text_id;
};

void ent_init();
void ent_end();
void ent_update(float delta);
void ent_render();

EntityID   ent_new(EntityType type);
void       ent_del(EntityID id);
void      *ent_data(EntityID id);
EntityType ent_type(EntityID id);

void *ent_component(EntityID id, EntityComponent comp);

#define MAC_ENTITY(NAME) \
void NAME##_update(EntityID, float), NAME##_render(EntityID), NAME##_del(EntityID);
	ENTITY_LIST
#undef MAC_ENTITY

EntityID ent_player_new(vec2 position);
EntityID ent_fireball_new(EntityID caster, vec2 position, vec2 vel);
EntityID ent_dummy_new(vec2 position);
EntityID ent_damage_number(vec2 position, float damage);

#define ENT_DATA(NAME, ID) ((NAME##_struct*)ent_data(ID))

#endif
