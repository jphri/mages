typedef unsigned int EntityID;
typedef unsigned int BodyID;
typedef unsigned int SceneSpriteID;

typedef float vec2[2];

#define ENTITY_LIST             \
	MAC_ENTITY(ENTITY_PLAYER)   \
	MAC_ENTITY(ENTITY_DUMMY)    \
	MAC_ENTITY(ENTITY_FIREBALL)

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

ENTITY_STRUCT(ENTITY_PLAYER) {
	BodyID body;
	SceneSpriteID sprite;
	int fired;
};

ENTITY_STRUCT(ENTITY_DUMMY) {
	BodyID body;
	SceneSpriteID sprite;
	float health;
};

ENTITY_STRUCT(ENTITY_FIREBALL) {
	BodyID body;
	SceneSpriteID sprite;
	EntityID caster;
	float time;
};

void ent_init();
void ent_end();
void ent_update(float delta);
void ent_render();

EntityID   ent_new(EntityType type);
void       ent_del(EntityID id);
void      *ent_data(EntityID id);
EntityType ent_type(EntityID id);

#define MAC_ENTITY(NAME) \
void NAME##_update(EntityID, float), NAME##_render(EntityID), NAME##_del(EntityID);
	ENTITY_LIST
#undef MAC_ENTITY

EntityID ent_player_new(vec2 position);
EntityID ent_fireball_new(EntityID caster, vec2 position, vec2 vel);
EntityID ent_dummy_new(vec2 position);

#define ENT_DATA(NAME, ID) ((NAME##_struct*)ent_data(ID))
