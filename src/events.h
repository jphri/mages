#ifndef EVENT_H
#define EVENT_H

#include "defs.h"

typedef enum {
	EVENT_NULL,
	EVENT_PLAYER_SPAWN,
	LAST_EVENT
} Event;

#define EVENT(EV) \
	EV##_struct*

#define DEFINE_EVENT(EVENT) \
typedef struct EVENT##_struct EVENT##_struct; \
struct EVENT##_struct

DEFINE_EVENT(EVENT_PLAYER_SPAWN) {
	Entity *player;
};

#define EVENT_EMIT(EVENT_TYPE, ...) \
	event_emit(EVENT_TYPE, &(EVENT_TYPE##_struct){ __VA_ARGS__ })

typedef struct Subscriber Subscriber;
typedef void (*SubscriberCallback)(Event event, const void *what);

void event_init(void);
void event_terminate(void);
void event_cleanup(void);

Subscriber *event_create_subscriber(SubscriberCallback cbk);
void         event_delete_subscriber(Subscriber *id);
void         event_subscribe(Subscriber *id, Event topic);
void         event_unsubscribe(Subscriber *id, Event event);

void         event_emit(Event event, const void *data);

#endif
