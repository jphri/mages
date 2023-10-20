#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <assert.h>

#include "vecmath.h"
#include "glutil.h"
#include "util.h"

#include "graphics.h"
#include "third/stb_image.h"

enum Uniform {
	U_PROJECTION,
	U_VIEW,
	U_IMAGE_TEXTURE,
	U_SPRITE_CR,
	U_TILE_MAP_SIZE,
	U_ALBEDO_TEXTURE,
	LAST_UNIFORM
};

enum VertexAttrib {
	VATTRIB_POSITION,
	VATTRIB_TEXCOORD,
	VATTRIB_INST_POSITION,
	VATTRIB_INST_SIZE,
	VATTRIB_INST_COLOR,
	VATTRIB_INST_ROTATION,
	VATTRIB_INST_SPRITE_ID,
	LAST_VATTRIB
};

#define FBO_SCALE 4
#define MAX_SPRITES 1024

typedef struct {
	vec2 position;
	vec2 texcoord;
} SpriteVertex;

typedef struct {
	TextureAtlas texture;
	GLuint instance_buffer;
	GLuint vao;
	GLuint sprite_count;
} SpriteBuffer;

typedef struct {
	GLuint texture;
	int cols, rows;
} TextureAtlasData;

typedef struct {
	vec2 position;
	vec2 tile_data;
} TileData;

#include "base-renderer.h"

void
intrend_bind_uniforms(ShaderProgram *shader)
{
	intrend_uniform_bind(shader, U_VIEW,           "u_View");
	intrend_uniform_bind(shader, U_IMAGE_TEXTURE,  "u_ImageTexture");
	intrend_uniform_bind(shader, U_PROJECTION,     "u_Projection");
	intrend_uniform_bind(shader, U_SPRITE_CR,      "u_SpriteCR");
	intrend_uniform_bind(shader, U_ALBEDO_TEXTURE, "u_AlbedoTexture");
}

void
intrend_bind_attribs(ShaderProgram *shader) 
{
	intrend_attrib_bind(shader, VATTRIB_POSITION, "v_Position");
	intrend_attrib_bind(shader, VATTRIB_TEXCOORD, "v_Texcoord");

	intrend_attrib_bind(shader, VATTRIB_INST_POSITION,  "v_InstPosition");
	intrend_attrib_bind(shader, VATTRIB_INST_SIZE,      "v_InstSize");
	intrend_attrib_bind(shader, VATTRIB_INST_SPRITE_ID, "v_InstSpriteID");
	intrend_attrib_bind(shader, VATTRIB_INST_ROTATION,  "v_InstRotation");
	intrend_attrib_bind(shader, VATTRIB_INST_COLOR,     "v_InstColor");
}

static void   create_texture_buffer(int w, int h);
static void   init_shaders();
static GLuint load_texture(const char *file);

static SpriteBuffer load_sprite_buffer(TextureAtlas texture);
static void         draw_sprite_buffer(SpriteBuffer *buffer);
static void         insert_sprite_buffer(SpriteBuffer *buffer, Sprite *sprite);
static void         end_sprite_buffer(SpriteBuffer *buffer);

static TextureAtlasData load_texture_atlas(const char *file, int cols, int rows);
static void             end_texture_atlas(TextureAtlasData *texture);

static void             draw_tmap(GraphicsTileMap *tmap);

static void             draw_post(ShaderProgram *program);

static GLuint sprite_buffer_gpu;
static int screen_width, screen_height;
static ShaderProgram sprite_program, tile_map_program;
static SpriteBuffer sprite_buffers[LAST_SPRITE];
static TextureAtlasData texture_atlas[LAST_TEXTURE_ATLAS];
static ArrayBuffer tile_maps;

static GLuint albedo_fbo, albedo_texture;
static GLuint post_process_vbo, post_process_vao;

static ShaderProgram post_clean;

void
gfx_init()
{
	static SpriteVertex vertex_data[] = {
		{ .position = { -1.0, -1.0 }, .texcoord = { 0.0, 0.0 }, },
		{ .position = {  1.0, -1.0 }, .texcoord = { 1.0, 0.0 }, },
		{ .position = {  1.0,  1.0 }, .texcoord = { 1.0, 1.0 }, },
		{ .position = {  1.0,  1.0 }, .texcoord = { 1.0, 1.0 }, },
		{ .position = { -1.0,  1.0 }, .texcoord = { 0.0, 1.0 }, },
		{ .position = { -1.0, -1.0 }, .texcoord = { 0.0, 0.0 }, },
	};
	static SpriteVertex post_process[] = {
		{ .position = { -1.0, -1.0 }, .texcoord = { 0.0, 0.0 }, },
		{ .position = {  1.0, -1.0 }, .texcoord = { 1.0, 0.0 }, },
		{ .position = {  1.0,  1.0 }, .texcoord = { 1.0, 1.0 }, },
		{ .position = {  1.0,  1.0 }, .texcoord = { 1.0, 1.0 }, },
		{ .position = { -1.0,  1.0 }, .texcoord = { 0.0, 1.0 }, },
		{ .position = { -1.0, -1.0 }, .texcoord = { 0.0, 0.0 }, },
	};
	init_shaders();

	sprite_buffer_gpu = ugl_create_buffer(GL_STATIC_DRAW, sizeof(vertex_data), vertex_data);
	sprite_buffers[SPRITE_PLAYER] = load_sprite_buffer(TEXTURE_PLAYER);
	sprite_buffers[SPRITE_FIRIE] = load_sprite_buffer(TEXTURE_FIRIE);

	arrbuf_init(&tile_maps);

	texture_atlas[TEXTURE_PLAYER] = load_texture_atlas("textures/player.png", 2, 1);
	texture_atlas[TERRAIN_NORMAL] = load_texture_atlas("textures/terrain.png", 16, 16);

	post_process_vbo = ugl_create_buffer(GL_STATIC_DRAW, sizeof(post_process), post_process);
	post_process_vao = ugl_create_vao(2, (VaoSpec[]){
		{ .name = VATTRIB_POSITION, .size = 2, .type = GL_FLOAT, .stride = sizeof(SpriteVertex), .offset = offsetof(SpriteVertex, position), .buffer = post_process_vbo },
		{ .name = VATTRIB_TEXCOORD, .size = 2, .type = GL_FLOAT, .stride = sizeof(SpriteVertex), .offset = offsetof(SpriteVertex, texcoord), .buffer = post_process_vbo },
	});
}

void
gfx_end()
{
	glDeleteProgram(sprite_program.program);
	glDeleteBuffers(1, (GLuint[]) {
		sprite_buffer_gpu
	});

	for(int i = 0; i < LAST_SPRITE; i++)
		end_sprite_buffer(&sprite_buffers[i]);

	for(int i = 0; i < LAST_TEXTURE_ATLAS; i++)
		end_texture_atlas(&texture_atlas[i]);
}

void
gfx_draw_sprite(Sprite *sprite) 
{
	insert_sprite_buffer(&sprite_buffers[sprite->sprite_type], sprite);
}

void
gfx_render(int w, int h) 
{
	mat3 view, projection;
	mat3_ident(view);
	affine2d_ortho_window(projection, w, h);

	create_texture_buffer(w, h);

	glBindFramebuffer(GL_FRAMEBUFFER, albedo_fbo);
	glViewport(0, 0, w / FBO_SCALE, h / FBO_SCALE);
	
	glClearColor(0.2, 0.3, 0.7, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	intrend_bind_shader(&sprite_program);
	intrend_uniform_mat3(U_VIEW,       view);
	intrend_uniform_mat3(U_PROJECTION, projection);
	intrend_uniform_iv(U_IMAGE_TEXTURE, 1, 1, &(GLint){ 0 });

	intrend_bind_shader(&tile_map_program);
	intrend_uniform_mat3(U_VIEW,       view);
	intrend_uniform_mat3(U_PROJECTION, projection);
	intrend_uniform_iv(U_IMAGE_TEXTURE, 1, 1, &(GLint){ 0 });

	GraphicsTileMap *tmap_list = tile_maps.data;
	for(size_t i = 0; 
		i < arrbuf_length(&tile_maps, sizeof(GraphicsTileMap)); 
		i++) 
	{
		draw_tmap(&tmap_list[i]);
	}
	arrbuf_clear(&tile_maps);

	for(int i = 0; i < LAST_SPRITE; i++)
		draw_sprite_buffer(&sprite_buffers[i]);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, w, h);

	draw_post(&post_clean);
}

void
init_shaders()
{
	#define SHADER_PROGRAM(symbol, ...) \
		intrend_link(&symbol, #symbol, sizeof((GLuint[]){ __VA_ARGS__ })/sizeof(GLuint), (GLuint[]){ __VA_ARGS__ })

	#define LIST_SHADERS \
		SHADER(default_vertex,   "shaders/default.vsh", GL_VERTEX_SHADER)\
		SHADER(default_fragment, "shaders/default.fsh", GL_FRAGMENT_SHADER)\
		SHADER(tilemap_vertex,   "shaders/tilemap.vsh", GL_VERTEX_SHADER) \
		\
		SHADER(post_vertex,      "shaders/post.vsh", GL_VERTEX_SHADER)\
		SHADER(post_clean_fsh,   "shaders/post_clean.fsh", GL_FRAGMENT_SHADER)

	#define SHADER(shader_name, shader_path, shader_type) \
		GLuint shader_name = ugl_compile_shader_file(shader_path, shader_type);
		LIST_SHADERS
	#undef SHADER

	SHADER_PROGRAM(sprite_program, default_vertex, default_fragment);
	SHADER_PROGRAM(tile_map_program, tilemap_vertex, default_fragment);
	SHADER_PROGRAM(post_clean, post_vertex, post_clean_fsh);

	#define SHADER(shader_name, shader_path, shader_type) \
		glDeleteShader(shader_name);
		LIST_SHADERS
	#undef SHADER

	#undef LIST_SHADERS
	#undef SHADER_PROGRAM
}

void
create_texture_buffer(int w, int h)
{
	if(screen_width == w && screen_height == h)
		return;

	screen_width = w;
	screen_height = h;
	
	if(glIsTexture(albedo_texture))
		glDeleteTextures(1, &albedo_texture);

	if(glIsFramebuffer(albedo_fbo))
		glDeleteFramebuffers(1, &albedo_fbo);
	
	glGenTextures(1, &albedo_texture);
	glGenFramebuffers(1, &albedo_fbo);

	glBindTexture(GL_TEXTURE_2D, albedo_texture);
	glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_RGBA32F,
			screen_width / FBO_SCALE, screen_height / FBO_SCALE,
			0, 
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);
	
	glBindFramebuffer(GL_FRAMEBUFFER, albedo_fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, albedo_texture, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint
load_texture(const char *file)
{
	GLuint texture;
	int image_width, image_height;
	unsigned char *image_data;

	image_data = stbi_load(file, &image_width, &image_height, NULL, 4);
	if(!image_data) {
		printf("Could not load the texture %s\n", file);
		return -1;
	}

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_RGBA8,
			image_width, image_height,
			0, 
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			image_data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	stbi_image_free(image_data);
	return texture;
}


SpriteBuffer 
load_sprite_buffer(TextureAtlas texture)
{
	GLuint instance_buffer = ugl_create_buffer(GL_STATIC_DRAW, sizeof(Sprite) * MAX_SPRITES, NULL);
	return (SpriteBuffer) {
		.texture         = texture,
		.instance_buffer = instance_buffer,
		.sprite_count    = 0,
		.vao = ugl_create_vao(7, (VaoSpec[]){
			{ .name = VATTRIB_POSITION, .size = 2, .type = GL_FLOAT, .stride = sizeof(SpriteVertex), .offset = offsetof(SpriteVertex, position), .buffer = sprite_buffer_gpu },
			{ .name = VATTRIB_TEXCOORD, .size = 2, .type = GL_FLOAT, .stride = sizeof(SpriteVertex), .offset = offsetof(SpriteVertex, texcoord), .buffer = sprite_buffer_gpu },

			{ .name = VATTRIB_INST_POSITION,  .size = 2, .type = GL_FLOAT, .stride = sizeof(Sprite), .offset = offsetof(Sprite, position),  .divisor = 1, .buffer = instance_buffer },
			{ .name = VATTRIB_INST_SIZE,      .size = 2, .type = GL_FLOAT, .stride = sizeof(Sprite), .offset = offsetof(Sprite, half_size), .divisor = 1, .buffer = instance_buffer },
			{ .name = VATTRIB_INST_SPRITE_ID, .size = 2, .type = GL_FLOAT, .stride = sizeof(Sprite), .offset = offsetof(Sprite, sprite_id), .divisor = 1, .buffer = instance_buffer },
			{ .name = VATTRIB_INST_COLOR,     .size = 4, .type = GL_FLOAT, .stride = sizeof(Sprite), .offset = offsetof(Sprite, color),     .divisor = 1, .buffer = instance_buffer },
			{ .name = VATTRIB_INST_ROTATION,  .size = 1, .type = GL_FLOAT, .stride = sizeof(Sprite), .offset = offsetof(Sprite, rotation),  .divisor = 1, .buffer = instance_buffer },
		})
	};
}

void
draw_sprite_buffer(SpriteBuffer *buffer)
{
	if(buffer->sprite_count == 0)
		return;

	intrend_bind_shader(&sprite_program);
	intrend_uniform_fv(U_SPRITE_CR, 1, 2, (float[]){ 
		texture_atlas[buffer->texture].cols,
		texture_atlas[buffer->texture].rows
	});

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_atlas[buffer->texture].texture);
	intrend_draw_instanced(&sprite_program, buffer->vao, GL_TRIANGLES, 6, buffer->sprite_count);
	glBindTexture(GL_TEXTURE_2D, 0);

	buffer->sprite_count = 0;
}

void
insert_sprite_buffer(SpriteBuffer *buffer, Sprite *sprite)
{
	if(buffer->sprite_count >= MAX_SPRITES)
		return;
		
	glBindBuffer(GL_ARRAY_BUFFER,    buffer->instance_buffer);
	glBufferSubData(GL_ARRAY_BUFFER, buffer->sprite_count * sizeof(Sprite), sizeof(Sprite), sprite);
	glBindBuffer(GL_ARRAY_BUFFER,    0);
	buffer->sprite_count ++;
}

void
end_sprite_buffer(SpriteBuffer *buffer)
{
	glDeleteVertexArrays(1, &buffer->vao);
	glDeleteBuffers(1, &buffer->instance_buffer);
}

GraphicsTileMap
gfx_tmap_new(TextureAtlas terrain, int w, int h, int *data) 
{
	unsigned int count_tiles = 0;
	ArrayBuffer buffer;
	arrbuf_init(&buffer);

	for(int y = 0; y < h; y++) {
		for(int x = 0; x < w; x++) {
			int i = x + y * w;
			if(!data[i])
				continue;

			int spr = data[i] - 1;
			TileData info = {
				.position = { x, y },
				.tile_data = {
					(int)(spr % texture_atlas[terrain].cols),
					(int)(spr / texture_atlas[terrain].cols)
				}
			};
			arrbuf_insert(&buffer, sizeof(info), &info);
			count_tiles ++;
		}
	}

	unsigned int gpu_buffer = ugl_create_buffer(GL_STATIC_DRAW, buffer.size, buffer.data);
	unsigned int gpu_vao    = ugl_create_vao(4, (VaoSpec[]) {
		{ .name = VATTRIB_POSITION, .size = 2, .type = GL_FLOAT, .stride = sizeof(SpriteVertex),    .offset = offsetof(SpriteVertex, position), .buffer = sprite_buffer_gpu },
		{ .name = VATTRIB_TEXCOORD, .size = 2, .type = GL_FLOAT, .stride = sizeof(SpriteVertex),    .offset = offsetof(SpriteVertex, texcoord), .buffer = sprite_buffer_gpu },
		{ .name = VATTRIB_INST_POSITION,   .size = 2, .type = GL_FLOAT, .stride = sizeof(TileData), .offset = offsetof(TileData, position),  .divisor = 1, .buffer = gpu_buffer },
		{ .name = VATTRIB_INST_SPRITE_ID,  .size = 2, .type = GL_FLOAT, .stride = sizeof(TileData), .offset = offsetof(TileData, tile_data), .divisor = 1, .buffer = gpu_buffer },
	});

	arrbuf_free(&buffer);

	return (GraphicsTileMap) {
		.buffer = gpu_buffer,
		.vao = gpu_vao,
		.count_tiles = count_tiles,
		.terrain = terrain,
	};
}

void
gfx_tmap_free(GraphicsTileMap *tmap) 
{
	glDeleteBuffers(1, &tmap->buffer);
	glDeleteVertexArrays(1, &tmap->vao);
}

void
gfx_tmap_draw(GraphicsTileMap *tmap) 
{
	arrbuf_insert(&tile_maps, sizeof(*tmap), tmap);
}

void
draw_tmap(GraphicsTileMap *tmap) 
{
	intrend_bind_shader(&tile_map_program);
	intrend_uniform_fv(U_SPRITE_CR, 1, 2, (GLfloat[]){ 
			texture_atlas[tmap->terrain].cols, 
			texture_atlas[tmap->terrain].rows 
	});
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_atlas[tmap->terrain].texture);
	intrend_draw_instanced(&tile_map_program, tmap->vao, GL_TRIANGLES, 6, tmap->count_tiles);
}

TextureAtlasData
load_texture_atlas(const char *path, int cols, int rows) 
{
	return (TextureAtlasData) {
		.texture = load_texture(path),
		.cols = cols,
		.rows = rows,
	};
}

void
end_texture_atlas(TextureAtlasData *terrain)
{
	glDeleteTextures(1, &terrain->texture);
}

void
draw_post(ShaderProgram *program)
{
	intrend_bind_shader(program);
	intrend_uniform_iv(U_ALBEDO_TEXTURE, 1, 1, (int[]){ 0 });

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, albedo_texture);

	intrend_draw(program, post_process_vao, GL_TRIANGLES, 6);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
}
