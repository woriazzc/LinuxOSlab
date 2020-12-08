#include "hash.h"
#include <stdio.h>

void hash_init(hash_t *hash, int size){
	hash->size = size;
	int i;
	for(i=0;i<size;i++)
		list_init(&(hash->hash_table[i]));
	hash->locks[0]=(void*)(&(hash->spinlock));
	hash->locks[1]=(void*)(&(hash->mutex));
	hash->locks[2]=(void*)(&(hash->pspinlock));
	hash->locks[3]=(void*)(&(hash->pmutex));
	lock_init(hash->locks[type]);
}
void hash_insert(hash_t *hash, unsigned int key){
	lock_acquire(hash->locks[type]);
	list_insert(&(hash->hash_table[key%hash->size]), key);
	lock_release(hash->locks[type]);
}
void hash_delete(hash_t *hash, unsigned int key){
	lock_acquire(hash->locks[type]);
	list_delete(&(hash->hash_table[key%hash->size]), key);
	lock_release(hash->locks[type]);
}
void *hash_lookup(hash_t *hash, unsigned int key){
	void* res = NULL;
	lock_acquire(hash->locks[type]);
	res = list_lookup(&(hash->hash_table[key%hash->size]), key);
	lock_release(hash->locks[type]);
	return res;
}
void hash_free(hash_t *hash){
	int i;
	for(i=0;i<hash->size;i++){
		list_free(&(hash->hash_table[i]));
	}
}