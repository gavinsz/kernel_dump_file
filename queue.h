#ifndef _CAM_QUEUE_H
#define _CAM_QUEUE_H

#include <linux/list.h>
//#include <linux/mutex.h>
#include <linux/spinlock.h>
#include "uvc_udp.h"

struct queue {
	unsigned int size;
	//struct mutex mutex;
	spinlock_t irqlock;
	struct list_head list;
};

struct element {
    struct pkt pk;
	struct list_head list;
};

extern struct queue v_queue;

static inline void queue_create(struct queue *queue)
{
	INIT_LIST_HEAD(&queue->list);
	//mutex_init(&queue->mutex);
	spin_lock_init(&queue->irqlock);
	queue->size = 0;
}

static inline void enqueue(struct element *e, struct queue *queue)
{
	unsigned long flags;
	//mutex_lock(&queue->mutex);
	spin_lock_irqsave(&queue->irqlock, flags);
	list_add_tail(&e->list, &queue->list);
	queue->size++;
	//mutex_unlock(&queue->mutex);
	spin_unlock_irqrestore(&queue->irqlock, flags);
}

static inline struct element *dequeue(struct queue *queue)
{
	struct element* tmp = NULL;
	unsigned long flags;

	//mutex_lock(&queue->mutex);
	spin_lock_irqsave(&queue->irqlock, flags);
	if (list_empty(&queue->list)) {
		//mutex_unlock(&queue->mutex);
		spin_unlock_irqrestore(&queue->irqlock, flags);
		return NULL;
	}

	tmp = list_first_entry(&queue->list, struct element, list);
	list_del(&tmp->list);
	queue->size--;
	//mutex_unlock(&queue->mutex);
	spin_unlock_irqrestore(&queue->irqlock, flags);
	return tmp;
}

static inline unsigned int queue_size(struct queue *queue)
{
	return queue->size;
}

#endif

