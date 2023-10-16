typedef float vec2[2];
typedef unsigned int BodyID;
typedef struct {
	vec2 position;
	vec2 half_size;
	vec2 velocity;
} Body;

typedef struct {
	BodyID id;
	vec2 normal;
	vec2 pierce;
} HitInfo;

void phx_init();
void phx_end();

BodyID phx_new();
void   phx_del(BodyID self);
void   phx_update(float delta);
void   phx_draw();
Body  *phx_data(BodyID self);

HitInfo *phx_hits(BodyID self_id, unsigned int *count);
