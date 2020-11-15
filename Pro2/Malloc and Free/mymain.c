#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "mem.h"

typedef struct node
{
	int size;
	struct node *next;
}node;

int main(int argc, char const *argv[]){
	assert(mem_init(1000)==0);
	mem_dump();
	node* p=(node*)mem_alloc(100,M_BESTFIT);
	printf("address: %p, size: %d\n", p, p->size);
	mem_dump();
	assert(p!=NULL);
	assert(mem_free(p)==0);
	return 0;
}