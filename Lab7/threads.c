#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define NUM_WORKERS 10

pthread_t *thread_ids;

pthread_mutex_t counter_lock;

int counter = 0;

void* simulate_work(void* arg);


int main(int argc, char* argv[]){

	// create our array of threads :)
	thread_ids = (pthread_t *)malloc(sizeof(pthread_t) * NUM_WORKERS);

	// initialize the lock
	pthread_mutex_init(&counter_lock, NULL);

	// give some fake "ids"!
	int numbers[NUM_WORKERS] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

	// create thread, give job, and cast args to void pointer!
	for(int i = 0; i < NUM_WORKERS; i++){
		pthread_create(&thread_ids[i], NULL, simulate_work, (void*)&(numbers[i]));
	}

	// Waiting on our threads to join our main thread of execution
	for (int j = 0; j < NUM_WORKERS; ++j){
		pthread_join(thread_ids[j], NULL);
	}

	printf("counter = %d\n", counter);

}


void* simulate_work(void* arg){

	// cast back to the type we are expecting!
	int *numbers = (int *)arg;

	printf("Thread %d going to sleep... zzzzzzz\n", *numbers);

	sleep(1);

	// lock the critical section to prevent race condition!
	pthread_mutex_lock(&counter_lock);
	for (int j = 0; j < 100; j++)
		counter++;
	pthread_mutex_unlock(&counter_lock);	

	pthread_exit(NULL);

}



