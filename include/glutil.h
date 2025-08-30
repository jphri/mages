#ifndef GLUTIL_H
#define GLUTIL_H

typedef struct {
	GLuint name;
	GLuint size;
	GLuint type;
	GLuint stride;
	GLuint offset;
	GLuint divisor;
	GLuint buffer;
} VaoSpec;

GLuint ugl_compile_shader(const char *shader_name, GLenum shader_type, GLint size, const char source[size]);
void   ugl_link_program(GLuint program, const char *program_name, GLsizei size, GLuint shaders[size]);
GLuint ugl_create_buffer(GLenum usage, GLuint size, void *data);

GLuint ugl_compile_shader_file(const char *file_path, GLenum shader_type);
GLuint ugl_create_vao(GLuint n_specs, VaoSpec specs[n_specs]);

void   ugl_draw(GLuint program, GLuint vao, GLenum type, GLuint vert);
void   ugl_draw_instanced(GLuint program, GLuint vao, GLenum type, GLuint vert, GLuint n_inst);
void   ugl_draw_specs(GLuint program, GLuint n_specs, VaoSpec specs[n_specs], GLenum type, GLuint vert);

#endif
