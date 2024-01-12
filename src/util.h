#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct ArrayBuffer ArrayBuffer;
typedef struct StrView StrView;
typedef struct Span Span;
typedef struct ObjectAllocator ObjectAllocator;
typedef struct RelPtr RelPtr;
typedef struct Allocator Allocator;

typedef uint_fast32_t ObjectID;

struct Allocator {
	void *userptr;
	void *(*allocate)(size_t bytes, void *user_ptr);
	void  (*deallocate)(void *ptr, void *user_ptr);
};

struct ArrayBuffer {
	bool initialized;
	size_t size;
	size_t reserved;
	void *data;
	Allocator allocator;
};

struct StrView {
	const char *begin;
	const char *end;
};

struct ObjectAllocator {
	ArrayBuffer data_buffer;
	ArrayBuffer free_stack;
	ArrayBuffer dirty_buffer;
	ArrayBuffer meta_buffer;
	ObjectID object_list;

	size_t node_size;
	size_t obj_size;
	size_t alignment;

	void (*clean_cbk)(ObjectAllocator *, ObjectID);
};

struct Span {
	void *begin;
	void *end;
};

struct RelPtr {
	void **base_pointer;
	uintptr_t offset;
};

#if defined(__GNUC__)
	#define ALIGNMENT_OF(X) __alignof__(X)
#elif defined(_MSC_VER)
	#define ALIGNMENT_OF(X) alignof(X)
#else
	#error "No alignment defined!"
#endif

#define LENGTH(ARR) (sizeof(ARR) / sizeof(ARR)[0])
#define ASSERT(CHECK) if(!(CHECK)) { die("%s:%d: '%s' failed\n", __FILE__, __LINE__, #CHECK); }

#define SPAN_FOR(SPAN, NAME, ...) for(__VA_ARGS__* NAME = SPAN.begin; NAME < (__VA_ARGS__*)SPAN.end; NAME++)

#define emalloc(size) _emalloc(size, __FILE__, __LINE__)
#define efree(ptr) _efree(ptr, __FILE__, __LINE__)
#define erealloc(ptr, size) _erealloc(ptr, size, __FILE__, __LINE__)

void   arrbuf_init(ArrayBuffer *buffer);
void   arrbuf_init_allocator(ArrayBuffer *buffer, Allocator allocator);

void   arrbuf_reserve(ArrayBuffer *buffer,   size_t element_size);
void   arrbuf_insert(ArrayBuffer *buffer,    size_t element_size, const void *data);
void   arrbuf_insert_at(ArrayBuffer *buffer, size_t element_size, const void *data, size_t pos);
void   arrbuf_remove(ArrayBuffer *buffer,    size_t element_size, size_t pos);
size_t arrbuf_length(ArrayBuffer *buffer,    size_t element_size);
void   arrbuf_clear(ArrayBuffer *buffer);
Span   arrbuf_span(ArrayBuffer *buffer);

void   *arrbuf_peektop(ArrayBuffer *buffer, size_t element_size);
void    arrbuf_poptop(ArrayBuffer *buffer, size_t element_size);
void   arrbuf_free(ArrayBuffer *buffer);

void arrbuf_printf(ArrayBuffer *buffer, const char *fmt, ...);

/* 
 * use only for IN-PLACE STRUCT FILLING, do not save the pointer, 
 * IT WILL become a dangling pointer after a realloc if you are not using 
 * the stack, example at arrbuf_insert
 */
void *arrbuf_newptr(ArrayBuffer *buffer, size_t element_size);
void *arrbuf_newptr_at(ArrayBuffer *buffer, size_t element_size, size_t pos);

char *readline(FILE *fp);
char *readline_mem(FILE *fp, void *data, size_t size);

StrView to_strview(const char *str);
StrView strview_token(StrView *str, const char *delim);
int     strview_cmp(StrView str, const char *str2);
char   *strview_str(StrView view);
void    strview_str_mem(StrView view, char *data, size_t size);

int strview_int(StrView str, int *result);
int strview_float(StrView str, float *result);

void die(const char *fmt, ...);
char *read_file(const char *path, size_t *size);

void *_emalloc(size_t size, const char *file, int line);
void  _efree(void *ptr, const char *file, int line);
void *_erealloc(void *ptr, size_t size, const char *file, int line);

void      objalloc_init(ObjectAllocator *alloc, size_t object_size);
void      objalloc_init_allocator(ObjectAllocator *alloc, size_t object_size, Allocator a);
void      objalloc_end(ObjectAllocator *alloc);
void      objalloc_clean(ObjectAllocator *alloc);
void      objalloc_reset(ObjectAllocator *alloc);
ObjectID  objalloc_alloc(ObjectAllocator *alloc);
void     *objalloc_data(ObjectAllocator *alloc, ObjectID id);
void      objalloc_free(ObjectAllocator *alloc, ObjectID id);
ObjectID  objalloc_begin(ObjectAllocator *alloc);
ObjectID  objalloc_next(ObjectAllocator *alloc, ObjectID next);
bool      objalloc_is_dead(ObjectAllocator *alloc, ObjectID id);

Allocator allocator_default();

void *alloct_allocate(Allocator *, size_t size);
void  alloct_deallocate(Allocator *, void *ptr);

static inline void *to_ptr(RelPtr ptr) {
	return ((unsigned char*)*ptr.base_pointer) + ptr.offset;
}

static inline int mini(int a, int b) {
	return a < b ? a : b;
}

static inline int maxi(int a, int b) {
	return a > b ? a : b;
}

static inline int clampi(int x, int minv, int maxv) {
	return mini(maxi(x, minv), maxv);
}

#endif
