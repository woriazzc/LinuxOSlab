#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include "counter.h"
#define MAX 1000000
#define MAXN 10
unsigned int type;
int thread_count;
counter_t counter;
struct timeval tp1,tp2;
double time1=0.0;
double time2=0.0;
double time3=0.0;
double time4=0.0;
double start,end;
double t2[21];
void* t_counter(void* rank);
int main(int argc,char* argv[])
{ 
	int i,j; 
	long thread;
	for(j=1;j<21;j++){
		time1=0.0;
		time2=0.0;
		time3=0.0;
		time4=0.0;
		pthread_t* thread_handles;
		thread_count=j;
		thread_handles=malloc(thread_count* sizeof(pthread_t));
		printf("Threads %d\n",j);
		for(type=0;type<4;type++)
		{ 
			for(i=0;i<thread_count;i++) 
				t2[i]=0.0; 
			for(i=0;i<MAXN;i++){
				counter_init(&counter,0);
				gettimeofday(&tp1,NULL);
				start=tp1.tv_sec+tp1.tv_usec/1000000.0;
				for(thread=0;thread<thread_count;thread++)
					pthread_create(&thread_handles[thread],NULL,t_counter,(void*)thread);
				for(thread=0;thread<thread_count;thread++)
					pthread_join(thread_handles[thread],NULL);

				gettimeofday(&tp1,NULL);
				end=tp1.tv_sec+tp1.tv_usec/1000000.0;
				if(type==0) time1+=end-start;
				else if(type==1) time2+=end-start;
				else if(type==2) time3+=end-start;
				else if(type==3) time4+=end-start;
			}
			if(type==0)
			{ 
				printf("-----------There is test of spinlock: time:%lfs-----------\n",(double)time1/MAXN);
				for(i=0;i<thread_count;i++)
					printf("%d %lf\n",i,t2[i]/MAXN);
			}
			else if(type==1)
			{ 
				printf("-----------There is test of mutex: time:%lfs-----------\n",(double)time2/MAXN);
				for(i=0;i<thread_count;i++)
					printf("%d %lf\n",i,t2[i]/MAXN);
			}
			else if(type==2)
			{ 
				printf("-----------There is test of pthread_spinlock: time:%lfs-----------\n",(double)time3/MAXN);
				for(i=0;i<thread_count;i++)
					printf("%d %lf\n",i,t2[i]/MAXN);
			}
			else if(type==3)
			{ 
				printf("-----------There is test of pthread_mutex: time:%lfs-----------\n",(double)time4/MAXN);
				for(i=0;i<thread_count;i++)
					printf("%d %lf\n",i,t2[i]/MAXN);

			}

		}//finish type 4
		free(thread_handles);
		printf("\n");
	}
	return 0;
}

void* t_counter(void* rank) 
{
	int i;
	long myrank=(long)rank;
	for(i=0;i<MAX;i++)
		counter_increment(&counter);
	gettimeofday(&tp2,NULL);
	t2[myrank]+=(double)tp2.tv_sec+tp2.tv_usec/1000000.0-start;
	return NULL;
}	