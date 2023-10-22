typedef struct {
	int w, h;
	int tiles[];
} Map;

Map *map_alloc(int w, int h);
Map *map_load(const char *file);

char *map_export(Map *map, size_t *out_data_size);
void map_set_gfx_scene(Map *map);
