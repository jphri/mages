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
	VATTRIB_COLOR,
	VATTRIB_INST_POSITION,
	VATTRIB_INST_SIZE,
	VATTRIB_INST_COLOR,
	VATTRIB_INST_ROTATION,
	VATTRIB_INST_SPRITE_ID,
	VATTRIB_INST_SPRITE_TYPE,
	VATTRIB_INST_CLIP,
	LAST_VATTRIB
};

#define FBO_SCALE 4
#define MAX_SPRITES 1024

typedef struct {
	vec2 position;
	vec2 texcoord;
} SpriteVertex;

typedef struct {
	GLuint texture;
	int cols, rows;
} TextureAtlasData;

typedef struct {
	vec2 position;
	vec2 tile_data;
} TileData;

typedef struct {
	vec2 position;
	vec4 color;
} DebugVertex;

typedef enum {
	TEXTURE_FORMAT_RGBA32,
	TEXTURE_FORMAT_RED
} TextureFormat;

#include "base-renderer.h"

void
intrend_bind_uniforms(ShaderProgram *shader)
{
	intrend_uniform_bind(shader, U_VIEW,           "u_View");
	intrend_uniform_bind(shader, U_IMAGE_TEXTURE,  "u_ImageTexture");
	intrend_uniform_bind(shader, U_PROJECTION,     "u_Projection");
	intrend_uniform_bind(shader, U_SPRITE_CR,      "u_SpriteCR");
	intrend_uniform_bind(shader, U_ALBEDO_TEXTURE, "u_AlbedoTexture");

	glUniformBlockBinding(
		shader->program,
		glGetUniformBlockIndex(shader->program, "u_TransformBlock"),
		0
	);

	glUniformBlockBinding(
		shader->program,
		glGetUniformBlockIndex(shader->program, "u_SpriteDataBlock"),
		1
	);
}

void
intrend_bind_attribs(ShaderProgram *shader) 
{
	intrend_attrib_bind(shader, VATTRIB_POSITION, "v_Position");
	intrend_attrib_bind(shader, VATTRIB_TEXCOORD, "v_Texcoord");
	intrend_attrib_bind(shader, VATTRIB_COLOR, "v_Color");

	intrend_attrib_bind(shader, VATTRIB_INST_POSITION,  "v_InstPosition");
	intrend_attrib_bind(shader, VATTRIB_INST_SIZE,      "v_InstSize");
	intrend_attrib_bind(shader, VATTRIB_INST_SPRITE_ID, "v_InstSpriteID");
	intrend_attrib_bind(shader, VATTRIB_INST_ROTATION,  "v_InstRotation");
	intrend_attrib_bind(shader, VATTRIB_INST_COLOR,     "v_InstColor");
	intrend_attrib_bind(shader, VATTRIB_INST_SPRITE_TYPE, "v_InstSpriteType");
	intrend_attrib_bind(shader, VATTRIB_INST_CLIP, "v_ClipRegion");
}

static void   create_texture_buffer(int w, int h);
static void   init_shaders();
static GLuint load_texture(const char *file, TextureFormat format);

static TextureAtlasData load_texture_atlas(const char *file, int cols, int rows, TextureFormat format);
static void             end_texture_atlas(TextureAtlasData *texture);
static void             draw_tmap(GraphicsTileMap *tmap);
static void             draw_post(ShaderProgram *program);

static void             insert_character(TextureAtlas atlas, vec2 position, vec2 size, vec4 color, vec4 clip_region, int c);

static GLuint sprite_buffer_gpu;
static int screen_width, screen_height;
static ShaderProgram sprite_program, tile_map_program, font_program;
static ShaderProgram debug_program;

static TextureAtlasData texture_atlas[LAST_TEXTURE_ATLAS];

static GLuint albedo_fbo, albedo_texture;
static GLuint post_process_vbo, post_process_vao;

static ShaderProgram post_clean;

static mat4 projection;
static mat4 view_matrix;

static GLuint matrix_buffer;
static GLuint sprite_colrow_inv_buffer;
static GraphicsTileMap *current_tmap;

static GLuint sprite_buffer, sprite_vao, sprite_count;

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

	gfx_set_camera((vec2){ 0.0, 0.0 }, (vec2){ 16, 16 });

	sprite_buffer_gpu               = ugl_create_buffer(GL_STATIC_DRAW, sizeof(vertex_data), vertex_data);

	texture_atlas[TEXTURE_ENTITIES]       = load_texture_atlas("textures/entities.png", 16, 16, TEXTURE_FORMAT_RGBA32);
	texture_atlas[TERRAIN_NORMAL]         = load_texture_atlas("textures/terrain.png", 16, 16, TEXTURE_FORMAT_RGBA32);
	texture_atlas[TEXTURE_FONT_CELLPHONE] = load_texture_atlas("textures/charmap-cellphone.png", 18, 7, TEXTURE_FORMAT_RGBA32);
	texture_atlas[TEXTURE_UI] = load_texture_atlas("textures/ui.png", 32, 32, TEXTURE_FORMAT_RGBA32);

	post_process_vbo = ugl_create_buffer(GL_STATIC_DRAW, sizeof(post_process), post_process);
	post_process_vao = ugl_create_vao(2, (VaoSpec[]){
		{ .name = VATTRIB_POSITION, .size = 2, .type = GL_FLOAT, .stride = sizeof(SpriteVertex), .offset = offsetof(SpriteVertex, position), .buffer = post_process_vbo },
		{ .name = VATTRIB_TEXCOORD, .size = 2, .type = GL_FLOAT, .stride = sizeof(SpriteVertex), .offset = offsetof(SpriteVertex, texcoord), .buffer = post_process_vbo },
	});

	intrend_bind_shader(&sprite_program);
	intrend_uniform_iv(U_IMAGE_TEXTURE, 8, 1, (GLint[]){ 0, 1, 2, 3, 4, 5, 6, 7 });

	intrend_bind_shader(&font_program);
	intrend_uniform_iv(U_IMAGE_TEXTURE, 8, 1, (GLint[]){ 0, 1, 2, 3, 4, 5, 6, 7 });

	intrend_bind_shader(&tile_map_program);
	intrend_uniform_iv(U_IMAGE_TEXTURE, 8, 1, (GLint[]){ 0, 1, 2, 3, 4, 5, 6, 7 });
	
	intrend_bind_shader(&debug_program);
	intrend_uniform_iv(U_IMAGE_TEXTURE, 8, 1, (GLint[]){ 0, 1, 2, 3, 4, 5, 6, 7 });

	glGenBuffers(1, &matrix_buffer);
	glBindBuffer(GL_UNIFORM_BUFFER, matrix_buffer);
	glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(mat4), NULL, GL_STREAM_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	vec2 data[] = {
		#define TEX(TEXTURE_ATLAS) { 1.0 / texture_atlas[TEXTURE_ATLAS].cols, 1.0 / texture_atlas[TEXTURE_ATLAS].rows }
		TEX(TEXTURE_ENTITIES),
		{0},
		TEX(TERRAIN_NORMAL),
		{0},
		TEX(TEXTURE_FONT_CELLPHONE),
		{0},
		TEX(TEXTURE_UI),
		{0},
		#undef TEX
	};
	glGenBuffers(1, &sprite_colrow_inv_buffer);
	glBindBuffer(GL_UNIFORM_BUFFER, sprite_colrow_inv_buffer);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	mat4_ident(projection);
	mat4_ident(view_matrix);

	sprite_buffer = ugl_create_buffer(GL_STREAM_DRAW, sizeof(Sprite) * MAX_SPRITES, NULL);
	sprite_vao = ugl_create_vao(9, (VaoSpec[]){
		{ .name = VATTRIB_POSITION, .size = 2, .type = GL_FLOAT, .stride = sizeof(SpriteVertex), .offset = offsetof(SpriteVertex, position), .buffer = sprite_buffer_gpu },
		{ .name = VATTRIB_TEXCOORD, .size = 2, .type = GL_FLOAT, .stride = sizeof(SpriteVertex), .offset = offsetof(SpriteVertex, texcoord), .buffer = sprite_buffer_gpu },
		{ .name = VATTRIB_INST_SPRITE_TYPE, .size = 1, .type = GL_INT,   .stride = sizeof(Sprite), .offset = offsetof(Sprite, type),        .divisor = 1, .buffer = sprite_buffer },
		{ .name = VATTRIB_INST_POSITION,    .size = 2, .type = GL_FLOAT, .stride = sizeof(Sprite), .offset = offsetof(Sprite, position),    .divisor = 1, .buffer = sprite_buffer },
		{ .name = VATTRIB_INST_SIZE,        .size = 2, .type = GL_FLOAT, .stride = sizeof(Sprite), .offset = offsetof(Sprite, half_size),   .divisor = 1, .buffer = sprite_buffer },
		{ .name = VATTRIB_INST_SPRITE_ID,   .size = 2, .type = GL_FLOAT, .stride = sizeof(Sprite), .offset = offsetof(Sprite, sprite_id),   .divisor = 1, .buffer = sprite_buffer },
		{ .name = VATTRIB_INST_COLOR,       .size = 4, .type = GL_FLOAT, .stride = sizeof(Sprite), .offset = offsetof(Sprite, color),       .divisor = 1, .buffer = sprite_buffer },
		{ .name = VATTRIB_INST_ROTATION,    .size = 1, .type = GL_FLOAT, .stride = sizeof(Sprite), .offset = offsetof(Sprite, rotation),    .divisor = 1, .buffer = sprite_buffer },
		{ .name = VATTRIB_INST_CLIP,        .size = 4, .type = GL_FLOAT, .stride = sizeof(Sprite), .offset = offsetof(Sprite, clip_region), .divisor = 1, .buffer = sprite_buffer },
	});
	sprite_count = 0;
}

void
gfx_end()
{
	glDeleteProgram(sprite_program.program);
	glDeleteBuffers(1, (GLuint[]) {
		sprite_buffer_gpu,
		sprite_buffer,
	});

	for(int i = 0; i < LAST_TEXTURE_ATLAS; i++)
		end_texture_atlas(&texture_atlas[i]);
}

void
gfx_draw_sprite(Sprite *sprite) 
{
	if(sprite_count >= MAX_SPRITES)
		return;
	glBindBuffer(GL_ARRAY_BUFFER, sprite_buffer);
	glBufferSubData(GL_ARRAY_BUFFER, sprite_count * sizeof(*sprite), sizeof(*sprite), sprite);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	sprite_count++;
}

void
gfx_draw_font(TextureAtlas atlas, vec2 position, vec2 char_size, vec4 color, vec4 clip_region, const char *fmt, ...) 
{
	char buffer[1024];
	va_list va;
	int characters;

	va_start(va, fmt);
	characters = vsnprintf(buffer, sizeof(buffer), fmt, va);
	va_end(va);

	vec2 p;
	vec2_dup(p, position);

	for(int i = 0; i < characters; i++) {
		switch(buffer[i]) {
		case '\n':
			p[0] = position[0];
			p[1] += char_size[1] * 2.0;
			break;
		default:
			insert_character(atlas, p, char_size, color, clip_region, buffer[i]);
			p[0] += char_size[0] * 2.0;
		}
	}
}

void
gfx_draw_line(TextureAtlas atlas, vec2 p1, vec2 p2, float thickness, vec4 color, vec4 clip_region)
{
	vec2 dir;
	vec2 sprite_pos, sprite_size;

	vec2_sub(dir, p1, p2);
	float distance = vec2_dot(dir, dir);
	distance = sqrtf(distance) / 2.0;

	vec2_add(sprite_pos, p2, p1);
	sprite_pos[0] = (sprite_pos[0]) / 2.0;
	sprite_pos[1] = (sprite_pos[1]) / 2.0;
	sprite_size[0] = distance + thickness;
	sprite_size[1] = thickness;
	float rotation = atan2f(-dir[1], dir[0]);

	gfx_draw_sprite(&(Sprite) {
		.type = atlas,
		.position = { sprite_pos[0], sprite_pos[1] },
		.rotation = rotation,
		.clip_region = { clip_region[0], clip_region[1], clip_region[2], clip_region[3] },
		.half_size = { sprite_size[0], sprite_size[1] },
		.color = { color[0], color[1], color[2], color[3] },
		.sprite_id = { 0, 0 }
	});
}

void
gfx_draw_rect(TextureAtlas atlas, vec2 position, vec2 half_size, float thickness, vec4 color, vec4 clip_region)
{
	vec2 min, max;
	vec2_sub(min, position, half_size);
	vec2_add(max, position, half_size);
	
	#define ADD_LINE(x1, y1, x2, y2) gfx_draw_line(atlas, (vec2){ x1, y1 }, (vec2){ x2, y2 }, thickness, color, clip_region)
	ADD_LINE(min[0], min[1], max[0], min[1]);
	ADD_LINE(max[0], min[1], max[0], max[1]);
	ADD_LINE(max[0], max[1], min[0], max[1]);
	ADD_LINE(min[0], max[1], min[0], min[1]);
	#undef ADD_LINE
}

void
gfx_make_framebuffers(int w, int h) 
{
	affine2d_setup_ortho_window(projection, w, h);
	create_texture_buffer(w, h);

	glBindBuffer(GL_ARRAY_BUFFER, matrix_buffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(projection), projection);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void
gfx_clear_framebuffers()
{
	glBindFramebuffer(GL_FRAMEBUFFER, albedo_fbo);
	glViewport(0, 0, screen_width / FBO_SCALE, screen_height / FBO_SCALE);
	
	glClearColor(0.2, 0.3, 0.7, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void
gfx_setup_draw_framebuffers()
{
	glBindFramebuffer(GL_FRAMEBUFFER, albedo_fbo);
	glViewport(0, 0, screen_width / FBO_SCALE, screen_height / FBO_SCALE);
}

void
gfx_end_draw_framebuffers()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, screen_width, screen_height);
}

void
gfx_draw_begin(GraphicsTileMap *tmap) 
{
	current_tmap = tmap;
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void
gfx_draw_end()
{
	glBindBuffer(GL_UNIFORM_BUFFER, matrix_buffer);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(projection), sizeof(view_matrix), view_matrix);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	for(int i = 0; i < LAST_TEXTURE_ATLAS; i++) {
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, texture_atlas[i].texture);
	}

	glBindBufferBase(GL_UNIFORM_BUFFER, 1, sprite_colrow_inv_buffer);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, matrix_buffer);
	if(current_tmap)
		draw_tmap(current_tmap);

	intrend_draw_instanced(&sprite_program, sprite_vao, GL_TRIANGLES, 6, sprite_count);

	glBindBufferBase(GL_UNIFORM_BUFFER, 0, 0);
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, 0);

	for(int i = 0; i < LAST_TEXTURE_ATLAS; i++) {
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	sprite_count = 0;
	glDisable(GL_BLEND);
}

void
gfx_begin_scissor(vec2 position, vec2 size)
{
	vec2 scissor_pos, scissor_size;

	glEnable(GL_SCISSOR_TEST);
	vec2_sub(scissor_pos, position, size);
	vec2_add(scissor_size, size, size);

	#define F(X, Y, W, H) \
		glScissor((X), (Y), (W), (H)); 

	F(scissor_pos[0], screen_height - scissor_pos[1] - scissor_size[1], scissor_size[0], scissor_size[1]);

	#undef F
}

void
gfx_end_scissor()
{
	glDisable(GL_SCISSOR_TEST);
}

void
gfx_render_present() 
{
	draw_post(&post_clean);
}

void
gfx_set_camera(vec2 position, vec2 scale)
{
	mat4_ident(view_matrix);
	affine2d_scale(view_matrix,     scale);
	affine2d_translate(view_matrix, position);
}

void
gfx_pixel_to_world(vec2 pixel, vec2 world_out) 
{
	world_out[0] = (pixel[0] - view_matrix[2][0]) / view_matrix[0][0];
	world_out[1] = (pixel[1] - view_matrix[2][1]) / view_matrix[1][1];
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
		SHADER(debug_vertex,     "shaders/debug.vsh", GL_VERTEX_SHADER)\
		SHADER(debug_fragment,   "shaders/debug.fsh", GL_FRAGMENT_SHADER)\
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
	SHADER_PROGRAM(debug_program, debug_vertex, debug_fragment);

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
load_texture(const char *file, TextureFormat format)
{
	GLuint texture;
	int image_width, image_height;
	unsigned char *image_data;
	
	GLenum internal_format, data_format, channels;

	switch(format) {
	case TEXTURE_FORMAT_RED:
		internal_format = GL_R8;
		data_format = GL_RED;
		channels = 1;
		break;
	case TEXTURE_FORMAT_RGBA32:
		internal_format = GL_RGBA8;
		data_format = GL_RGBA;
		channels = 4;
		break;
	}

	image_data = stbi_load(file, &image_width, &image_height, NULL, channels);
	if(!image_data) {
		printf("Could not load the texture %s\n", file);
		return -1;
	}


	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D,
			0,
			internal_format,
			image_width, image_height,
			0, 
			data_format,
			GL_UNSIGNED_BYTE,
			image_data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	stbi_image_free(image_data);
	return texture;
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
draw_tmap(GraphicsTileMap *tmap) 
{
	intrend_draw_instanced(&tile_map_program, tmap->vao, GL_TRIANGLES, 6, tmap->count_tiles);
}

TextureAtlasData
load_texture_atlas(const char *path, int cols, int rows, TextureFormat format) 
{
	return (TextureAtlasData) {
		.texture = load_texture(path, format),
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

void
insert_character(TextureAtlas atlas, vec2 position, vec2 character_size, vec4 color, vec4 clip_region, int c)
{
	if(sprite_count >= MAX_SPRITES) 
		return;

	c -= 32;
	glBindBuffer(GL_ARRAY_BUFFER, sprite_buffer);
	glBufferSubData(GL_ARRAY_BUFFER, sprite_count * sizeof(Sprite), sizeof(Sprite), &(Sprite){
		.position  = { position[0], position[1] },
		.half_size = { character_size[0], character_size[1] },
		.color     = { color[0], color[1], color[2], color[3] },
		.sprite_id = {
			(float)(c % texture_atlas[atlas].cols),
			(float)(c / texture_atlas[atlas].cols)
		},
		.clip_region = { clip_region[0], clip_region[1], clip_region[2], clip_region[3] },
		.type = SPRITE_FONT_CELLPHONE,
		.rotation = 0
	});
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	sprite_count++;
}
