#ifndef SYSTEM_H
#define SYSTEM_H

#ifndef SYS_ID_TYPE
#error "SYS_ID_TYPE not defined"
#endif

#ifndef SYS_NODE_TYPE
#error "SYS_NODE_TYPE not defined"
#endif

#include "util.h"

static void _sys_int_cleanup(SYS_ID_TYPE id);

static ArrayBuffer _sys_buffer;
static ArrayBuffer _sys_free_stack;
static SYS_ID_TYPE _sys_list;
static ArrayBuffer _sys_should_die;

static inline void _sys_init()
{
	arrbuf_init(&_sys_buffer);
	arrbuf_init(&_sys_free_stack);
	arrbuf_init(&_sys_should_die);
	_sys_list = 0;
}

static inline void _sys_reset()
{
	arrbuf_clear(&_sys_buffer);
	arrbuf_clear(&_sys_free_stack);
	arrbuf_clear(&_sys_should_die);
	_sys_list = 0;
}

static inline void _sys_deinit()
{
	efree(_sys_buffer.data);
	efree(_sys_free_stack.data);
	efree(_sys_should_die.data);
	_sys_list = 0;
}

static inline SYS_NODE_TYPE *_sys_node(SYS_ID_TYPE id)
{
	return &((SYS_NODE_TYPE*)_sys_buffer.data)[id - 1];
}

static inline void _sys_insert_list(SYS_ID_TYPE id)
{
	_sys_node(id)->next = _sys_list;
	_sys_node(id)->prev = 0;
	if(_sys_list)
		_sys_node(_sys_list)->prev = id;
	_sys_list = id;
}

static inline void _sys_remove_list(SYS_ID_TYPE id)
{
	SYS_ID_TYPE next = _sys_node(id)->next;
	SYS_ID_TYPE prev = _sys_node(id)->prev;
	
	if(id == _sys_list)
		_sys_list = next;
	if(next) _sys_node(next)->prev = _sys_node(id)->prev;
	if(prev) _sys_node(prev)->next = _sys_node(id)->next;
}

static inline SYS_ID_TYPE _sys_new()
{
	SYS_ID_TYPE n;
	SYS_ID_TYPE *stack = arrbuf_peektop(&_sys_free_stack, sizeof(SYS_ID_TYPE));

	if(stack) {
		n = *stack;
		arrbuf_poptop(&_sys_free_stack, sizeof(SYS_ID_TYPE));
	} else {
		SYS_NODE_TYPE *ptr = arrbuf_newptr(&_sys_buffer, sizeof(SYS_NODE_TYPE));
		n = (ptr - (SYS_NODE_TYPE*)_sys_buffer.data) + 1;
	}
	memset(_sys_node(n), 0, sizeof(SYS_NODE_TYPE));
	_sys_insert_list(n);
	_sys_node(n)->dead = false;
	
	return n;
}

static inline void _sys_del(SYS_ID_TYPE id)
{
	if(!_sys_node(id)->dead) {
		arrbuf_insert(&_sys_should_die, sizeof(id), &id);
		_sys_node(id)->dead = true;
	}
}

static inline void _sys_cleanup()
{
	for(SYS_ID_TYPE *id = _sys_should_die.data;
		id != (SYS_ID_TYPE*)((unsigned char*)_sys_should_die.data + _sys_should_die.size);
		id++)
	{
		_sys_int_cleanup(*id);
		_sys_remove_list(*id);
		arrbuf_insert(&_sys_free_stack, sizeof(SYS_ID_TYPE), id);
	}
	arrbuf_clear(&_sys_should_die);
}


#endif
