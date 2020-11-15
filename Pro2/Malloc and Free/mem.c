#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "mem.h"

int pg_size = 0;
int m_error;
int has_inited = 0;

typedef struct node
{
	int size;
	struct node *next;
}node;

node* headfree;
int tot_size;

int mem_init(int size_of_region)
{
	m_error = 0;
	pg_size = getpagesize();
	if(size_of_region <= 0 || has_inited){
		m_error = E_BAD_ARGS;
		return -1;
	}
	size_of_region += (pg_size - size_of_region % pg_size) % pg_size;
	int fd = open("/dev/zero", O_RDWR);
	headfree = mmap(NULL, size_of_region, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if(headfree == MAP_FAILED){ perror("mmap"); exit(1); }
	close(fd);
	tot_size = size_of_region;
	headfree->size = tot_size - sizeof(node);
	has_inited = 1;
	return 0;
}


void* mem_alloc(int size, int style)
{
	m_error = 0;
	node* res = NULL;
	size += (8 - (size % 8)) % 8;
	node* p = headfree;
	switch(style){
		case M_BESTFIT:
		{
			int mn = tot_size + 10;
			while(p != NULL){
				if(p->size >= size){
					if(p->size < mn){
						mn = p->size;
						res = p;
					}
				}
				p = p->next;
			}
			break;
		}
		case M_WORSTFIT:
		{
			int mx = -1;
			while(p != NULL){
				if(p->size > mx){
					mx = p->size;
					res = p;
				}
				p = p->next;
			}
			if(res != NULL && res->size < size)res = NULL;
			break;
		}
		case M_FIRSTFIT:
		{
			while(p != NULL){
				if(p->size >= size){
					res = p;
					break;
				}
				p = p->next;
			}
			break;
		}
	}
	if(res == NULL){
		m_error = E_NO_SPACE;
		return NULL;
	}
	else{
		if(res->size > size + sizeof(node)){
			node* np = res + size + sizeof(node);
			np->size = res->size - size - 2 * sizeof(node);
			np->next = res->next;
			if(res == headfree){
				headfree = np;
			}
			else{
				p = headfree;
				while(p != NULL){
					if(p->next == res){
						p->next = np;
						break;
					}
					p = p->next;
				}
			}
		}
		else{
			if(res == headfree){
				headfree = res->next;
			}
			else{
				p = headfree;
				while(p != NULL){
					if(p->next == res){
						p->next = res->next;
						break;
					}
					p = p->next;
				}
			}
		}
		res->size = size;
		return (void*)res;
	}
}

int mem_free(void *pptr)
{
	if(pptr == NULL)return 0;
	node* ptr = (node*)pptr;
	if(ptr < headfree){
		if(headfree < ptr + ptr->size + sizeof(node)){
			return -1;
		}
		node* tmp = headfree;
		headfree = ptr;
		ptr->next = tmp;
		if(ptr->next != NULL && ptr + ptr->size + sizeof(node) == ptr->next){
			ptr->size += ptr->next->size + sizeof(node);
			ptr->next = ptr->next->next;
		}
	}
	else{
		node* p = headfree;
		while(p != NULL){
			if(p < ptr && (p->next > ptr))break;
			p = p->next;
		}
		if(p == NULL)return -1;
		if(p + p->size + sizeof(node) > ptr || (p->next != NULL && ptr + ptr->size + sizeof(node) > p->next))
			return -1;
		ptr->next = p->next;
		p->next = ptr;
		if(ptr->next != NULL && ptr + ptr->size + sizeof(node) == ptr->next){
			ptr->size += ptr->next->size + sizeof(node);
			ptr->next = ptr->next->next;
		}
		if(p + p->size + sizeof(node) == ptr){
			p->size += ptr->size + sizeof(node);
			p->next = ptr->next;
		}
	}
	return 0;
}

void mem_dump()
{
	node* p = headfree;
	while(p != NULL){
		printf("address: %p, size: %d\n", p, p->size);
		p = p->next;
	}
}
