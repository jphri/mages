#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <assert.h>

#include "vecmath.h"
#include "glutil.h"
#include "util.h"

#include "graphics.h"
#include "third/stb_image.h"
#include "third/stb_truetype.h"

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
	LAST_VATTRIB
};

#define FBO_SCALE 4
#define MAX_SPRITES 1024

typedef struct {
	vec2 position;
	vec2 half_size;
	vec2 sprite_id;
	vec4 color;
} FontInstance;

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

typedef struct {
	vec2 position;
	vec4 color;
} DebugVertex;

typedef struct {
	TextureAtlas texture;
	GLuint instance_buffer;
	GLuint vao;
	GLuint char_count;
} FontBuffer;

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
}

static void   create_texture_buffer(int w, int h);
static void   init_shaders();
static GLuint load_texture(const char *file, TextureFormat format);

static SpriteBuffer load_sprite_buffer(TextureAtlas texture);
static void         draw_sprite_buffer(SpriteBuffer *buffer);
static void         insert_sprite_buffer(SpriteBuffer *buffer, Sprite *sprite);
static void         end_sprite_buffer(SpriteBuffer *buffer);

static TextureAtlasData load_texture_atlas(const char *file, int cols, int rows, TextureFormat format);
static void             end_texture_atlas(TextureAtlasData *texture);
static void             draw_tmap(GraphicsTileMap *tmap);
static void             draw_post(ShaderProgram *program);

static void             load_font_buffer(FontBuffer *buffer, TextureAtlas atlas); 
static void             insert_character(FontBuffer *buffer, vec2 position, vec2 size, vec4 color, int c);
static void             draw_characters(FontBuffer *buffer);
static void             end_font_buffer(FontBuffer *buffer);

static GLuint sprite_buffer_gpu;
static int screen_width, screen_height;
static ShaderProgram sprite_program, tile_map_program, font_program;
static ShaderProgram debug_program;

static SpriteBuffer sprite_buffers[LAST_SPRITE];
static TextureAtlasData texture_atlas[LAST_TEXTURE_ATLAS];
static FontBuffer font_buffers[LAST_FONT];

static GLuint albedo_fbo, albedo_texture;
static GLuint post_process_vbo, post_process_vao;

static GLuint debug_lines_buffer, debug_quad_buffer, debug_fill_quad_buffer;
static GLuint debug_lines_vao, debug_quad_vao, debug_fill_quad_vao;
static GLuint debug_lines_count, debug_quad_count, debug_fill_quad_count;
static vec4 debug_color;

static ShaderProgram post_clean;

static mat3 view_matrix;

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

	sprite_buffer_gpu = ugl_create_buffer(GL_STATIC_DRAW, sizeof(vertex_data), vertex_data);
	sprite_buffers[SPRITE_PLAYER] = load_sprite_buffer(TEXTURE_PLAYER);
	sprite_buffers[SPRITE_FIRIE] = load_sprite_buffer(TEXTURE_FIRIE);
	sprite_buffers[SPRITE_TERRAIN] = load_sprite_buffer(TERRAIN_NORMAL);

	load_font_buffer(&font_buffers[FONT_CELLPHONE], TEXTURE_FONT_CELLPHONE);

	texture_atlas[TEXTURE_PLAYER]         = load_texture_atlas("textures/player.png", 2, 1, TEXTURE_FORMAT_RGBA32);
	texture_atlas[TERRAIN_NORMAL]         = load_texture_atlas("textures/terrain.png", 16, 16, TEXTURE_FORMAT_RGBA32);
	texture_atlas[TEXTURE_FONT_CELLPHONE] = load_texture_atlas("textures/charmap-cellphone.png", 18, 7, TEXTURE_FORMAT_RED);

	post_process_vbo = ugl_create_buffer(GL_STATIC_DRAW, sizeof(post_process), post_process);
	post_process_vao = ugl_create_vao(2, (VaoSpec[]){
		{ .name = VATTRIB_POSITION, .size = 2, .type = GL_FLOAT, .stride = sizeof(SpriteVertex), .offset = offsetof(SpriteVertex, position), .buffer = post_process_vbo },
		{ .name = VATTRIB_TEXCOORD, .size = 2, .type = GL_FLOAT, .stride = sizeof(SpriteVertex), .offset = offsetof(SpriteVertex, texcoord), .buffer = post_process_vbo },
	});

	debug_lines_buffer     = ugl_create_buffer(GL_STREAM_DRAW, sizeof(DebugVertex) * 4096, NULL);
	debug_quad_buffer      = ugl_create_buffer(GL_STREAM_DRAW, sizeof(DebugVertex) * 4096, NULL);
	debug_fill_quad_buffer = ugl_create_buffer(GL_STREAM_DRAW, sizeof(DebugVertex) * 4096, NULL);

	#define CREATE_DEBUG_VAO(BUFFER) \
		ugl_create_vao(2, (VaoSpec[]) {\
			{ .name = VATTRIB_POSITION, .size = 2, .type = GL_FLOAT, .stride = sizeof(DebugVertex), .offset = offsetof(DebugVertex, position), .buffer = BUFFER },\
			{ .name = VATTRIB_COLOR,    .size = 4, .type = GL_FLOAT, .stride = sizeof(DebugVertex), .offset = offsetof(DebugVertex, color),    .buffer = BUFFER },\
		})

	debug_lines_vao = CREATE_DEBUG_VAO(debug_lines_buffer);
	debug_quad_vao = CREATE_DEBUG_VAO(debug_quad_buffer);
	debug_fill_quad_vao = CREATE_DEBUG_VAO(debug_fill_quad_buffer);
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

	for(int i = 0; i < LAST_FONT; i++)
		end_font_buffer(&font_buffers[i]);

	for(int i = 0; i < LAST_TEXTURE_ATLAS; i++)
		end_texture_atlas(&texture_atlas[i]);
}

void
gfx_draw_sprite(Sprite *sprite) 
{
	insert_sprite_buffer(&sprite_buffers[sprite->sprite_type], sprite);
}

void
gfx_draw_font(Font font, vec2 position, vec2 char_size, vec4 color, const char *fmt, ...) 
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
			insert_character(&font_buffers[font], p, char_size, color, buffer[i]);
			p[0] += char_size[0] * 2.0;
		}
	}
}

void
gfx_make_framebuffers(int w, int h) 
{
	mat3 projection;

	affine2d_ortho_window(projection, w, h);
	create_texture_buffer(w, h);

	intrend_bind_shader(&sprite_program);
	intrend_uniform_mat3(U_VIEW,       view_matrix);
	intrend_uniform_mat3(U_PROJECTION, projection);
	intrend_uniform_iv(U_IMAGE_TEXTURE, 1, 1, &(GLint){ 0 });

	intrend_bind_shader(&font_program);
	intrend_uniform_mat3(U_VIEW,       view_matrix);
	intrend_uniform_mat3(U_PROJECTION, projection);
	intrend_uniform_iv(U_IMAGE_TEXTURE, 1, 1, &(GLint){ 0 });

	intrend_bind_shader(&tile_map_program);
	intrend_uniform_mat3(U_VIEW,       view_matrix);
	intrend_uniform_mat3(U_PROJECTION, projection);
	intrend_uniform_iv(U_IMAGE_TEXTURE, 1, 1, &(GLint){ 0 });
	
	intrend_bind_shader(&debug_program);
	intrend_uniform_mat3(U_VIEW,       view_matrix);
	intrend_uniform_mat3(U_PROJECTION, projection);
	intrend_uniform_iv(U_IMAGE_TEXTURE, 1, 1, &(GLint){ 0 });
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
	glViewport(0, 0, screen_width / FBO_SCALE, screen_height / FBO_SCALE);
	if(tmap)
		draw_tmap(tmap);
}

void
gfx_draw_end()
{
	for(int i = 0; i < LAST_SPRITE; i++)
		draw_sprite_buffer(&sprite_buffers[i]);
	for(int i = 0; i < LAST_FONT; i++)
		draw_characters(&font_buffers[i]);
}

void 
gfx_debug_begin()
{
	debug_quad_count = 0;
	debug_lines_count = 0;
	debug_fill_quad_count = 0;
}

void 
gfx_debug_set_color(vec4 color)
{
	vec4_dup(debug_color, color);
}

#define DEBUG_FILL_COLOR { debug_color[0], debug_color[1], debug_color[2], debug_color[3] }
inline void pass_data_buffer(GLuint buffer, size_t offset, size_t buffer_size, void *data)
{
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferSubData(GL_ARRAY_BUFFER,
			offset * sizeof(DebugVertex),
			buffer_size,
			data);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void 
gfx_debug_line(vec2 p1, vec2 p2)
{
	DebugVertex data[] = {
		{ .position = { p1[0], p1[1] }, .color = DEBUG_FILL_COLOR },
		{ .position = { p2[0], p2[1] }, .color = DEBUG_FILL_COLOR },
	};
	pass_data_buffer(debug_lines_buffer, debug_lines_count, sizeof(data), data);
	debug_lines_count += 2;
}

void 
gfx_debug_quad(vec2 p, vec2 hs)
{
	DebugVertex data[] = {
		{ .position = { p[0] - hs[0], p[1] - hs[1] }, .color = DEBUG_FILL_COLOR },
		{ .position = { p[0] + hs[0], p[1] - hs[1] }, .color = DEBUG_FILL_COLOR },
		{ .position = { p[0] + hs[0], p[1] + hs[1] }, .color = DEBUG_FILL_COLOR },
		{ .position = { p[0] + hs[0], p[1] + hs[1] }, .color = DEBUG_FILL_COLOR },
		{ .position = { p[0] - hs[0], p[1] + hs[1] }, .color = DEBUG_FILL_COLOR },
		{ .position = { p[0] - hs[0], p[1] - hs[1] }, .color = DEBUG_FILL_COLOR },
	};
	pass_data_buffer(debug_quad_buffer, debug_quad_count, sizeof(data), data);
	debug_quad_count += 6;
}

void 
gfx_debug_fill_quad(vec2 p, vec2 hs)
{
	DebugVertex data[] = {
		{ .position = { p[0] - hs[0], p[1] - hs[1] }, .color = DEBUG_FILL_COLOR },
		{ .position = { p[0] + hs[0], p[1] - hs[1] }, .color = DEBUG_FILL_COLOR },
		{ .position = { p[0] + hs[0], p[1] + hs[1] }, .color = DEBUG_FILL_COLOR },
		{ .position = { p[0] + hs[0], p[1] + hs[1] }, .color = DEBUG_FILL_COLOR },
		{ .position = { p[0] - hs[0], p[1] + hs[1] }, .color = DEBUG_FILL_COLOR },
		{ .position = { p[0] - hs[0], p[1] - hs[1] }, .color = DEBUG_FILL_COLOR },
	};
	pass_data_buffer(debug_fill_quad_buffer, debug_fill_quad_count, sizeof(data), data);
	debug_fill_quad_count += 6;
}

void 
gfx_debug_end()
{
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	intrend_draw(&debug_program, debug_lines_vao, GL_LINES, debug_lines_count);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	intrend_draw(&debug_program, debug_quad_vao, GL_TRIANGLES, debug_quad_count);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	intrend_draw(&debug_program, debug_fill_quad_vao, GL_TRIANGLES, debug_fill_quad_count);
}

void
gfx_render_present() 
{
	draw_post(&post_clean);
}

void
gfx_set_camera(vec2 position, vec2 scale)
{
	mat3_ident(view_matrix);
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
		SHADER(font_fragment,    "shaders/font.fsh",    GL_FRAGMENT_SHADER) \
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
	SHADER_PROGRAM(font_program, default_vertex, font_fragment);

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


SpriteBuffer 
load_sprite_buffer(TextureAtlas texture)
{
	GLuint instance_buffer = ugl_create_buffer(GL_STREAM_DRAW, sizeof(Sprite) * MAX_SPRITES, NULL);
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
load_font_buffer(FontBuffer *atlas, TextureAtlas texture) 
{
	atlas->texture = texture;
	atlas->instance_buffer = ugl_create_buffer(GL_STREAM_DRAW, sizeof(FontInstance) * MAX_SPRITES, NULL);
	atlas->vao = ugl_create_vao(6, (VaoSpec[]){
		{ .name = VATTRIB_POSITION, .size = 2, .type = GL_FLOAT, .stride = sizeof(SpriteVertex), .offset = offsetof(SpriteVertex, position), .buffer = sprite_buffer_gpu },
		{ .name = VATTRIB_TEXCOORD, .size = 2, .type = GL_FLOAT, .stride = sizeof(SpriteVertex), .offset = offsetof(SpriteVertex, texcoord), .buffer = sprite_buffer_gpu },

		{ .name = VATTRIB_INST_POSITION,  .size = 2, .type = GL_FLOAT, .stride = sizeof(FontInstance), .offset = offsetof(FontInstance, position),  .divisor = 1, .buffer = atlas->instance_buffer },
		{ .name = VATTRIB_INST_SIZE,      .size = 2, .type = GL_FLOAT, .stride = sizeof(FontInstance), .offset = offsetof(FontInstance, half_size), .divisor = 1, .buffer = atlas->instance_buffer },
		{ .name = VATTRIB_INST_SPRITE_ID, .size = 2, .type = GL_FLOAT, .stride = sizeof(FontInstance), .offset = offsetof(FontInstance, sprite_id), .divisor = 1, .buffer = atlas->instance_buffer },
		{ .name = VATTRIB_INST_COLOR,     .size = 4, .type = GL_FLOAT, .stride = sizeof(FontInstance), .offset = offsetof(FontInstance, color),     .divisor = 1, .buffer = atlas->instance_buffer },
	});
	atlas->char_count = 0;
}

void
insert_character(FontBuffer *buffer, vec2 position, vec2 character_size, vec4 color, int c)
{
	if(buffer->char_count >= MAX_SPRITES) 
		return;

	c -= 32;
	glBindBuffer(GL_ARRAY_BUFFER, buffer->instance_buffer);
	glBufferSubData(GL_ARRAY_BUFFER, buffer->char_count * sizeof(FontInstance), sizeof(FontInstance), &(FontInstance){
		.position  = { position[0], position[1] },
		.half_size = { character_size[0], character_size[1] },
		.color     = { color[0], color[1], color[2], color[3] },
		.sprite_id = {
			(float)(c % texture_atlas[buffer->texture].cols),
			(float)(c / texture_atlas[buffer->texture].cols)
		}
	});
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	buffer->char_count++;
}

void
draw_characters(FontBuffer *buffer) 
{
	if(buffer->char_count == 0)
		return;

	intrend_bind_shader(&font_program);
	intrend_uniform_fv(U_SPRITE_CR, 1, 2, (float[]){ 
		texture_atlas[buffer->texture].cols,
		texture_atlas[buffer->texture].rows
	});

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_atlas[buffer->texture].texture);
	intrend_draw_instanced(&font_program, buffer->vao, GL_TRIANGLES, 6, buffer->char_count);
	glBindTexture(GL_TEXTURE_2D, 0);
	buffer->char_count = 0;
}

void
end_font_buffer(FontBuffer *buffer)
{
	glDeleteVertexArrays(1, &buffer->vao);
	glDeleteBuffers(1, &buffer->instance_buffer);
}
