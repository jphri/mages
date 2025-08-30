#include <glad/gles2.h>
#include <stdio.h>
#include <stdarg.h>

#include "util.h"
#include "glutil.h"

GLuint
ugl_compile_shader(const char *shader_name, GLenum shader_type, GLint size, const char source[static size])
{
	GLuint shader = glCreateShader(shader_type);
	int compiled;

	glShaderSource(shader, 1, &source, &size);
	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if(!compiled) {
		int info_log_length;

		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_log_length);
		char *info_log = emalloc(info_log_length);

		glGetShaderInfoLog(shader, info_log_length, &info_log_length, info_log);
		printf("Error compiling the shader '%s': %s\n", shader_name, info_log);

		free(info_log);
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

void
ugl_link_program(GLuint program, const char *program_name, GLsizei size, GLuint shaders[static size])
{
	int linked;

	for(GLsizei i = 0; i < size; i++)
		glAttachShader(program, shaders[i]);

	glLinkProgram(program);

	for(GLsizei i = 0; i < size; i++)
		glDetachShader(program, shaders[i]);

	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if(!linked) {
		int info_log_length;

		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_log_length);
		char *info_log = emalloc(info_log_length);

		glGetProgramInfoLog(program, info_log_length, &info_log_length, info_log);
		printf("Error linking the program '%s': %s\n", program_name, info_log);

		free(info_log);
	}
}

GLuint
ugl_create_buffer(GLenum usage, GLuint size, void *data)
{
	GLuint buffer;

	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, size, data, usage);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return buffer;
}

GLuint
ugl_compile_shader_file(const char *file_path, GLenum shader_type)
{
	FILE *fp = fopen(file_path, "r");
	size_t size;
	char *data;

	if(!fp) {
		printf("Error compiling the shader '%s': file not found\n", file_path);
		return 0;
	}

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	data = emalloc(size);

	(void)!fread(data, 1, size, fp);

	fclose(fp);
	GLuint shader = ugl_compile_shader(file_path, shader_type, size, data);
	efree(data);

	return shader;
}

GLuint
ugl_create_vao(GLuint n_specs, VaoSpec specs[static n_specs])
{
	GLuint vao;

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	for(GLuint i = 0; i < n_specs; i++) {
		uintptr_t offset = specs[i].offset;
		glBindBuffer(GL_ARRAY_BUFFER, specs[i].buffer);
		glEnableVertexAttribArray(specs[i].name);
		switch(specs[i].type) {
		case GL_INT:
		case GL_BYTE:
		case GL_SHORT:
		case GL_UNSIGNED_INT:
		case GL_UNSIGNED_BYTE:
		case GL_UNSIGNED_SHORT:
			glVertexAttribIPointer(
				specs[i].name,
				specs[i].size,
				specs[i].type,
				specs[i].stride,
				(void*)offset
			);
			break;
		default:
			glVertexAttribPointer(
				specs[i].name,
				specs[i].size,
				specs[i].type,
				GL_FALSE,
				specs[i].stride,
				(void*)offset
			);
		}
		glVertexAttribDivisor(specs[i].name, specs[i].divisor);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return vao;
}

void
ugl_draw(GLuint program, GLuint vao, GLenum type, GLuint vert)
{
	glUseProgram(program);
	glBindVertexArray(vao);

	glDrawArrays(type, 0, vert);

	glBindVertexArray(0);
	glUseProgram(0);
}

void
ugl_draw_instanced(GLuint program, GLuint vao, GLenum type, GLuint vert, GLuint n_inst)
{
	glUseProgram(program);
	glBindVertexArray(vao);

	glDrawArraysInstanced(type, 0, vert, n_inst);

	glBindVertexArray(0);
	glUseProgram(0);
}

