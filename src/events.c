#include <string.h>
#include "util.h"
#include "events.h"

typedef unsigned int SubscriptionID;

typedef struct {
	SubscriberCallback cbk;
	SubscriptionID subs[LAST_EVENT];
} Subscriber;

typedef struct {
	Event topic;
	SubscriberID subscriber;
	/* for subscription_list */
	SubscriptionID next, prev;
} Subscription;

#define SUBSCRIBER(ID) \
	((Subscriber*)objalloc_data(&subscribers, ID))

#define SUBSCRIPTION(ID) \
	((Subscription*)objalloc_data(&subscriptions, ID))

static ObjectAllocator subscribers;
static ObjectAllocator subscriptions;
static SubscriptionID subscription_list[LAST_EVENT];

static ArrayBuffer to_unsub;

void
event_init(void)
{
	arrbuf_init(&to_unsub);
	objalloc_init(&subscribers, sizeof(Subscriber));
	objalloc_init(&subscriptions, sizeof(Subscription));
}

void
event_terminate(void)
{
	arrbuf_free(&to_unsub);
	objalloc_end(&subscribers);
	objalloc_end(&subscriptions);
}

void
event_cleanup(void)
{
	Span tounsub_span = arrbuf_span(&to_unsub);
	SPAN_FOR(tounsub_span, id, SubscriptionID) {
		SubscriptionID next, prev;

		next = SUBSCRIPTION(*id)->next;
		prev = SUBSCRIPTION(*id)->prev;
		if(next) SUBSCRIPTION(next)->prev = prev;
		if(prev) SUBSCRIPTION(prev)->next = next;
		if(subscription_list[SUBSCRIPTION(*id)->topic] == *id)
			subscription_list[SUBSCRIPTION(*id)->topic] = SUBSCRIPTION(SUBSCRIPTION(*id)->topic)->next;
	}

	arrbuf_clear(&to_unsub);
	objalloc_clean(&subscribers);
	objalloc_clean(&subscriptions);
}

SubscriberID 
event_create_subscriber(SubscriberCallback cbk)
{
	SubscriberID id = objalloc_alloc(&subscribers);
	SUBSCRIBER(id)->cbk         = cbk;
	memset(SUBSCRIBER(id)->subs, 0, sizeof(SUBSCRIBER(id)->subs));

	return id;
}

void
event_delete_subscriber(SubscriberID id)
{
	for(int i = 0; i < LAST_EVENT; i++)
		event_unsubscribe(id, i);
	objalloc_free(&subscribers, id);
}

void
event_subscribe(SubscriberID id, Event topic)
{
	SubscriptionID sub = objalloc_alloc(&subscriptions);
	SUBSCRIBER(id)->subs[topic] = sub;
	SUBSCRIPTION(sub)->subscriber = id;
	SUBSCRIPTION(sub)->topic = topic;
	
	SUBSCRIPTION(sub)->prev = 0;
	SUBSCRIPTION(sub)->next = subscription_list[topic];
	if(subscription_list[topic])
		SUBSCRIPTION(subscription_list[topic])->prev = sub;
	subscription_list[topic] = sub;
}

void
event_unsubscribe(SubscriberID id, Event topic)
{
	SubscriptionID sub = SUBSCRIBER(id)->subs[topic];
	if(!sub)
		return;

	arrbuf_insert(&to_unsub, sizeof(SubscriptionID), &sub);

	SUBSCRIBER(id)->subs[topic] = 0;
	objalloc_free(&subscriptions, sub);
}

void
event_emit(Event event, const void *data)
{
	for(SubscriptionID id = subscription_list[event];
		id;
		id = SUBSCRIPTION(id)->next)
	{
		SUBSCRIBER(SUBSCRIPTION(id)->subscriber)->cbk(event, data);
	}
}
