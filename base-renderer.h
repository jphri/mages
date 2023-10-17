typedef enum Uniform Uniform;
typedef enum VertexAttrib VertexAttrib;

typedef struct {
	GLuint program;
	GLint  uniforms[LAST_UNIFORM];
} ShaderProgram;

static ShaderProgram *current_shader;

static void intrend_bind_uniforms(ShaderProgram *shader);
static void intrend_bind_attribs(ShaderProgram *shader);

static inline void intrend_uniform_bind(ShaderProgram *shader, Uniform uniform, const char *name) {
	shader->uniforms[uniform] = glGetUniformLocation(shader->program, name);
}

static void intrend_attrib_bind(ShaderProgram *shader, VertexAttrib attrib, const char *name) {
	glBindAttribLocation(shader->program, attrib, name);
}

static inline void intrend_link(ShaderProgram *shader, const char *name, GLsizei size, GLuint shaders[size]) {
	shader->program = glCreateProgram();
	
	intrend_bind_attribs(shader);
	ugl_link_program(shader->program, name, size, shaders);
	intrend_bind_uniforms(shader);
}

static inline void intrend_bind_shader(ShaderProgram *shader) {
	current_shader = shader;
	glUseProgram(shader->program);
}

static inline void intrend_uniform_mat3(Uniform uniform, mat3 const mat) {
	glUniformMatrix3fv(current_shader->uniforms[uniform], 1, GL_FALSE, &mat[0][0]);
}

static inline void intrend_uniform_fv(Uniform uniform, size_t count, size_t size, const float *f) {
	switch(size) {
	case 1: glUniform1fv(current_shader->uniforms[uniform], count, f); break;
	case 2: glUniform2fv(current_shader->uniforms[uniform], count, f); break;
	case 3: glUniform3fv(current_shader->uniforms[uniform], count, f); break;
	case 4: glUniform4fv(current_shader->uniforms[uniform], count, f); break;
	default:
		assert(0 && "size can be from 1 to 4.");
	}
}

static inline void intrend_uniform_iv(Uniform uniform, size_t count, size_t size, const GLint *f) {
	switch(size) {
	case 1: glUniform1iv(current_shader->uniforms[uniform], count, f); break;
	case 2: glUniform2iv(current_shader->uniforms[uniform], count, f); break;
	case 3: glUniform3iv(current_shader->uniforms[uniform], count, f); break;
	case 4: glUniform4iv(current_shader->uniforms[uniform], count, f); break;
	default:
		assert(0 && "size can be from 1 to 4.");
	}
}

static inline void intrend_draw(ShaderProgram *program, GLuint vao, GLenum type, GLsizei num_vertex) {
	ugl_draw(program->program, vao, type, num_vertex);
}

static inline void intrend_draw_instanced(ShaderProgram *program, GLuint vao, GLenum type, GLsizei num_vertex, GLsizei n_inst) {
	ugl_draw_instanced(program->program, vao, type, num_vertex, n_inst);
}
