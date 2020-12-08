#ifndef __COUNTER_H_
#define __COUNTER_H_

#include "lock.h"

typedef struct __counter_t
{
	int value;
	spinlock_t spinlock;
	mutex_t mutex;
	pthread_spinlock_t pspinlock;
	pthread_mutex_t pmutex;
	void* locks[4];
}counter_t;

void counter_init(counter_t *c, int value);
int counter_get_value(counter_t *c);
void counter_increment(counter_t *c);
void counter_decrement(counter_t *c);

#endif
