#ifndef __LIST_H_
#define __LIST_H_

#include "lock.h"

typedef struct __node_t
{
	unsigned int value;
	struct __node_t *next;
}node_t;
typedef struct __list_t
{
	node_t *head;
	spinlock_t spinlock;
	mutex_t mutex;
	pthread_spinlock_t pspinlock;
	pthread_mutex_t pmutex;
	void* locks[4];
}list_t;
void list_init(list_t *list);
void list_insert(list_t *list, unsigned int key);
void list_delete(list_t *list, unsigned int key);
void *list_lookup(list_t *list, unsigned int key);
void list_free(list_t *list);

#endif
