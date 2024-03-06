#ifndef ENTITY_H
#define ENTITY_H

#include <stdbool.h>
#include <stdint.h>

#include "game_objects.h"
#include "vecmath.h"
#include "math.h"
#include "physics.h"

#define ENTITY_LIST             \
	MAC_ENTITY(ENTITY_PLAYER)   \
	MAC_ENTITY(ENTITY_DUMMY)    \
	MAC_ENTITY(ENTITY_FIREBALL) \
	MAC_ENTITY(ENTITY_DAMAGE_NUMBER) \
	MAC_ENTITY(ENTITY_PARTICLE)

typedef enum {
	ENTITY_COMP_MOB,
	ENTITY_COMP_DAMAGE,
	ENTITY_COMP_BODY,
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

typedef struct {
	BodyID body;
	void (*pre_solve)(EntityID self, BodyID other, Contact *contact);
} EntityBody;

ENTITY_STRUCT(ENTITY_PARTICLE) {
	SceneSpriteID sprite;
	float time;
	EntityBody body;
};

ENTITY_STRUCT(ENTITY_PLAYER) {
	SceneSpriteID sprite;
	int fired;
	bool moving;
	EntityMob mob;
	EntityBody body;
};

ENTITY_STRUCT(ENTITY_DUMMY) {
	SceneSpriteID sprite;
	EntityMob mob;
	EntityBody body;
};

ENTITY_STRUCT(ENTITY_FIREBALL) {
	SceneSpriteID sprite;
	EntityID caster;
	float time;
	
	EntityDamage damage;
	EntityBody body;
};

ENTITY_STRUCT(ENTITY_DAMAGE_NUMBER) {
	vec2 position;
	float time, max_time;
	char damage_str[32];
	unsigned int text_id;
};

typedef struct {
	void (*update)(EntityID, float delta);
	void (*render)(EntityID);
	void (*take_damage)(EntityID, float damage);
	void (*die)(EntityID);
} EntityInterface;

void ent_init(void);
void ent_end(void);
void ent_update(float delta);
void ent_render(void);
void ent_reset(void);

EntityID   ent_new(EntityType type, EntityInterface *interface);
void       ent_del(EntityID id);
void      *ent_data(EntityID id);
EntityType ent_type(EntityID id);
RelPtr     ent_relptr(void *ptr);

bool       ent_implements(EntityID id, ptrdiff_t offset);
void       ent_take_damage(EntityID id, float damage, vec2 damage_indicator_pos);

#define ENT_IMPLEMENTS(ID, FUNCTION_NAME) \
	ent_implements(ID, offsetof(EntityInterface, FUNCTION_NAME))

EntityID ent_player_new(vec2 position);
EntityID ent_fireball_new(EntityID caster, vec2 position, vec2 vel);
EntityID ent_dummy_new(vec2 position);
EntityID ent_damage_number(vec2 position, float damage);

EntityID ent_particle_new(vec2 position, vec2 velocity, vec4 color, float time);
void     ent_shot_particles(vec2 position, vec2 velocity, vec4 color, float time, int count);

#define ENT_DATA(NAME, ID) ((NAME##_struct*)ent_data(ID))

#endif
