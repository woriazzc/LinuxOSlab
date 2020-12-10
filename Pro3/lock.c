#include "lock.h"
#include <stdint.h>
#include <stdio.h>

//spinlock
void spinlock_init(spinlock_t *lock){
	lock->flag = 0;
}
void spinlock_acquire(spinlock_t *lock){
	while(xchg(&(lock->flag), 1) == 1)
		;
}
void spinlock_release(spinlock_t *lock){
	lock->flag = 0;
}

//mutex
void mutex_init(mutex_t *lock){
	lock->flag = 0;
}
void mutex_acquire(mutex_t *lock){
	if(xchg(&(lock->flag), 1) == 0){
		return;
	}
	while(1){
		if(xchg(&(lock->flag), 1) == 0)return;
		sys_futex((void*)(&(lock->flag)), FUTEX_WAIT, 1, NULL, NULL, 0);
	}
}
void mutex_release(mutex_t *lock){
	xchg(&(lock->flag), 0);
	sys_futex((void*)(&(lock->flag)), FUTEX_WAKE, 1, NULL, NULL, 0);
}


void lock_init(void *lock){
	switch(type){
		case 0:
			spinlock_init((spinlock_t*)lock);
			break;
		case 1:
			mutex_init((mutex_t*)lock);
			break;
		case 2:
			pthread_spin_init((pthread_spinlock_t*)lock, PTHREAD_PROCESS_SHARED);
			break;
		case 3:
			pthread_mutex_init((pthread_mutex_t*)lock, NULL);
			break;
	}
}
void lock_acquire(void *lock){
	switch(type){
		case 0:
			spinlock_acquire((spinlock_t*)lock);
			break;
		case 1:
			mutex_acquire((mutex_t*)lock);
			break;
		case 2:
			pthread_spin_lock((pthread_spinlock_t*)lock);
			break;
		case 3:
			pthread_mutex_lock((pthread_mutex_t*)lock);
			break;
	}
}
void lock_release(void *lock){
	switch(type){
		case 0:
			spinlock_release((spinlock_t*)lock);
			break;
		case 1:
			mutex_release((mutex_t*)lock);
			break;
		case 2:
			pthread_spin_unlock((pthread_spinlock_t*)lock);
			break;
		case 3:
			pthread_mutex_unlock((pthread_mutex_t*)lock);
			break;
	}
}
