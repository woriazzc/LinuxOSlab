#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include "hash.h"
#define MAX 1000000	//操作次数
#define T 10	//尝试次数，取平均
#define MXTHREAD 20	//最多线程数
unsigned int type;
unsigned int size = 10000;
int thread_count;
hash_t hash;
struct timeval tp1;
double start,end;
double cost[T + 5], aver[MXTHREAD + 5], vari[MXTHREAD + 5];
unsigned int sizetab[6]={10,100,1000,10000,100000,1000000};
void* t_hash1(void* rank);
void* t_hash2(void* rank);
void* t_hash3(void* rank);
pthread_t thread_handles[MXTHREAD + 5];
int i, t, j;
int main(int argc, char const *argv[])
{
	int op = (argv[1][0] - '0');
	if(op >= 4){
		thread_count = 20;
		for(type = 0; type < 4; type++){
			if(type == 0)printf("-----------Here is test of spinlock: -----------\n");
			else if(type == 1)printf("-----------Here is test of mutex: -----------\n");
			else if(type == 2)printf("-----------Here is test of pthread_spinlock: -----------\n");
			else printf("-----------Here is test of pthread_mutex: -----------\n");
			for(j = 0; j < 6; j++){
				size = sizetab[j];
				printf("size = %d\n", size);
				for(t = 0; t < T; t++){
					cost[t] = 0.0;
					hash_init(&hash, size);
					gettimeofday(&tp1,NULL);
					start = tp1.tv_sec + tp1.tv_usec / 1000000.0;
					for(i = 0; i < thread_count; i++){
						switch(op){
							case 4:
								pthread_create(&thread_handles[i],NULL,t_hash1,NULL);
								break;
							case 5:
								pthread_create(&thread_handles[i],NULL,t_hash2,NULL);
								break;
							case 6:
								pthread_create(&thread_handles[i],NULL,t_hash3,NULL);
								break;
						}
					}
					for(i = 0; i < thread_count; i++)
						pthread_join(thread_handles[i], NULL);
					gettimeofday(&tp1,NULL);
					end=tp1.tv_sec+tp1.tv_usec/1000000.0;
					cost[t] = end - start;
					hash_free(&hash);
				}
				for(t = 0; t < T; t++)aver[j] += cost[t];
				aver[j] /= T;
				for(t = 0; t < T; t++)
					vari[j] += (cost[t] - aver[j])*(cost[t] - aver[j]);
				vari[j] /= T;
			}
			for(i = 0; i < 6; i++)printf("%lf%c", aver[i], " \n"[i==5]);
			for(i = 0; i < 6; i++)printf("%lf%c", vari[i], " \n"[i==5]);
		}
		return 0;
	}
	for(type = 0; type < 4; type++){
		if(type == 0)printf("-----------Here is test of spinlock: -----------\n");
		else if(type == 1)printf("-----------Here is test of mutex: -----------\n");
		else if(type == 2)printf("-----------Here is test of pthread_spinlock: -----------\n");
		else printf("-----------Here is test of pthread_mutex: -----------\n");
		for(thread_count = 1; thread_count <= MXTHREAD; thread_count++){
			printf("thread = %d\n", thread_count);
			for(t = 0; t < T; t++){
				cost[t] = 0.0;
				hash_init(&hash, size);
				gettimeofday(&tp1,NULL);
				start=tp1.tv_sec+tp1.tv_usec/1000000.0;
				for(i = 0; i < thread_count; i++){
					switch(op){
						case 1:
							pthread_create(&thread_handles[i],NULL,t_hash1,NULL);
							break;
						case 2:
							pthread_create(&thread_handles[i],NULL,t_hash2,NULL);
							break;
						case 3:
							pthread_create(&thread_handles[i],NULL,t_hash3,NULL);
							break;
					}
				}
				for(i = 0; i < thread_count; i++)
					pthread_join(thread_handles[i], NULL);
				gettimeofday(&tp1,NULL);
				end=tp1.tv_sec+tp1.tv_usec/1000000.0;
				cost[t] = end - start;
				hash_free(&hash);
			}
			for(t = 0; t < T; t++)aver[thread_count] += cost[t];
			aver[thread_count] /= T;
			for(t = 0; t < T; t++)
				vari[thread_count] += (cost[t] - aver[thread_count])*(cost[t] - aver[thread_count]);
			vari[thread_count] /= T;
		}
		for(i = 1; i <= MXTHREAD; i++)printf("%lf%c", aver[i], " \n"[i==MXTHREAD]);
		for(i = 1; i <= MXTHREAD; i++)printf("%lf%c", vari[i], " \n"[i==MXTHREAD]);
	}
	return 0;
}
void* t_hash1(void* rank){
	int i;
	for(i=0;i<MAX;i++){
		hash_insert(&hash, i);
	}
	return NULL;
}
void* t_hash2(void* rank){
	int i;
	for(i=0;i<MAX;i++){
		hash_insert(&hash, i);
	}
	for(i=0;i<MAX;i++){
		hash_delete(&hash, i);
	}
	return NULL;
}
void* t_hash3(void* rank){
	int i;
	for(i=0;i<MAX;i++){
		hash_insert(&hash, rand()%100000);
	}
	for(i=0;i<MAX;i++){
		hash_delete(&hash, rand()%100000);
	}
	return NULL;
}