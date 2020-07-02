#include <pthread.h>
#include <semaphore.h>
#include <sched.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "a2_helper.h"

pthread_t threads[5];
pthread_t threads2[49];
pthread_t threads3[5];

int argumente[5] = {1,2,3,4,5};
int argumente2[49];

pthread_mutex_t m1, m2 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t c1, c2 = PTHREAD_COND_INITIALIZER;
int t3_ready = 0;
int t1_starts = 0;

sem_t semafor;
pthread_barrier_t bariera;
int t14_finished = 0;
int nr_threads = 49;

void* thFunction(void* arg) {
	int val = *(int*) arg;
	
	if(val == 1){
		pthread_mutex_lock(&m1);
		info(BEGIN, 7, val);
		t1_starts = 1;
		pthread_cond_signal(&c2);
		
		while(t3_ready == 0){
			pthread_cond_wait(&c1, &m1);
		}
		
		info(END, 7, val);
		pthread_mutex_unlock(&m1);
	}
	else if(val == 3){
		pthread_mutex_lock(&m1);
		
		while(t1_starts == 0){
			pthread_cond_wait(&c2, &m1);
		}
		
		info(BEGIN, 7, val);
		t3_ready = 1;
		info(END, 7, val);
		
		pthread_mutex_unlock(&m1);
		pthread_cond_signal(&c1);
	}
	else{
		info(BEGIN, 7, val);
		info(END, 7, val);
	}
	
	return NULL;
}

void* thFunction2(void* arg) {
	int val = *(int*) arg;
	
	sem_wait(&semafor);
	info(BEGIN, 6, val);
	
	if((nr_threads == 6 && t14_finished == 0) || val == 14){
		pthread_barrier_wait(&bariera);
	
		if(val == 14){
			info(END, 6, val);
			t14_finished = 1;
		}
	
		pthread_barrier_wait(&bariera);
	}
	
	pthread_mutex_lock(&m2);
	nr_threads--;
	pthread_mutex_unlock(&m2);
	
	if(val != 14){
		info(END, 6, val);
	}
	
	sem_post(&semafor);
	
	return NULL;
}

void* thFunction3(void* arg) {
	int val = *(int*) arg;
	
	info(BEGIN, 3, val);
	
	info(END, 3, val);
	
	return NULL;
}

int main(int argc, char **argv){
    init();

    info(BEGIN, 1, 0);

	pid_t pid2, pid3, pid4, pid5, pid6, pid7;
	
	pid2 = fork();
	if(pid2 == 0){
		info(BEGIN, 2, 0);
		
		pid3 = fork();
		if(pid3 == 0){
			info(BEGIN, 3, 0);
			
			pid4 = fork();
			if(pid4 == 0){
				info(BEGIN, 4, 0);
				info(END, 4, 0);
			}
			else if(pid4 > 0){
				
				pid7 = fork();
				if(pid7 == 0){
					info(BEGIN, 7, 0);
					
					for(int i = 0; i < 5; i++){
						pthread_create(&threads[i], NULL, thFunction, &argumente[i]);
					}
					
					for(int i = 0; i < 5; i++){
						pthread_join(threads[i], NULL);
					}
					
					info(END, 7, 0);
				}
				else if(pid7 > 0){
					
					for(int i = 0; i < 5; i++){
						pthread_create(&threads3[i], NULL, thFunction3, &argumente[i]);
					}
					
					for(int i = 0; i < 5; i++){
						pthread_join(threads3[i], NULL);
					}
					
					wait(NULL);
					wait(NULL);
					info(END, 3, 0);
				}
			}
		}
		else if(pid3 > 0){
			
			pid5 = fork();
			if(pid5 == 0){
				info(BEGIN, 5, 0);
				
				pid6 = fork();
				if(pid6 == 0){
					info(BEGIN, 6, 0);
					
					for(int i = 0; i < 49; i++){
						argumente2[i] = i + 1;
					}
					
					sem_init(&semafor, 0, 6);
					pthread_barrier_init(&bariera, NULL, 6);
					
					for(int i = 0; i < 49; i++){
						pthread_create(&threads2[i], NULL, thFunction2, &argumente2[i]);
					}
					
					for(int i = 0; i < 49; i++){
						pthread_join(threads2[i], NULL);
					}
					
					info(END, 6, 0);
				}
				else if(pid6 > 0){
					wait(NULL);
					info(END, 5, 0);
				}
			}
			else if(pid5 > 0){
				wait(NULL);
				wait(NULL);
				info(END, 2, 0);
			}
		}
	}
	else if(pid2 > 0){
		wait(NULL);
		info(END, 1, 0);
	}
    
    return 0;
}
