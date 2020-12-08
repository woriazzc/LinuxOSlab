#ifndef __HASH_H_
#define __HASH_H_

#include "lock.h"
#include "list.h"

#define MAXHASH 100

typedef struct __hash_t
{
	int size;
	list_t hash_table[MAXHASH];
	spinlock_t spinlock;
	mutex_t mutex;
	pthread_spinlock_t pspinlock;
	pthread_mutex_t pmutex;
	void* locks[4];
}hash_t;
void hash_init(hash_t *hash, int size);
void hash_insert(hash_t *hash, unsigned int key);
void hash_delete(hash_t *hash, unsigned int key);
void *hash_lookup(hash_t *hash, unsigned int key);
void hash_free(hash_t *hash);

#endif
