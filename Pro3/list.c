#include "list.h"
#include <stdlib.h>
#include <stdio.h>

void list_init(list_t *list){
	list->head = NULL;
	list->locks[0]=(void*)(&(list->spinlock));
	list->locks[1]=(void*)(&(list->mutex));
	list->locks[2]=(void*)(&(list->pspinlock));
	list->locks[3]=(void*)(&(list->pmutex));
	lock_init(list->locks[type]);
}
void list_insert(list_t *list, unsigned int key){
	node_t* nw = malloc(sizeof(node_t));
	lock_acquire(list->locks[type]);
	nw->value = key;
	nw->next = list->head;
	list->head = nw;
	lock_release(list->locks[type]);
}
void list_delete(list_t *list, unsigned int key){
	lock_acquire(list->locks[type]);
	node_t* fa = NULL;
	node_t* p = list->head;
	while(p != NULL){
		if(p->value == key){
			if(p == list->head){
				list->head = p->next;
			}
			else{
				fa->next = fa->next->next;
			}
			//printf("%p\n", p);
			free(p);
			break;
		}
		fa = p;
		p = p->next;
	}
	lock_release(list->locks[type]);
}
void *list_lookup(list_t *list, unsigned int key){
	lock_acquire(list->locks[type]);
	node_t* p = list->head;
	while(p != NULL){
		if(p->value == key){
			lock_release(list->locks[type]);
			return (void*)p;
		}
		p = p->next;
	}
	lock_release(list->locks[type]);
	return NULL;
}
void list_free(list_t *list){
	node_t* p = list->head;
	node_t* f;
	while(p != NULL){
		f = p;
		p = p->next;
		free(f);
	}
}