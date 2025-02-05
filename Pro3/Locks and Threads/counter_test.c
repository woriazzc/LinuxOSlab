#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include "counter.h"
#define MAX 10000	//操作次数
#define T 10	//尝试次数，取平均
#define MXTHREAD 20	//最多线程数
unsigned int type;
int thread_count;
counter_t counter;
struct timeval tp1;
struct timeval tmp;
double start,end;
double aver[MXTHREAD + 10], vari[MXTHREAD + 10], cost[MXTHREAD + 10];
void* t_counter(void* rank);
pthread_t thread_handles[MXTHREAD + 10];
int i, t, j;
int main(int argc, char const *argv[])
{
	for(type = 0; type < 4; type++){
		if(type == 0)printf("-----------Here is test of spinlock: -----------\n");
		else if(type == 1)printf("-----------Here is test of mutex: -----------\n");
		else if(type == 2)printf("-----------Here is test of pthread_spinlock: -----------\n");
		else printf("-----------Here is test of pthread_mutex: -----------\n");
		for(thread_count = 1; thread_count <= MXTHREAD; thread_count++){
			aver[thread_count] = vari[thread_count] = 0.0;
			printf("thread = %d\n", thread_count);
			for(t = 0; t < T; t++){
				counter_init(&counter, 0);
				gettimeofday(&tp1,NULL);
				start=tp1.tv_sec+tp1.tv_usec/1000000.0;
				for(i = 0; i < thread_count; i++)
					pthread_create(&thread_handles[i],NULL,t_counter,(void*)i);
				for(i = 0; i < thread_count; i++)
					pthread_join(thread_handles[i], NULL);
				gettimeofday(&tp1,NULL);
				end=tp1.tv_sec+tp1.tv_usec/1000000.0;
				aver[thread_count] += end - start;
				double sum = 0.0;
				for(i = 0; i < thread_count; i++)
					sum += cost[i];
				sum /= thread_count;
				double var = 0.0;
				for(i = 0; i < thread_count; i++){
					var += (cost[i] - sum)*(cost[i] - sum);
				}
				var /= thread_count;
				vari[thread_count] += var;
			}
			aver[thread_count] /= T;
			vari[thread_count] /= T;
		}
		for(i = 1; i <= MXTHREAD; i++)printf("%lf%c", aver[i], " \n"[i==MXTHREAD]);
		for(i = 1; i <= MXTHREAD; i++)printf("%lf%c", vari[i], " \n"[i==MXTHREAD]);
	}
	return 0;
}
void* t_counter(void* rank){
	int i;
	int rk = (int)rank;
	double mstart, mend;
	gettimeofday(&tmp,NULL);
	mstart=tmp.tv_sec+tmp.tv_usec/1000000.0;
	for(i=0;i<MAX;i++){
		counter_increment(&counter);
	}
	gettimeofday(&tmp,NULL);
	mend=tmp.tv_sec+tmp.tv_usec/1000000.0;
	cost[rk] = mend - mstart;
	return NULL;
}