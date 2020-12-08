#ifndef __LOCK_H_
#define __LOCK_H_

#include <pthread.h>
#include <stdint.h>
#define FUTEX_WAIT 0
#define FUTEX_WAKE 1

extern unsigned int type;

unsigned int xchg(volatile unsigned int *addr, unsigned int newval);
long sys_futex(void *addr1, int op, int val1, struct timespec *timeout, void *addr2, int val3);

//spinlock
typedef struct __spinlock_t
{
	unsigned int flag;
}spinlock_t;
void spinlock_init(spinlock_t *lock);
void spinlock_acquire(spinlock_t *lock);
void spinlock_release(spinlock_t *lock);


//mutex
typedef struct __mutex_t
{
	unsigned int flag;
}mutex_t;
void mutex_init(mutex_t *lock);
void mutex_acquire(mutex_t *lock);
void mutex_release(mutex_t *lock);


void lock_init(void *lock);
void lock_acquire(void *lock);
void lock_release(void *lock);

#endif
