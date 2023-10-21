typedef unsigned int EntityID;
typedef unsigned int BodyID;

typedef unsigned int SceneSpriteID;

typedef float vec2[2];

typedef enum {
	ENTITY_NULL,
	ENTITY_PLAYER,
	LAST_ENTITY
} EntityType;

typedef struct {
	BodyID body;
	SceneSpriteID sprite;
} EntityPlayer;

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
