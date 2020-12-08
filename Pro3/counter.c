#include "counter.h"

void counter_init(counter_t *c, int value){
	c->value = value;
	c->locks[0]=(void*)(&(c->spinlock));
	c->locks[1]=(void*)(&(c->mutex));
	c->locks[2]=(void*)(&(c->pspinlock));
	c->locks[3]=(void*)(&(c->pmutex));
	lock_init((c->locks[type]));
}
int counter_get_value(counter_t *c){
	int res;
	lock_acquire(c->locks[type]);
	res = c->value;
	lock_release(c->locks[type]);
	return res;
}
void counter_increment(counter_t *c){
	lock_acquire(c->locks[type]);
	c->value++;
	lock_release(c->locks[type]);
}
void counter_decrement(counter_t *c){
	lock_acquire(c->locks[type]);
	c->value--;
	lock_release(c->locks[type]);
}