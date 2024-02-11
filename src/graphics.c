#include <SDL.h>
#include <glad/gles2.h>
#include <assert.h>
#include <stb_image.h>
#include <string.h>

#include "SDL_stdinc.h"
#include "game_objects.h"
#include "vecmath.h"
#include "glutil.h"
#include "util.h"

#include "graphics.h"

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
	VATTRIB_INST_TEXTURE_POSITION,
	VATTRIB_INST_TEXTURE_SIZE,
	VATTRIB_INST_SPRITE_TYPE,
	VATTRIB_INST_CLIP,
	LAST_VATTRIB
};

#define FBO_SCALE 4

typedef struct {
	vec2 position;
	vec2 texcoord;
} SpriteVertex;

typedef struct {
	float rotation;
	GLuint type;

	vec2 position;
	vec2 half_size;
	vec2 texpos;
	vec2 texsize;
	vec4 color;
	vec4 clip_region;
} SpriteInternal;

typedef struct {
	vec2 position;
	vec2 half_size;
} ClipStack;

typedef struct {
	GLuint texture;
	int width, height;
} TextureData;

typedef struct {
	Texture texture;
	int rows, cols;
} SpriteAtlasData;

typedef struct {
	vec2 position;
	vec2 tile_data;
} TileData;

typedef struct {
	vec2 position;
	vec4 color;
} DebugVertex;

typedef struct {
	int count_chars;
	Texture texture;
	int baseline;
	int line_offset;
	struct CharData {
		int char_id;
		int width, height;
		int char_x, char_y;
		int x_advance;
		int x_offset, y_offset;
	} *data;
} FontData;

typedef struct {
	vec2 size; 
} FontSizeParser;

typedef struct {
	float height;
	vec2 position;
	vec4 color;
} FontRenderParser;

typedef enum {
	TEXTURE_FORMAT_RGBA32,
	TEXTURE_FORMAT_RED,
} TextureFormat;

typedef enum {
	TEXTURE_FILTER_NEAREST,
	TEXTURE_FILTER_LINEAR
} TextureFilter;

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

	intrend_attrib_bind(shader, VATTRIB_INST_POSITION,         "v_InstPosition");
	intrend_attrib_bind(shader, VATTRIB_INST_SIZE,             "v_InstSize");
	intrend_attrib_bind(shader, VATTRIB_INST_TEXTURE_POSITION, "v_InstTexPosition");
	intrend_attrib_bind(shader, VATTRIB_INST_TEXTURE_SIZE,     "v_InstTexSize");
	intrend_attrib_bind(shader, VATTRIB_INST_SPRITE_ID,        "v_InstSpriteID");
	intrend_attrib_bind(shader, VATTRIB_INST_ROTATION,         "v_InstRotation");
	intrend_attrib_bind(shader, VATTRIB_INST_COLOR,            "v_InstColor");
	intrend_attrib_bind(shader, VATTRIB_INST_SPRITE_TYPE,      "v_InstSpriteType");
	intrend_attrib_bind(shader, VATTRIB_INST_CLIP,             "v_ClipRegion");
}

static void   create_texture_buffer(int w, int h);
static void   init_shaders(void);
static int    load_texture(TextureData *tex, const char *file, TextureFormat format, TextureFilter filter);

static void             end_texture_atlas(TextureData *texture);
static void             draw_tmap(GraphicsTileMap *tmap);
static void             draw_post(ShaderProgram *program);

static void             load_font_info(Font font, Texture texture, const char *path);

static struct CharData  *find_char_idx(Font font, int charid);

static void parse_buffer(StrView buffer_view, Font font, void *user_data, void (*cbk)(struct CharData *, Font font, vec2 char_offset, void *user_data));
static void parser_font_size(struct CharData *, Font font, vec2 char_offset, void *parser);
static void parser_font_render(struct CharData *, Font font, vec2 char_offset, void *parser);

static void sprite_buffer_reserve(int count_sprites);
static void sprite_insert(int count_sprites, SpriteInternal *spr_buf);

static bool enabled_camera;

static GLuint sprite_buffer_gpu;
static int screen_width, screen_height;
static ShaderProgram sprite_program, tile_map_program, font_program;
static ShaderProgram debug_program;

static TextureData texture_atlas[LAST_TEXTURE_ATLAS];
static FontData font_data[LAST_FONT];
static SpriteAtlasData sprite_atlas[LAST_SPRITE] =
{
	[SPRITE_TERRAIN] = {
		.texture = TERRAIN_NORMAL,
		.rows = 16, .cols = 16
	},
	[SPRITE_ENTITIES] = {
		.texture = TEXTURE_ENTITIES,
		.rows = 16, .cols = 16
	},
	[SPRITE_UI] = {
		.texture = TEXTURE_UI,
		.rows = 32, .cols = 32
	},
	[SPRITE_FONT_CELLPHONE] = {
		.texture = TEXTURE_FONT_CELLPHONE,
		.rows = 16, .cols = 16
	}
};

static GLuint albedo_fbo, albedo_texture;
static GLuint post_process_vbo, post_process_vao;

static ShaderProgram post_clean;

static mat4 projection;
static mat4 view_matrix;

static GLuint matrix_buffer;
static GLuint sprite_colrow_inv_buffer;
static GraphicsTileMap *current_tmap;

static GLuint sprite_buffer, sprite_vao, sprite_count, 
			  sprite_reserved = 1; /* for initialization, this will be overrided at gfx_init() */
static mat4 ident_mat;

static int clip_id;
static Rectangle clip_stack[1024];

static inline void get_global_clip(vec4 out_clip)
{
	Rectangle rect = clip_stack[0];
	for(int i = 1; i < clip_id; i++) {
		rect_intersect(&rect, &rect, &clip_stack[i]);
	}
	out_clip[0] = rect.position[0];
	out_clip[1] = rect.position[1];
	out_clip[2] = rect.half_size[0];
	out_clip[3] = rect.half_size[1];
}

void
gfx_init(void)
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

	load_font_info(FONT_ROBOTO, TEXTURE_FONT_ROBOTO, "fonts/bmfiles/roboto-slab.fnt");

	sprite_buffer_gpu               = ugl_create_buffer(GL_STATIC_DRAW, sizeof(vertex_data), vertex_data);

	load_texture(&texture_atlas[TEXTURE_ENTITIES],       "textures/entities.png",            TEXTURE_FORMAT_RGBA32, TEXTURE_FILTER_NEAREST);
	load_texture(&texture_atlas[TERRAIN_NORMAL],         "textures/terrain.png",             TEXTURE_FORMAT_RGBA32, TEXTURE_FILTER_NEAREST);
	load_texture(&texture_atlas[TEXTURE_FONT_CELLPHONE], "textures/charmap-cellphone.png",   TEXTURE_FORMAT_RGBA32, TEXTURE_FILTER_NEAREST);
	load_texture(&texture_atlas[TEXTURE_UI],             "textures/ui.png",                  TEXTURE_FORMAT_RGBA32, TEXTURE_FILTER_NEAREST);
	load_texture(&texture_atlas[TEXTURE_FONT_ROBOTO],    "fonts/textures/roboto-slab_0.png", TEXTURE_FORMAT_RGBA32, TEXTURE_FILTER_LINEAR);

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
		#define TEX(TEXTURE_ATLAS) { 1.0 / sprite_atlas[TEXTURE_ATLAS].cols, 1.0 / sprite_atlas[TEXTURE_ATLAS].rows }
		TEX(SPRITE_ENTITIES),
		{0},
		TEX(SPRITE_TERRAIN),
		{0},
		TEX(SPRITE_FONT_CELLPHONE),
		{0},
		TEX(SPRITE_UI),
		{0},
		#undef TEX
	};
	glGenBuffers(1, &sprite_colrow_inv_buffer);
	glBindBuffer(GL_UNIFORM_BUFFER, sprite_colrow_inv_buffer);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	mat4_ident(projection);
	mat4_ident(view_matrix);

	sprite_count = 0;
	sprite_reserved = 1;
	sprite_buffer_reserve(1024);

	mat4_ident(ident_mat);
}

void
gfx_end(void)
{
	glDeleteProgram(sprite_program.program);
	glDeleteBuffers(2, (GLuint[]) {
		sprite_buffer_gpu,
		sprite_buffer,
	});

	for(int i = 0; i < LAST_TEXTURE_ATLAS; i++)
		end_texture_atlas(&texture_atlas[i]);
}

void 
gfx_draw_texture_rect(TextureStamp *stamp, vec2 position, vec2 size, float rotation, vec4 color)
{
	SpriteInternal internal;

	internal.type                  = stamp->texture;
	internal.rotation              = rotation;
	vec2_dup(internal.position,    position);
	vec2_dup(internal.half_size,   size);
	vec2_dup(internal.texpos,      stamp->position);
	vec2_dup(internal.texsize,     stamp->size);
	vec4_dup(internal.color,       color);
	get_global_clip(internal.clip_region);

	sprite_insert(1, &internal);
}

void
gfx_draw_font2(Font font, vec2 position, float height, vec4 color, const char *fmt, ...)
{
	char buffer[1024];
	va_list va;
	int characters;
	(void)font;

	va_start(va, fmt);
	characters = vsnprintf(buffer, sizeof(buffer), fmt, va);
	va_end(va);

	FontRenderParser render;

	vec4_dup(render.color, color);
	vec2_dup(render.position, position);
	render.height = height;

	parse_buffer(to_strview_buffer(buffer, characters), font, &render, parser_font_render);
}

void
gfx_draw_font(Font font, vec2 position, float height, vec4 color, StrView str)
{
	FontRenderParser render;

	vec4_dup(render.color, color);
	vec2_dup(render.position, position);
	render.height = height;

	parse_buffer(str, font, &render, parser_font_render);
}

void
gfx_draw_line(vec2 p1, vec2 p2, float thickness, vec4 color)
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

	gfx_draw_texture_rect(
		&(TextureStamp){ .texture = TEXTURE_UI, .position = { 0.0, 0.0 }, .size = { 0.001, 0.001 } },
		sprite_pos,
		sprite_size,
		rotation,
		color
	);
}

void
gfx_draw_rect(vec2 position, vec2 half_size, float thickness, vec4 color)
{
	vec2 min, max;
	vec2_sub(min, position, half_size);
	vec2_add(max, position, half_size);
	
	#define ADD_LINE(x1, y1, x2, y2) gfx_draw_line((vec2){ x1, y1 }, (vec2){ x2, y2 }, thickness, color)
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

	clip_stack[0].position[0] =  w / 2.0;
	clip_stack[0].position[1] =  h / 2.0;
	clip_stack[0].half_size[0] = w / 2.0;
	clip_stack[0].half_size[1] = h / 2.0;

	glBindBuffer(GL_ARRAY_BUFFER, matrix_buffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(projection), projection);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void
gfx_clear(void)
{
	glClearColor(0.2, 0.3, 0.7, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
}

void
gfx_setup_draw_framebuffers(void)
{
	glBindFramebuffer(GL_FRAMEBUFFER, albedo_fbo);
	glViewport(0, 0, screen_width / FBO_SCALE, screen_height / FBO_SCALE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void
gfx_end_draw_framebuffers(void)
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

	clip_id = 1;
}

void
gfx_draw_end(void)
{
	if(!current_tmap && sprite_count == 0)
		return;

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
gfx_render_present(void) 
{
	draw_post(&post_clean);
}

void
gfx_set_camera(vec2 position, vec2 scale)
{
	mat4_ident(view_matrix);
	affine2d_scale(view_matrix,     scale);
	affine2d_translate(view_matrix, position);

	glBindBuffer(GL_UNIFORM_BUFFER, matrix_buffer);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(projection), sizeof(view_matrix), 
			enabled_camera ? view_matrix : ident_mat);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void
gfx_camera_set_enabled(bool enabled)
{
	enabled_camera = enabled;

	glBindBuffer(GL_UNIFORM_BUFFER, matrix_buffer);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(projection), sizeof(view_matrix), 
			enabled_camera ? view_matrix : ident_mat);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void
gfx_pixel_to_world(vec2 pixel, vec2 world_out) 
{
	world_out[0] = (pixel[0] - view_matrix[3][0]) / view_matrix[0][0];
	world_out[1] = (pixel[1] - view_matrix[3][1]) / view_matrix[1][1];
}

void
init_shaders(void)
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
			GL_RGBA,
			screen_width / FBO_SCALE, screen_height / FBO_SCALE,
			0, 
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
	
	glBindFramebuffer(GL_FRAMEBUFFER, albedo_fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, albedo_texture, 0);

	GLenum state = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	switch(state) {
	#define S(STATUS) case STATUS: printf("framebuffer status " #STATUS "\n"); break;
	S(GL_FRAMEBUFFER_UNDEFINED);
	S(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT);
	S(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT);
	S(GL_FRAMEBUFFER_UNSUPPORTED);
	S(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE);
	S(GL_FRAMEBUFFER_COMPLETE);
	#undef S
	}
	glDrawBuffers(1, (GLenum[]){ GL_COLOR_ATTACHMENT0 });
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GraphicsTileMap
gfx_tmap_new(SpriteType terrain, int w, int h, int *data) 
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
					(int)(spr % sprite_atlas[terrain].cols),
					(int)(spr / sprite_atlas[terrain].cols)
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

int
load_texture(TextureData *texture, const char *path, TextureFormat format, TextureFilter filter) 
{
	unsigned char *image_data;
	
	GLenum internal_format, data_format, channels;
	GLenum internal_filter;

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

	switch(filter) {
	case TEXTURE_FILTER_LINEAR: internal_filter = GL_LINEAR; break;
	case TEXTURE_FILTER_NEAREST: internal_filter = GL_NEAREST; break;
	default:
		die("filter not found: %d\n", filter);
	}

	image_data = stbi_load(path, &texture->width, &texture->height, NULL, channels);
	if(!image_data) {
		printf("Could not load the texture %s\n", path);
		return -1;
	}

	glGenTextures(1, &texture->texture);
	glBindTexture(GL_TEXTURE_2D, texture->texture);
	glTexImage2D(GL_TEXTURE_2D,
			0,
			internal_format,
			texture->width, texture->height,
			0, 
			data_format,
			GL_UNSIGNED_BYTE,
			image_data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, internal_filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, internal_filter);
	glBindTexture(GL_TEXTURE_2D, 0);

	stbi_image_free(image_data);
	return 0;
}

void
end_texture_atlas(TextureData *terrain)
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

typedef struct {
	StrView name, value;
} Parameter;

static int read_parameter(StrView *view, Parameter *param)
{
	param->name = strview_token(view, "=");
	param->value = strview_token(view, "=");
	return param->name.begin != param->name.end && param->value.begin != param->value.end;
}

static int compare_chardata(const void *a, const void *b)
{
	struct CharData *c1 = (struct CharData*)a;
	struct CharData *c2 = (struct CharData*)b;
	return c1->char_id - c2->char_id;
}

static void
load_font_info(Font font, Texture font_texture, const char *path)
{
	FileBuffer file;
	int idx = 0;
	fbuf_open(&file, path, "r", allocator_default());

	while(fbuf_read_line(&file, '\n') != EOF) {
		StrView line = to_strview_buffer(fbuf_data(&file), fbuf_data_size(&file)),
		        ldup = line;
		Parameter param;

		StrView first = strview_token(&ldup, " ");
		if(strview_cmp(first, "chars") == 0) {
			while(read_parameter(&ldup, &param)) {
				if(strview_cmp(param.name, "count") == 0) {
					strview_int(param.value, &font_data[font].count_chars);
					font_data[font].data = emalloc(sizeof(font_data[font].data[0]) * font_data[font].count_chars);
				}
			}
		} else if(strview_cmp(first, "char") == 0) {
			StrView param_raw = strview_token(&ldup, " "),
				    param_dup = param_raw;
			
			while(read_parameter(&param_dup, &param)) {
				if(strview_cmp(param.name, "id") == 0)
					strview_int(param.value, &font_data[font].data[idx].char_id);
				else if(strview_cmp(param.name, "x") == 0)
					strview_int(param.value, &font_data[font].data[idx].char_x);
				else if(strview_cmp(param.name, "y") == 0)
					strview_int(param.value, &font_data[font].data[idx].char_y);
				else if(strview_cmp(param.name, "width") == 0)
					strview_int(param.value, &font_data[font].data[idx].width);
				else if(strview_cmp(param.name, "height") == 0)
					strview_int(param.value, &font_data[font].data[idx].height);
				else if(strview_cmp(param.name, "xoffset") == 0)
					strview_int(param.value, &font_data[font].data[idx].x_offset);
				else if(strview_cmp(param.name, "yoffset") == 0)
					strview_int(param.value, &font_data[font].data[idx].y_offset);
				else if(strview_cmp(param.name, "xadvance") == 0)
					strview_int(param.value, &font_data[font].data[idx].x_advance);
				
				param_raw = strview_token(&ldup, " ");
				param_dup = param_raw;
			}
			idx ++;
		} else if(strview_cmp(first, "common") == 0) {
			StrView param_raw = strview_token(&ldup, " "),
					param_dup = param_raw;

			while(read_parameter(&param_dup, &param)) {
				if(strview_cmp(param.name, "lineHeight") == 0)
					strview_int(param.value, &font_data[font].line_offset);
				else if(strview_cmp(param.name, "base") == 0)
					strview_int(param.value, &font_data[font].baseline);
				
				param_raw = strview_token(&ldup, " ");
				param_dup = param_raw;
			}

		}
	}
	fbuf_close(&file);
	/* keep it ordered for binary search, please */
	SDL_qsort(&font_data[font].data[0], font_data[font].count_chars, sizeof(font_data[font].data[0]), compare_chardata);
	font_data[font].texture = font_texture;
}

static struct CharData *find_char_idx_int(Font font, int charid, Span r)
{
	int size = (struct CharData *)r.end - (struct CharData *)r.begin;
	struct CharData *middle = (struct CharData*)r.begin + (size / 2);

	if(size <= 0) {
		return NULL;
	}

	if(middle->char_id == charid) {
		return middle;
	} else if(charid < middle->char_id) {
		return find_char_idx_int(font, charid, (Span){
			.begin = r.begin,
			.end = middle
		});
	} else if(charid > middle->char_id) {
		return find_char_idx_int(font, charid, (Span){
			.begin = middle + 1,
			.end = r.end,
		});
	}

	return NULL;
}

static struct CharData *
find_char_idx(Font font, int charid)
{
	return find_char_idx_int(font, charid, (Span){
		font_data[font].data,
		font_data[font].data + font_data[font].count_chars
	});
}

TextureStamp 
get_sprite(SpriteType sprite, int sprite_x, int sprite_y)
{
	const float dx = 1.0 / sprite_atlas[sprite].cols;
	const float dy = 1.0 / sprite_atlas[sprite].rows;
	const float xx = sprite_x * dx;
	const float yy = sprite_y * dy;

	return (TextureStamp) {
		.texture = sprite_atlas[sprite].texture,
		.position = {
			xx, yy
		},
		.size = {
			dx, dy
		}
	};
}

TextureStamp *
gfx_white_texture(void)
{
	static TextureStamp tmp = { 
		.texture = TEXTURE_UI, 
		.position = { 0.0, 0.0 },
		{ 1.0/32.0, 1.0/32.0 } 
	};
	return &tmp;
}

void
gfx_push_clip(vec2 p, vec2 v)
{
	vec2_dup(clip_stack[clip_id].position, p);
	vec2_dup(clip_stack[clip_id].half_size, v);
	clip_id++;
}

void
gfx_pop_clip(void)
{
	clip_id--;
}

Rectangle
gfx_window_rectangle(void)
{
	return clip_stack[0];
}

void 
gfx_font_size(vec2 out_size, Font font, float height, const char *fmt, ...)
{
	char buffer[1024];
	
	int buffer_size;
	out_size[0] = 0;
	out_size[1] = 0;
	
	FontSizeParser parser = {0};
	
	va_list va;
	va_start(va, fmt);
	buffer_size = vsnprintf(buffer, sizeof(buffer), fmt, va);
	va_end(va);

	parse_buffer(to_strview_buffer(buffer, buffer_size), font, &parser, parser_font_size);
	vec2_add_scaled(out_size, out_size, parser.size, height * 0.5);
}

void 
gfx_font_size_view(vec2 out_size, Font font, float height, StrView view)
{
	out_size[0] = 0;
	out_size[1] = 0;
	
	FontSizeParser parser = {0};

	parse_buffer(view, font, &parser, parser_font_size);
	vec2_add_scaled(out_size, out_size, parser.size, height * 0.5);
}

void
parse_buffer(StrView buffer, Font font, void *user_data, void (*cbk)(struct CharData *, Font font, vec2 char_offset, void *user_data))
{
	struct CharData *char_data;
	vec2 textoff = { 0, -(font_data[font].line_offset - font[font_data].baseline) };
	vec2 char_off;

	for(;buffer.begin < buffer.end;) {
		int code = utf8_decode(buffer);
		switch(code) {
		case '\n':
			textoff[0] = 0.0;
			textoff[1] += font_data[font].line_offset;
			break;
		default:
			char_data = find_char_idx(FONT_ROBOTO, code);
			if(!char_data) {
				wprintf(L"cannot print character: %d (%c)\n", code, code);
				goto next_character;
			}

			vec2_add_scaled(char_off, (vec2){ 0.0, 0.0 }, textoff, 1.0);
			vec2_add_scaled(char_off, char_off, (vec2){ char_data->x_offset, char_data->y_offset }, 1.0);

			cbk(char_data, font, char_off, user_data);
			textoff[0] += char_data->x_advance;
		}
next_character:
		utf8_advance(&buffer);
	}
}

static void 
parser_font_size(struct CharData *c, Font font, vec2 char_offset, void *parser)
{
	(void)font;
	FontSizeParser *p = parser;
	float char_x = char_offset[0] + c->x_advance;
	float char_y = char_offset[1] + c->height;

	if(char_x > p->size[0]) {
		p->size[0] = char_x;
	}
	
	if(char_y > p->size[1]) {
		p->size[1] = char_y;
	}
}


void 
parser_font_render(struct CharData *char_data, Font font, vec2 char_offset, void *parser)
{
	FontRenderParser *p = parser;
	Texture atlas = font_data[font].texture;

	vec2 pp, ss;

	ss[0] = char_data->width  * p->height * 0.5;
	ss[1] = char_data->height * p->height * 0.5;

	vec2_add(pp, p->position, ss);
	vec2_add_scaled(pp, pp, char_offset, p->height);

	gfx_draw_texture_rect(
		&(TextureStamp){ 
			.texture = atlas, 
			.position = {
				char_data->char_x / (float)texture_atlas[atlas].width,
				char_data->char_y / (float)texture_atlas[atlas].height,
			},
			.size =  {
				char_data->width  / (float)texture_atlas[atlas].width,
				char_data->height / (float)texture_atlas[atlas].height,
			}
		}, 
		pp, 
		ss, 
		0.0, 
		p->color
	);
}

void
gfx_sprite_count_rows_cols(SpriteType type, int *rows, int *cols)
{
	*rows = sprite_atlas[type].rows;
	*cols = sprite_atlas[type].cols;
}

static void
sprite_buffer_reserve(int count_sprites)
{
	long new_reserv = sprite_reserved;
	GLuint new_buffer;

	while((sprite_count + count_sprites) > new_reserv)
		new_reserv = new_reserv * 2;

	if(new_reserv == sprite_reserved)
		return;

	new_buffer = ugl_create_buffer(GL_STREAM_DRAW, sizeof(SpriteInternal) * new_reserv, NULL);
	if(sprite_count > 0) {
		glBindBuffer(GL_COPY_READ_BUFFER, sprite_buffer);
		glBindBuffer(GL_COPY_WRITE_BUFFER, new_buffer);
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, sprite_count * sizeof(SpriteInternal));
		glBindBuffer(GL_COPY_READ_BUFFER, 0);
		glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
	}

	sprite_reserved = new_reserv;
	sprite_buffer = new_buffer;
	sprite_vao = ugl_create_vao(10, (VaoSpec[]){
		{ .name = VATTRIB_POSITION, .size = 2, .type = GL_FLOAT, .stride = sizeof(SpriteVertex),   .offset = offsetof(SpriteVertex,   position), .buffer = sprite_buffer_gpu },
		{ .name = VATTRIB_TEXCOORD, .size = 2, .type = GL_FLOAT, .stride = sizeof(SpriteVertex),   .offset = offsetof(SpriteVertex,   texcoord), .buffer = sprite_buffer_gpu },

		{ .name = VATTRIB_INST_SPRITE_TYPE,      .size = 1, .type = GL_UNSIGNED_INT, .stride = sizeof(SpriteInternal), .offset = offsetof(SpriteInternal, type),        .divisor = 1, .buffer = sprite_buffer },
		{ .name = VATTRIB_INST_ROTATION,         .size = 1, .type = GL_FLOAT,        .stride = sizeof(SpriteInternal), .offset = offsetof(SpriteInternal, rotation),    .divisor = 1, .buffer = sprite_buffer },
		{ .name = VATTRIB_INST_POSITION,         .size = 2, .type = GL_FLOAT,        .stride = sizeof(SpriteInternal), .offset = offsetof(SpriteInternal, position),    .divisor = 1, .buffer = sprite_buffer },
		{ .name = VATTRIB_INST_SIZE,             .size = 2, .type = GL_FLOAT,        .stride = sizeof(SpriteInternal), .offset = offsetof(SpriteInternal, half_size),   .divisor = 1, .buffer = sprite_buffer },
		{ .name = VATTRIB_INST_TEXTURE_POSITION, .size = 2, .type = GL_FLOAT,        .stride = sizeof(SpriteInternal), .offset = offsetof(SpriteInternal, texpos),      .divisor = 1, .buffer = sprite_buffer },
		{ .name = VATTRIB_INST_TEXTURE_SIZE,     .size = 2, .type = GL_FLOAT,        .stride = sizeof(SpriteInternal), .offset = offsetof(SpriteInternal, texsize),     .divisor = 1, .buffer = sprite_buffer },
		{ .name = VATTRIB_INST_COLOR,            .size = 4, .type = GL_FLOAT,        .stride = sizeof(SpriteInternal), .offset = offsetof(SpriteInternal, color),       .divisor = 1, .buffer = sprite_buffer },
		{ .name = VATTRIB_INST_CLIP,             .size = 4, .type = GL_FLOAT,        .stride = sizeof(SpriteInternal), .offset = offsetof(SpriteInternal, clip_region), .divisor = 1, .buffer = sprite_buffer },
	});
}

static void
sprite_insert(int count_sprites, SpriteInternal *spr)
{
	sprite_buffer_reserve(count_sprites);
	glBindBuffer(GL_ARRAY_BUFFER, sprite_buffer);
	glBufferSubData(GL_ARRAY_BUFFER, sprite_count * sizeof(spr[0]), count_sprites * sizeof(spr[0]), spr);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	sprite_count += count_sprites;
}
