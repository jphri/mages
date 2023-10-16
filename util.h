#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

typedef struct ArrayBuffer ArrayBuffer;
typedef struct StrView StrView;

struct ArrayBuffer {
	size_t size;
	size_t reserved;
	void *data;
};

struct StrView {
	const char *begin;
	const char *end;
};

#define LENGTH(ARR) (sizeof(ARR) / sizeof(ARR)[0])
#define ASSERT(CHECK) if(!(CHECK)) { die("%s:%d: '%s' failed\n", __FILE__, __LINE__, #CHECK); }

#define emalloc(size) _emalloc(size, __FILE__, __LINE__)
#define efree(ptr) _efree(ptr, __FILE__, __LINE__)
#define erealloc(ptr, size) _erealloc(ptr, size, __FILE__, __LINE__)

void   arrbuf_init(ArrayBuffer *buffer);
void   arrbuf_init_mem(ArrayBuffer *buffer, size_t data_size, char data[data_size]);

void   arrbuf_reserve(ArrayBuffer *buffer,   size_t element_size);
void   arrbuf_insert(ArrayBuffer *buffer,    size_t element_size, const void *data);
void   arrbuf_insert_at(ArrayBuffer *buffer, size_t element_size, const void *data, size_t pos);
void   arrbuf_remove(ArrayBuffer *buffer,    size_t element_size, size_t pos);
size_t arrbuf_length(ArrayBuffer *buffer,    size_t element_size);
void   arrbuf_clear(ArrayBuffer *buffer);

void   *arrbuf_peektop(ArrayBuffer *buffer, size_t element_size);
void    arrbuf_poptop(ArrayBuffer *buffer, size_t element_size);

void   arrbuf_free(ArrayBuffer *buffer);

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
char   *strview_str(StrView view);
void    strview_str_mem(StrView view, char *data, size_t size);

void die(const char *fmt, ...);
char *read_file(const char *path, size_t *size);

void *_emalloc(size_t size, const char *file, int line);
void  _efree(void *ptr, const char *file, int line);
void *_erealloc(void *ptr, size_t size, const char *file, int line);

#endif
