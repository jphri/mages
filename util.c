#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "util.h"
static void *readline_proc(FILE *fp, ArrayBuffer *buffer);

void
arrbuf_init(ArrayBuffer *buffer) 
{
	buffer->size = 0;
	buffer->reserved = 1;
	buffer->data = malloc(1);
}

void
arrbuf_init_mem(ArrayBuffer *buffer, size_t size, char data[size])
{
	buffer->size = 0;
	buffer->reserved = size;
	buffer->data = data;
}

void
arrbuf_reserve(ArrayBuffer *buffer, size_t size)
{
	int need_change = 0;

	while(buffer->reserved < buffer->size + size) {
		buffer->reserved *= 2;
		need_change = 1;
	}

	if(need_change)
		buffer->data = realloc(buffer->data, buffer->reserved);
}

void
arrbuf_insert(ArrayBuffer *buffer, size_t element_size, const void *data)
{
	void *ptr = arrbuf_newptr(buffer, element_size);
	memcpy(ptr, data, element_size);
}

void
arrbuf_insert_at(ArrayBuffer *buffer, size_t size, const void *data, size_t pos)
{
	void *ptr = arrbuf_newptr_at(buffer, size, pos);
	memcpy(ptr, data, size);
}

void
arrbuf_remove(ArrayBuffer *buffer, size_t size, size_t pos) 
{
	memmove((unsigned char *)buffer->data + pos, (unsigned char*)buffer->data + pos + size, buffer->size - pos - size);
	buffer->size -= size;
}

size_t
arrbuf_length(ArrayBuffer *buffer, size_t element_size)
{
	return buffer->size / element_size;
}

void
arrbuf_clear(ArrayBuffer *buffer) 
{
	buffer->size = 0;
}

void *
arrbuf_peektop(ArrayBuffer *buffer, size_t element_size)
{
	if(buffer->size < element_size)
		return NULL;
	return (unsigned char*)buffer->data + (buffer->size - element_size);
}

void
arrbuf_poptop(ArrayBuffer *buffer, size_t element_size)
{
	if(element_size > buffer->size)
		buffer->size = 0;
	else
		buffer->size -= element_size;
}

void
arrbuf_free(ArrayBuffer *buffer)
{
	free(buffer->data);
}

void *
arrbuf_newptr(ArrayBuffer *buffer, size_t size)
{
	void *ptr;

	arrbuf_reserve(buffer, size);
	ptr = (unsigned char*)buffer->data + buffer->size;
	buffer->size += size;

	return ptr;
}

void *
arrbuf_newptr_at(ArrayBuffer *buffer, size_t size, size_t pos)
{
	void *ptr;

	arrbuf_reserve(buffer, size);
	memmove((unsigned char *)buffer->data + pos + size, (unsigned char*)buffer->data + pos, buffer->size - pos);
	ptr = (unsigned char*)buffer->data + pos;
	buffer->size += size;

	return ptr;
}

void
arrbuf_printf(ArrayBuffer *buffer, const char *fmt, ...) 
{
	va_list va;
	size_t print_size;

	va_start(va, fmt);
	print_size = vsnprintf(NULL, 0, fmt, va);
	va_end(va);
	
	char *ptr = arrbuf_newptr(buffer, print_size + 1);

	va_start(va, fmt);
	vsnprintf(ptr, print_size + 1, fmt, va);
	va_end(va);

	buffer->size --;
}

char *
readline_mem(FILE *fp, void *data, size_t size) 
{
	void *ret_data;
	ArrayBuffer buffer;

	arrbuf_init_mem(&buffer, size, data);
	ret_data = readline_proc(fp, &buffer);
	return ret_data;
}

char *
readline(FILE *fp)
{
	void *data;
	ArrayBuffer buffer;

	arrbuf_init(&buffer);
	data = readline_proc(fp, &buffer);
	if(!data)
		free(buffer.data);
	
	return data;
}

StrView
to_strview(const char *str)
{
	return (StrView) {
		.begin = str,
		.end = str + strlen(str)
	};
}

StrView
strview_token(StrView *str, const char *delim)
{
	StrView result = {
		.begin = str->begin,
		.end   = str->begin
	};

	if(str->begin >= str->end)
		return result;

	while(str->begin < str->end) {
		if(strchr(delim, *str->begin) != NULL)
			break;
		str->begin++;
	}
	result.end = str->begin;
	str->begin++;

	return result;
}

int
strview_cmp(StrView str, const char *str2)
{
	if(str.end - str.begin == 0)
		return strlen(str2) == 0 ? 0 : 1;
	return strncmp(str.begin, str2, str.end - str.begin - 1);
}

int
strview_int(StrView str, int *result)
{
	const char *s = str.begin;
	int is_negative = 0;

	if(*s == '-') {
		is_negative = 1;
		s++;
	}

	*result = 0;
	while(s != str.end) {
		if(!isdigit(*s))
			return 0;
		*result = *result * 10 + *s - '0';	
		s++;
	}

	if(is_negative)
		*result = *result * -1;
	
	return 1;
}

int
strview_float(StrView str, float *result)
{
	const char *s;
	int is_negative = 0;

	int integer_part = 0;
	float fract_part = 0;
	
	StrView ss = str;
	StrView number = strview_token(&ss, ".");

	if(*number.begin == '-') {
		is_negative = 1;
		number.begin ++;
	}

	*result = 0;
	if(!strview_int(number, &integer_part))
		return 0;
	*result += integer_part;
	
	number = strview_token(&ss, ".");
	
	s = number.begin;
	float f = 0.1;
	for(; s != number.end; s++, f *= 0.1) {
		if(!isdigit(*s))
			return 0;
		fract_part += (float)(*s - '0') * f;
		s++;
	}

	*result += fract_part;
	if(is_negative)
		*result *= -1;

	return 1;
}

char *
strview_str(StrView view)
{
	size_t size = view.end - view.begin;
	char *ptr = malloc(size + 1);

	strview_str_mem(view, ptr, size + 1);

	return ptr;
}

void
strview_str_mem(StrView view, char *data, size_t size) 
{
	size = (size > (size_t)(view.end - view.begin) + 1) ? (size_t)(view.end - view.begin) + 1 : size;
	strncpy(data, view.begin, size);
	data[size-1] = 0;
}

void *
readline_proc(FILE *fp, ArrayBuffer *buffer) 
{
	int c;
	
	/* skip all new-lines/carriage return */
	while((c = fgetc(fp)) != EOF)
		if(c != '\n' && c != '\r')
			break;
	
	if(c == EOF) {
		//free(buffer->data);
		return NULL;
	}
	
	for(; c != EOF && c != '\n' && c != '\r'; c = fgetc(fp))
		arrbuf_insert(buffer, sizeof(char), &(char){c});
	
	c = 0;
	arrbuf_insert(buffer, sizeof c, &c);

	return buffer->data;
}

void
die(const char *fmt, ...) 
{
	va_list va;
	
	va_start(va, fmt);
	(void)vfprintf(stderr, fmt, va);
	va_end(va);

	exit(EXIT_FAILURE);
}

char *
read_file(const char *path, size_t *s)
{
	char *result;
	size_t size;
	FILE *fp = fopen(path, "r");
	if(!fp)
		return NULL;

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	result = malloc(size);
	fread(result, 1, size, fp);
	fclose(fp);

	if(s)
		*s = size;
	
	return result;
}

void *
_emalloc(size_t size, const char *file, int line)
{
	void *ptr = malloc(size);
	if(!ptr)
		die("malloc failed at %s:%d\n", file, line);
	return ptr;
}

void
_efree(void *ptr, const char *file, int line)
{
	if(!ptr) {
		fprintf(stderr, "freeing null at %s:%d\n", file, line);
		return;
	}
	free(ptr);
}

void *
_erealloc(void *ptr, size_t size, const char *file, int line)
{
	ptr = realloc(ptr, size);
	if(!ptr) {
		die("realloc failed at %s:%d\n", file, line);
	}
    return ptr;								   
}
