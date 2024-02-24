#ifndef EVENT_H
#define EVENT_H

typedef enum {
	EVENT_NULL,
	LAST_EVENT
} Event;

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
