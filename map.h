typedef float vec2[2];
typedef struct CollisionData CollisionData;
struct CollisionData {
	vec2 position, half_size;
	CollisionData *next;
};

typedef struct {
	int w, h;
	int *tiles;
	CollisionData *collision;
} Map;

Map *map_alloc(int w, int h);
Map *map_load(const char *file);
void map_free(Map *);

char *map_export(Map *map, size_t *out_data_size);
void map_set_gfx_scene(Map *map);
void map_set_phx_scene(Map *map);
