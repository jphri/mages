typedef unsigned int EntityID;
typedef unsigned int BodyID;
typedef unsigned int SceneSpriteID;

typedef float vec2[2];

typedef enum {
	ENTITY_NULL,
	ENTITY_PLAYER,
	ENTITY_DUMMY,
	ENTITY_FIREBALL,
	LAST_ENTITY
} EntityType;

typedef struct {
	BodyID body;
	SceneSpriteID sprite;
	int fired;
} EntityPlayer;

typedef struct {
	BodyID body;
	SceneSpriteID sprite;
	float health;
} EntityDummy;

typedef struct {
	BodyID body;
	SceneSpriteID sprite;
	EntityID caster;
	float time;
} EntityFireball;

void ent_init();
void ent_end();
void ent_update(float delta);
void ent_render();

EntityID   ent_new(EntityType type);
void       ent_del(EntityID id);
void      *ent_data(EntityID id);
EntityType ent_type(EntityID id);

EntityID ent_player_new(vec2 position);
void ent_player_update(EntityID id, float delta);
void ent_player_render(EntityID id);
void ent_player_del(EntityID id);

EntityID ent_dummy_new(vec2 position);
void     ent_dummy_update(EntityID id, float delta);
void     ent_dummy_render(EntityID id);
void     ent_dummy_del(EntityID id);

EntityID ent_fireball_new(EntityID caster, vec2 position, vec2 vel);
void     ent_fireball_update(EntityID id, float delta);
void     ent_fireball_del(EntityID id);
