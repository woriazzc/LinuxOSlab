#include <stdio.h>
#include "counter.h"
#include "list.h"
#include "hash.h"

unsigned int type = 0;

counter_t ct;
list_t lst;
hash_t hsh;
int main(int argc, char const *argv[])
{
	if(argc > 1)
		type = (argv[1][0] - '0');
	printf("hello type %d\n", type);
	counter_init(&ct, 100);
	int i;
	for(i=0;i<10;i++){
		counter_increment(&ct);
		printf("%d\n", counter_get_value(&ct));
	}
	for(i=0;i<10;i++){
		counter_decrement(&ct);
		printf("%d\n", counter_get_value(&ct));
	}
	list_init(&lst);
	list_insert(&lst, 12);
	list_insert(&lst, 20);
	list_insert(&lst, 32);
	list_insert(&lst, 50);
	list_insert(&lst, 12);
	list_insert(&lst, 30);
	list_delete(&lst, 12);
	printf("hello\n");
	printf("%d\n", ((node_t*)list_lookup(&lst, 12))->value);
	printf("hello\n");
	hash_init(&hsh, 100);
	for(i=0;i<200;i++){
		hash_insert(&hsh, i);
	}
	for(i=0;i<101;i++){
		hash_delete(&hsh, i);
	}
	printf("%d\n", ((node_t*)hash_lookup(&hsh, 199))->value);
	return 0;
}