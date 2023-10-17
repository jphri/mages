#include <SDL2/SDL.h>
#include <GL/glew.h>

#include "vecmath.h"
#include "glutil.h"
#include "util.h"

#include "graphics.h"

enum Uniform {
	U_VIEW,
	LAST_UNIFORM
};

enum VertexAttrib {
	VATTRIB_POSITION,
	VATTRIB_TEXCOORD,
	VATTRIB_COLOR,
	LAST_VATTRIB
};

typedef struct {
	vec2 position;
	vec2 texcoord;
	vec4 color;
} SpriteVertex;

#include "base-renderer.h"

void
intrend_bind_uniforms(ShaderProgram *shader)
{
	intrend_uniform_bind(shader, U_VIEW, "u_View");
}

void
intrend_bind_attribs(ShaderProgram *shader) 
{
	intrend_attrib_bind(shader, VATTRIB_POSITION, "v_Position");
	intrend_attrib_bind(shader, VATTRIB_TEXCOORD, "v_Texcoord");
	intrend_attrib_bind(shader, VATTRIB_COLOR,    "v_Color");
}


static void init_shaders();

static SDL_GLContext context;
static GLuint sprite_buffer_gpu, sprite_vao, sprite_count;
static ShaderProgram sprite_program;

void
gfx_init()
{
	SpriteVertex vertex_data[] = {
		{ .position = { -1.0, -1.0 }, .texcoord = { 0.0, 0.0 }, .color = { 1.0, 0.0, 0.0, 1.0 } },
		{ .position = {  1.0, -1.0 }, .texcoord = { 1.0, 0.0 }, .color = { 0.0, 1.0, 0.0, 1.0 } },
		{ .position = {  1.0,  1.0 }, .texcoord = { 1.0, 1.0 }, .color = { 0.0, 0.0, 1.0, 1.0 } },
		{ .position = {  1.0,  1.0 }, .texcoord = { 1.0, 1.0 }, .color = { 0.0, 0.0, 1.0, 1.0 } },
		{ .position = { -1.0,  1.0 }, .texcoord = { 0.0, 1.0 }, .color = { 1.0, 0.0, 1.0, 1.0 } },
		{ .position = { -1.0, -1.0 }, .texcoord = { 0.0, 0.0 }, .color = { 1.0, 0.0, 0.0, 1.0 } },
	};
	init_shaders();

	sprite_buffer_gpu = ugl_create_buffer(GL_STATIC_DRAW, sizeof(vertex_data), vertex_data);
	sprite_vao        = ugl_create_vao(3, (VaoSpec[]){
		{ .name = VATTRIB_POSITION, .size = 2, .type = GL_FLOAT, .stride = sizeof(SpriteVertex), .offset = offsetof(SpriteVertex, position), .buffer = sprite_buffer_gpu },
		{ .name = VATTRIB_TEXCOORD, .size = 2, .type = GL_FLOAT, .stride = sizeof(SpriteVertex), .offset = offsetof(SpriteVertex, texcoord), .buffer = sprite_buffer_gpu },
		{ .name = VATTRIB_COLOR,    .size = 4, .type = GL_FLOAT, .stride = sizeof(SpriteVertex), .offset = offsetof(SpriteVertex, color),    .buffer = sprite_buffer_gpu },
	});
}

void
gfx_end()
{
	glDeleteProgram(sprite_program.program);
}

void
gfx_draw_sprite(Sprite *sprite) 
{
}

void
gfx_render(int w, int h) 
{
	mat3 view;
	mat3_ident(view);

	intrend_bind_shader(&sprite_program);
	intrend_uniform_mat3(U_VIEW, view);
	intrend_draw(&sprite_program, sprite_vao, GL_TRIANGLES, 6);
}

void
init_shaders()
{
	#define SHADER_PROGRAM(symbol, ...) \
		intrend_link(&symbol, #symbol, sizeof((GLuint[]){ __VA_ARGS__ })/sizeof(GLuint), (GLuint[]){ __VA_ARGS__ })

	GLuint default_vertex   = ugl_compile_shader_file("shaders/default.vsh", GL_VERTEX_SHADER);
	GLuint default_fragment = ugl_compile_shader_file("shaders/default.fsh", GL_FRAGMENT_SHADER);

	SHADER_PROGRAM(sprite_program, default_vertex, default_fragment);

	glDeleteShader(default_vertex);
	glDeleteShader(default_fragment);

	#undef SHADER_PROGRAM
}

