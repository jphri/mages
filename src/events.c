#include <string.h>
#include "util.h"
#include "events.h"

typedef struct Subscription Subscription;
struct Subscription {
	Event topic;
	Subscriber* subscriber;
	/* for subscription_list */
	Subscription *next, *prev;
};

struct Subscriber {
	SubscriberCallback cbk;
	Subscription *subs[LAST_EVENT];
};


#define SUBSCRIBER(ID) \
	((Subscriber*)objalloc_data(&subscribers, ID))

#define SUBSCRIPTION(ID) \
	((Subscription*)objalloc_data(&subscriptions, ID))

static ObjectPool subscribers;
static ObjectPool subscriptions;
static Subscription *subscription_list[LAST_EVENT];

static ArrayBuffer to_unsub;

void
event_init(void)
{
	arrbuf_init(&to_unsub);
	objpool_init(&subscribers, sizeof(Subscriber), DEFAULT_ALIGNMENT);
	objpool_init(&subscriptions, sizeof(Subscription), DEFAULT_ALIGNMENT);
}

void
event_terminate(void)
{
	arrbuf_free(&to_unsub);
	objpool_terminate(&subscribers);
	objpool_terminate(&subscriptions);
}

void
event_cleanup(void)
{
	Span tounsub_span = arrbuf_span(&to_unsub);
	SPAN_FOR(tounsub_span, id, Subscription*) {
		Subscription *next, *prev;

		next = (*id)->next;
		prev = (*id)->prev;
		if(next) next->prev = prev;
		if(prev) prev->next = next;
		if(subscription_list[(*id)->topic] == *id)
			subscription_list[(*id)->topic] = subscription_list[((*id)->topic)]->next;
	}

	arrbuf_clear(&to_unsub);
	objpool_clean(&subscribers);
	objpool_clean(&subscriptions);
}

Subscriber *
event_create_subscriber(SubscriberCallback cbk)
{
	Subscriber *id = objpool_new(&subscribers);
	(id)->cbk         = cbk;
	memset(id->subs, 0, sizeof(id->subs));

	return id;
}

void
event_delete_subscriber(Subscriber *id)
{
	for(int i = 0; i < LAST_EVENT; i++)
		event_unsubscribe(id, i);
	objpool_free(id);
}

void
event_subscribe(Subscriber *scriber, Event topic)
{
	Subscription *sub = objpool_new(&subscriptions);
	scriber->subs[topic] = sub;
	sub->subscriber = scriber;
	sub->topic = topic;
	
	sub->prev = 0;
	sub->next = subscription_list[topic];
	if(subscription_list[topic])
		subscription_list[topic]->prev = sub;
	subscription_list[topic] = sub;
}

void
event_unsubscribe(Subscriber *scriber, Event topic)
{
	Subscription *sub = scriber->subs[topic];
	if(!sub)
		return;

	arrbuf_insert(&to_unsub, sizeof(Subscription*), &sub);

	scriber->subs[topic] = 0;
	objpool_free(sub);
}

void
event_emit(Event event, const void *data)
{
	for(Subscription *ption = subscription_list[event];
		ption;
		ption = objpool_next(ption))
	{
		ption->subscriber->cbk(event, data);
	}
}
