#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>

#include "util.h"

typedef struct {
	ObjectID next, prev;
	bool dead;
	unsigned char obj[];
} ObjectNode;

static ObjectID    node_to_object_id(ObjectAllocator *alloc, void *ptr);
static ObjectNode *object_id_to_node(ObjectAllocator *alloc, ObjectID id);
static void       *object_id_to_object(ObjectAllocator *alloc, ObjectID id);
static void        insert_obj_list(ObjectAllocator *alloc, ObjectID id);
static void        remove_obj_list(ObjectAllocator *alloc, ObjectID id);

static void *defaultalloc_allocate(size_t bytes, void *user_ptr);
static void  defaultalloc_deallocate(void *ptr, void *user_ptr);

static void *readline_proc(FILE *fp, ArrayBuffer *buffer);
static inline void check_buffer_initialized(ArrayBuffer *buffer)
{
	assert(buffer->initialized && "You didn't initialize the buffer, you idiot!");
}

void
arrbuf_init(ArrayBuffer *buffer)
{
	arrbuf_init_allocator(buffer, allocator_default());
}

void
arrbuf_init_allocator(ArrayBuffer *buffer, Allocator allocator)
{
	buffer->size = 0;
	buffer->initialized = true;
	buffer->reserved = 1;
	buffer->allocator = allocator;
	buffer->data = alloct_allocate(&buffer->allocator, 1);
}

void
arrbuf_reserve(ArrayBuffer *buffer, size_t size)
{
	int need_change = 0;
	check_buffer_initialized(buffer);

	while(buffer->reserved < buffer->size + size) {
		buffer->reserved *= 2;
		need_change = 1;
	}

	if(need_change) {
		void *newptr = alloct_allocate(&buffer->allocator, buffer->reserved);
		memcpy(newptr, buffer->data, buffer->size);
		alloct_deallocate(&buffer->allocator, buffer->data);
		buffer->data = newptr;
	}
}

void
arrbuf_insert(ArrayBuffer *buffer, size_t element_size, const void *data)
{
	check_buffer_initialized(buffer);
	void *ptr = arrbuf_newptr(buffer, element_size);
	memcpy(ptr, data, element_size);
}

void
arrbuf_insert_at(ArrayBuffer *buffer, size_t size, const void *data, size_t pos)
{
	check_buffer_initialized(buffer);
	void *ptr = arrbuf_newptr_at(buffer, size, pos);
	memcpy(ptr, data, size);
}

void
arrbuf_remove(ArrayBuffer *buffer, size_t size, size_t pos)
{
	check_buffer_initialized(buffer);
	memmove((unsigned char *)buffer->data + pos, (unsigned char*)buffer->data + pos + size, buffer->size - pos - size);
	buffer->size -= size;
}

size_t
arrbuf_length(ArrayBuffer *buffer, size_t element_size)
{
	check_buffer_initialized(buffer);
	return buffer->size / element_size;
}

void
arrbuf_clear(ArrayBuffer *buffer)
{
	check_buffer_initialized(buffer);
	buffer->size = 0;
}

Span
arrbuf_span(ArrayBuffer *buffer)
{
	return (Span){ buffer->data, (unsigned char*)buffer->data + buffer->size };
}

void *
arrbuf_peektop(ArrayBuffer *buffer, size_t element_size)
{
	check_buffer_initialized(buffer);
	if(buffer->size < element_size)
		return NULL;
	return (unsigned char*)buffer->data + (buffer->size - element_size);
}

void
arrbuf_poptop(ArrayBuffer *buffer, size_t element_size)
{
	check_buffer_initialized(buffer);
	if(element_size > buffer->size)
		buffer->size = 0;
	else
		buffer->size -= element_size;
}

void
arrbuf_free(ArrayBuffer *buffer)
{
	free(buffer->data);
	buffer->initialized = false;
}

void *
arrbuf_newptr(ArrayBuffer *buffer, size_t size)
{
	void *ptr;

	check_buffer_initialized(buffer);
	arrbuf_reserve(buffer, size);
	ptr = (unsigned char*)buffer->data + buffer->size;
	buffer->size += size;

	return ptr;
}

void *
arrbuf_newptr_at(ArrayBuffer *buffer, size_t size, size_t pos)
{
	void *ptr;

	check_buffer_initialized(buffer);
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

	check_buffer_initialized(buffer);
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
	/* broken, don't use it. */
	void *ret_data;
	ArrayBuffer buffer;

	(void)data;
	(void)size;

	arrbuf_init_allocator(&buffer, allocator_default());
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
	(void)!fread(result, 1, size, fp);
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

void
objalloc_init(ObjectAllocator *obj, size_t obj_size)
{
	objalloc_init_allocator(obj, obj_size, allocator_default());
}

void
objalloc_init_allocator(ObjectAllocator *obj, size_t obj_size, Allocator alloc)
{
	arrbuf_init_allocator(&obj->data_buffer,  alloc);
	arrbuf_init_allocator(&obj->free_stack, alloc);
	arrbuf_init_allocator(&obj->dirty_buffer, alloc);
	arrbuf_init_allocator(&obj->meta_buffer, alloc);

	obj->obj_size = obj_size;
	obj->node_size = sizeof(ObjectNode);
	obj->object_list = 0;
	obj->clean_cbk = NULL;
}

void
objalloc_end(ObjectAllocator *obj)
{
	arrbuf_free(&obj->data_buffer);
	arrbuf_free(&obj->free_stack);
	arrbuf_free(&obj->dirty_buffer);
	arrbuf_free(&obj->meta_buffer);
	obj->object_list = 0;
}

void
objalloc_clean(ObjectAllocator *obj)
{
	Span span = arrbuf_span(&obj->dirty_buffer);
	SPAN_FOR(span, id, ObjectID) {
		if(obj->clean_cbk)
			obj->clean_cbk(obj, *id);

		remove_obj_list(obj, *id);
		arrbuf_insert(&obj->free_stack, sizeof(ObjectID), id);
	}
	arrbuf_clear(&obj->dirty_buffer);
}

void
objalloc_reset(ObjectAllocator *obj)
{
	arrbuf_clear(&obj->dirty_buffer);
	arrbuf_clear(&obj->free_stack);
	arrbuf_clear(&obj->data_buffer);
	arrbuf_clear(&obj->meta_buffer);
	obj->object_list = 0;
}

ObjectID
objalloc_alloc(ObjectAllocator *obj)
{
	ObjectID *stack = arrbuf_peektop(&obj->free_stack, sizeof(ObjectID)),
			  node_id;
	ObjectNode *node;

	if(stack) {
		node = object_id_to_node(obj, *stack);
		arrbuf_poptop(&obj->free_stack, sizeof(ObjectID));
	} else {
		node = arrbuf_newptr(&obj->meta_buffer, obj->node_size);
		arrbuf_newptr(&obj->data_buffer, obj->obj_size);
	}
	node_id = node_to_object_id(obj, node);
	memset(node, 0, obj->node_size);
	insert_obj_list(obj, node_id);

	return node_id;
}

void *
objalloc_data(ObjectAllocator *alloc, ObjectID id)
{
	return object_id_to_object(alloc, id);
}

void
objalloc_free(ObjectAllocator *alloc, ObjectID id)
{
	if(!object_id_to_node(alloc, id)->dead) {
		object_id_to_node(alloc, id)->dead = true;
		arrbuf_insert(&alloc->dirty_buffer, sizeof(ObjectID), &id);
	}
}

ObjectID
objalloc_begin(ObjectAllocator *alloc)
{
	ObjectID list = alloc->object_list;
	while(list && object_id_to_node(alloc, list)->dead)
		list = object_id_to_node(alloc, list)->next;
	return list;
}

ObjectID
objalloc_next(ObjectAllocator *alloc, ObjectID id)
{
	do {
		id = object_id_to_node(alloc, id)->next;
	} while(id && object_id_to_node(alloc, id)->dead);
	return id;
}

ObjectID
node_to_object_id(ObjectAllocator *alloc, void *ptr)
{
	return 1 + ((unsigned char*)ptr - (unsigned char*)alloc->meta_buffer.data) / alloc->node_size;
}

ObjectNode *
object_id_to_node(ObjectAllocator *alloc, ObjectID id)
{
	if(id == 0)
		return NULL;
	id --;
	return (ObjectNode*)((unsigned char*)alloc->meta_buffer.data + alloc->node_size * id);
}

void *
object_id_to_object(ObjectAllocator *alloc, ObjectID id)
{
	if(id == 0)
		return NULL;
	id --;
	return (unsigned char*)alloc->data_buffer.data + alloc->obj_size * id;
}

void
insert_obj_list(ObjectAllocator *alloc, ObjectID id)
{
	ObjectNode *node      = object_id_to_node(alloc, id);
	ObjectNode *base_node = object_id_to_node(alloc, alloc->object_list);

	if(base_node)
		base_node->prev = id;

	node->prev = 0;
	node->next = alloc->object_list;
	alloc->object_list = id;
}

void
remove_obj_list(ObjectAllocator *alloc, ObjectID id)
{
	ObjectNode *next, *prev, *node;
	node = object_id_to_node(alloc, id);
	next = object_id_to_node(alloc, node->next);
	prev = object_id_to_node(alloc, node->prev);

	if(next) next->prev = node->prev;
	if(prev) prev->next = node->next;

	if(id == alloc->object_list)
		alloc->object_list = node->next;
}

bool
objalloc_is_dead(ObjectAllocator *alloc, ObjectID id)
{
	return object_id_to_node(alloc, id)->dead;
}

void *
alloct_allocate(Allocator *a, size_t s)
{
	return a->allocate(s, a->userptr);
}

void
alloct_deallocate(Allocator *a, void *ptr)
{
	a->deallocate(ptr, a->userptr);
}

Allocator 
allocator_default()
{
	return (Allocator) {
		.allocate = defaultalloc_allocate,
		.deallocate = defaultalloc_deallocate
	};
}

void *
defaultalloc_allocate(size_t bytes, void *user_ptr)
{
	(void)user_ptr;
	return malloc(bytes);
}

void 
defaultalloc_deallocate(void *ptr, void *user_ptr)
{
	(void)user_ptr;
	free(ptr);
}
