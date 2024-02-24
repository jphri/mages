#ifndef EVENT_H
#define EVENT_H

#include "game_objects.h"
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
	EntityID player_id;
};

#define EVENT_EMIT(EVENT_TYPE, ...) \
	event_emit(EVENT_TYPE, &(EVENT_TYPE##_struct){ __VA_ARGS__ })

typedef unsigned int SubscriberID;
typedef void (*SubscriberCallback)(Event event, const void *what);

void event_init(void);
void event_terminate(void);
void event_cleanup(void);

SubscriberID event_create_subscriber(SubscriberCallback cbk);
void         event_delete_subscriber(SubscriberID id);
void         event_subscribe(SubscriberID id, Event topic);
void         event_unsubscribe(SubscriberID id, Event event);

void         event_emit(Event event, const void *data);

#endif
