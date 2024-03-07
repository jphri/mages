#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>

#include "util.h"

#define UTF8_TWO_BYTES 0xC0
#define UTF8_THREE_BYTES 0xE0
#define UTF8_FOUR_BYTES 0xF0

#define OBJECT_ALLOCATOR_PAGE_SIZE 1024

typedef struct ObjectNode ObjectNode;
struct ObjectNode {
	ObjectPool *pool;
	ObjectNode *next, *prev;
	bool dead;
};

static uintptr_t align_memory(uintptr_t ptr, size_t align)
{
	return (((ptr + align - 1) / align) * align);
}

static ObjectNode *data_to_node(void *data)
{
	return ((void**)data)[-1];
}

static void new_obj_page(ObjectPool *pool)
{
	void *page = malloc(pool->node_size * OBJECT_ALLOCATOR_PAGE_SIZE);
	arrbuf_insert(&pool->pages, sizeof(void*), &page);
	
	for(int i = 0; i < OBJECT_ALLOCATOR_PAGE_SIZE; i++) {
		ObjectNode *node = (ObjectNode*)((uintptr_t)page + pool->node_size * i);
		node->pool = pool;
		node->prev = NULL;
		node->next = NULL;
		node->dead = true;

		void *data = (void*)align_memory((uintptr_t)node + sizeof(void*) + sizeof(ObjectNode), pool->alignment);
		/* store the node address before the data itself */
		((void**)data)[-1] = node;
		
		/* store the data pointer because it is easier to get the node back from data than it is
		 * to calculate the alignment again */
		arrbuf_insert(&pool->free_stack, sizeof(void*), &data);
	}
}

static void insert_obj_node(ObjectPool *pool, void *data)
{
	ObjectNode *node = data_to_node(data);
	
	/* store the data, not the node */
	if(pool->object_list)
		data_to_node(pool->object_list)->prev = data;

	node->prev = NULL;
	node->next = pool->object_list;
	pool->object_list = data;
}

static void remove_obj_node(ObjectPool *pool, void *data)
{
	ObjectNode *node = data_to_node(data);
	
	if(node->next)
		data_to_node(node->next)->prev = node->prev;
	if(node->prev)
		data_to_node(node->prev)->next = node->next;
	
	if(pool->object_list == data)
		pool->object_list = node->next;
}

static void *defaultalloc_allocate(size_t bytes, void *user_ptr);
static void  defaultalloc_deallocate(void *ptr, void *user_ptr);

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
	alloct_deallocate(&buffer->allocator, buffer->data);
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

int
fbuf_open(FileBuffer *buffer, const char *path, const char *mode, Allocator alloc)
{
	buffer->file_handle = fopen(path, mode);
	if(!buffer->file_handle)
		return 1;
	arrbuf_init_allocator(&buffer->data_buffer, alloc);
	return 0;
}

int
fbuf_read(FileBuffer *buffer, size_t size)
{
	void *ptr;
	int count;

	arrbuf_clear(&buffer->data_buffer);
	ptr = arrbuf_newptr(&buffer->data_buffer, size);
	
	count = fread(ptr, 1, size, buffer->file_handle);
	return count;
}

int
fbuf_write(FileBuffer *buffer, size_t size, void *ptr)
{
	arrbuf_insert(buffer->data_buffer.data, size, ptr);
	return 0;
}

int
fbuf_flush(FileBuffer *buffer)
{
	int count = fwrite(buffer->data_buffer.data, buffer->data_buffer.size, 1, buffer->file_handle);
	fflush(buffer->file_handle);
	return count;
}

char *
fbuf_data(FileBuffer *buffer)
{
	return buffer->data_buffer.data;
}

int
fbuf_data_size(FileBuffer *buffer)
{
	return buffer->data_buffer.size;
}

int
fbuf_read_line(FileBuffer *buffer, int delim)
{
	int c;
	arrbuf_clear(&buffer->data_buffer);
	while((c = fgetc(buffer->file_handle)) != EOF) {
		if(c == delim) {
			break;
		}
		arrbuf_insert(&buffer->data_buffer, sizeof(char), &(char){ c });
	}
	if(buffer->data_buffer.size > 0)
		return buffer->data_buffer.size;
	return EOF;
}

StrView
fbuf_data_view(FileBuffer *buffer)
{
	return to_strview_buffer(fbuf_data(buffer), fbuf_data_size(buffer));
}

void
fbuf_close(FileBuffer *buffer)
{
	fbuf_flush(buffer);
	arrbuf_free(&buffer->data_buffer);
	fclose(buffer->file_handle);
}

StrView
to_strview(const char *str)
{
	return (StrView) {
		.begin = (const unsigned char*)str,
		.end = (const unsigned char*)str + strlen(str)
	};
}

StrView 
to_strview_buffer(const void *ptr, size_t s)
{
	return (StrView){
		.begin = ptr,
		.end = (const unsigned char*)ptr + s
	};
}

StrView
strview_token(StrView *str, const char *delim)
{
	StrView result;

	if(str->begin >= str->end)
		return (StrView){ str->begin, str->begin };

	/* find the begin of next token */
	while(str->begin < str->end)
		if(strchr(delim, utf8_decode(*str)) == NULL)
			break;
		else
			utf8_advance(str);
	
	result.begin = str->begin;
	result.end = str->end;
	
	while(str->begin < str->end) {
		if(strchr(delim, utf8_decode(*str)) != NULL)
			break;
		utf8_advance(str);
	}
	result.end = str->begin;
	utf8_advance(str);

	return result;
}

int
strview_cmp(StrView str, const char *str2)
{
	size_t len1 = (unsigned char*)str.end - (unsigned char *)str.begin;
	size_t len2 = strlen(str2);

	if(len1 != len2)
		return len1 - len2;
	else
		return memcmp(str.begin, str2, len1);
}

int
strview_cmpstr(StrView str, StrView str2)
{
	size_t len1 = (unsigned char*)str.end - (unsigned char *)str.begin;
	size_t len2 = (unsigned char*)str2.end - (unsigned char *)str2.begin;

	if(len1 != len2)
		return len1 - len2;
	else
		return memcmp(str.begin, str2.begin, len1);
}


int
strview_int(StrView str, int *result)
{
	const unsigned char *s = str.begin;
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
	const unsigned char *s;
	int is_negative = 0;

	int integer_part = 0;
	float fract_part = 0;

	StrView ss = str;
	StrView number = strview_token(&ss, ".");

	if(utf8_decode(number) == '-') {
		is_negative = 1;
		utf8_advance(&number);
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
	}

	*result += fract_part;
	if(is_negative)
		*result *= -1;

	return 1;
}

char *
strview_str(StrView view)
{
	size_t size = (unsigned char*)view.end - (unsigned char*)view.begin;
	char *ptr = malloc(size + 1);

	strview_str_mem(view, ptr, size + 1);

	return ptr;
}

void
strview_str_mem(StrView view, char *data, size_t size)
{
	size = (size > (size_t)((unsigned char*)view.end - (unsigned char*)view.begin) + 1) ? (size_t)((unsigned char*)view.end - (unsigned char*)view.begin) + 1 : size;
	strncpy(data, (const char*)view.begin, size);
	data[size-1] = 0;
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
objpool_init(ObjectPool *pool, size_t object_size, size_t object_alignment)
{
	arrbuf_init(&pool->pages);
	arrbuf_init(&pool->free_stack);
	arrbuf_init(&pool->dirty_buffer);

	pool->node_size = align_memory(sizeof(ObjectNode) + object_alignment + sizeof(void*) + object_size, object_alignment);
	pool->obj_size  = object_size;
	pool->alignment = object_alignment;

	objpool_reset(pool);
}

void
objpool_clean(ObjectPool *pool) 
{
	Span span = arrbuf_span(&pool->dirty_buffer);
	SPAN_FOR(span, data, void*) {
		if(pool->clean_cbk)
			pool->clean_cbk(pool, *data);
		remove_obj_node(pool, *data);
		arrbuf_insert(&pool->free_stack, sizeof(void*), data);
	}
	arrbuf_clear(&pool->dirty_buffer);
}

void
objpool_reset(ObjectPool *pool)
{
	Span span = arrbuf_span(&pool->pages);
	SPAN_FOR(span, page, void*) {
		free(*page);
	}
	arrbuf_clear(&pool->pages);
	arrbuf_clear(&pool->free_stack);
	arrbuf_clear(&pool->dirty_buffer);
	pool->object_list = NULL;
	new_obj_page(pool);
}

void
objpool_terminate(ObjectPool *pool)
{
	Span span = arrbuf_span(&pool->pages);
	SPAN_FOR(span, page, void*) {
		free(*page);
	}
	arrbuf_free(&pool->pages);
	arrbuf_free(&pool->free_stack);
	arrbuf_free(&pool->dirty_buffer);
}

void *
objpool_begin(ObjectPool *pool) 
{
	void *ptr = pool->object_list;
	while(ptr && data_to_node(ptr)->dead) {
		ptr = data_to_node(ptr)->next;
	}
	return ptr;
}

void *
objpool_next(void *data)
{
	void *ptr = data_to_node(data)->next;
	while(ptr && data_to_node(ptr)->dead) {
		ptr = data_to_node(ptr)->next;
	}
	return ptr;
}

void *
objpool_new(ObjectPool *pool)
{
	void **element = arrbuf_peektop(&pool->free_stack, sizeof(void*));
	if(!element) {
		new_obj_page(pool);
		element = arrbuf_peektop(&pool->free_stack, sizeof(void*));
	}
	arrbuf_poptop(&pool->free_stack, sizeof(void*));
	data_to_node(*element)->dead = false;
	insert_obj_node(pool, *element);
	return *element;
}

void
objpool_free(void *object_ptr)
{
	ObjectNode *node = data_to_node(object_ptr);
	if(node->dead) {
		printf("Detected double free.\n");
		printf("Be careful...\n");
		return;
	}
	node->dead = true;
	
	arrbuf_insert(&node->pool->dirty_buffer, sizeof(void*), &object_ptr);
}

bool
objpool_is_dead(void *object_ptr)
{
	return data_to_node(object_ptr)->dead;
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
allocator_default(void)
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

int 
utf8_decode(StrView str)
{
	int code = 0;
	const unsigned char *begin = str.begin;
	const unsigned char *end   = str.end;

	if(begin > end) {
		return -1;
	}
	
	if(*begin < 0x80) {
		return *begin;
	}

	if((*begin & 0xF0) == 0xF0) {
		if((end - begin) < 4)
			return -1;
		code  = (*begin & 0x1f) << 18; begin++;
		code |= (*begin & 0x3f) << 12; begin++;
		code |= (*begin & 0x3f) <<  6; begin++;
		code |= (*begin & 0x3f) <<  0; begin++;
		return code;
	}

	if((*begin & 0xE0) == 0xE0) {
		if((end - begin) < 3)
			return -1;
		code  = (*begin & 0x1f) << 12; begin++;
		code |= (*begin & 0x3f) <<  6; begin++;
		code |= (*begin & 0x3f) <<  0; begin++;
		return code;
	}

	if((*begin & 0xC0) == 0xC0) {
		if((end - begin) < 2)
			return -1;
		code  = (*begin & 0x1f) << 6; begin++;
		code |= (*begin & 0x3f); begin++;
		return code;
	}
	
	return -1;
}

void
utf8_advance(StrView *str)
{
	const unsigned char *begin = str->begin;
	const unsigned char *end = str->end;

	if((*begin & 0xF0) == 0xF0) {
		begin += 4;
	} else if((*begin & 0xE0) == 0xE0) {
		begin += 3;
	} else if((*begin & 0xC0) == 0xC0) {
		begin += 2;
	} else {
		begin += 1;
	}

	str->begin = begin;
	str->end = end;
}

int
utf8_multibyte_next(StrView view, int from)
{
	const unsigned char *c = view.begin + from;
	c++;
	while(c < view.end) {
		if((*c & 0x80) == 0x80)
			c++;
		else
			break;
	}
	if(c > view.end)
		c = view.end;
	return (c - view.begin) - from;
}

int
utf8_multibyte_prev(StrView view, int from)
{
	const unsigned char *c = view.begin + from;
	c--;
	while(c < view.begin) {
		if((*c & 0x80) == 0x80)
			c--;
		else
			break;
	}
	if(c < view.begin)
		c = view.begin;

	ptrdiff_t prev = from - (c - view.begin);
	return prev;
}

