#ifndef ENTITY_H
#define ENTITY_H

#include <stdbool.h>
#include <stdint.h>

#include "graphics.h"
#include "game_objects.h"
#include "vecmath.h"
#include "math.h"
#include "defs.h"

#define ENTITY_LIST                  \
	MAC_ENTITY(ENTITY_PLAYER)        \
	MAC_ENTITY(ENTITY_DUMMY)         \
	MAC_ENTITY(ENTITY_FIREBALL)      \
	MAC_ENTITY(ENTITY_DAMAGE_NUMBER) \
	MAC_ENTITY(ENTITY_PARTICLE)      \
	MAC_ENTITY(ENTITY_DOOR)

typedef enum {
	ENTITY_NULL,
	#define MAC_ENTITY(NAME) NAME,
		ENTITY_LIST
	#undef MAC_ENTITY
	LAST_ENTITY
} EntityType;

typedef struct {
	float health, health_max;
} Mob;

#define ENTITY_STRUCT(NAME) \
struct NAME##_struct

union Entity {
	ENTITY_STRUCT(ENTITY_DAMAGE_NUMBER) {
		vec2 position;
		float time, max_time;
		char damage_str[32];
		unsigned int text_id;
		SceneText *text;
	} damage_number;

	ENTITY_STRUCT(ENTITY_FIREBALL) {
		SceneSprite *sprite;
		Entity *caster;
		float time;

		float damage;
		Body *body;
	} fireball;

	ENTITY_STRUCT(ENTITY_DUMMY) {
		SceneSprite *sprite;
		Body *body;
		
		Mob mob;
	} dummy;

	ENTITY_STRUCT(ENTITY_PARTICLE) {
		Body *body;
		SceneSprite *sprite;
		float time;
	} particle;

	ENTITY_STRUCT(ENTITY_PLAYER) {
		SceneAnimatedSprite *sprite;

		int fired;
		bool moving;
		Mob mob;
		Body *body;
	} player;

	ENTITY_STRUCT(ENTITY_DOOR) {
		Direction direction;

		Body *body;
		bool open;
		float openness, openness_speed, door_angle;
		SceneLine *line;
	} door;
};

typedef struct ENTITY_DAMAGE_NUMBER_struct DamageNumber;
typedef struct ENTITY_FIREBALL_struct Fireball;
typedef struct ENTITY_DUMMY_struct Dummy;
typedef struct ENTITY_PARTICLE_struct Particle;
typedef struct ENTITY_PLAYER_struct Player;
typedef struct ENTITY_DOOR_struct Door;

typedef struct {
	void (*update)(Entity*, float delta);
	void (*render)(Entity*);
	void (*take_damage)(Entity*, float damage);
	void (*die)(Entity*);

	bool (*mouse_hovered)(Entity *, vec2 mouse_click);
	void (*mouse_interact)(Entity *self, Player *who, vec2 mouse_click);
} EntityInterface;


void ent_init(void);
void ent_end(void);
void ent_update(float delta);
void ent_render(void);
void ent_reset(void);

bool       ent_implements(Entity* id, ptrdiff_t offset);
void       ent_take_damage(Entity* id, float damage, vec2 damage_indicator_pos);

#define ENT_IMPLEMENTS(ID, FUNCTION_NAME) \
	ent_implements(ID, offsetof(EntityInterface, FUNCTION_NAME))

void mob_process(Entity *entity, Mob *mob);

Entity       *ent_new(EntityType type, EntityInterface *interface);
void          ent_del(Entity *entity);
EntityType    ent_type(Entity *entity);

Player       *ent_player_new(vec2 position);
Fireball     *ent_fireball_new(Entity *caster, vec2 position, vec2 vel);
Dummy        *ent_dummy_new(vec2 position);
DamageNumber *ent_damage_number(vec2 position, float damage);
Door         *ent_door_new(vec2 position, Direction direction);

Particle     *ent_particle_new(vec2 position, vec2 velocity, vec4 color, float time);
void          ent_shot_particles(vec2 position, vec2 velocity, vec4 color, float time, int count);

Entity *ent_hover(vec2 position);
void    ent_mouse_interact(Player *who, vec2 mouse_click);

void ent_door_open(Door *door);
void ent_door_close(Door *door);
bool ent_door_is_open(Door *door);
RelPtr   ent_relptr(void *ptr);

#define ENT_DATA(NAME, ID) ((NAME##_struct*)ent_data(ID))

#endif
