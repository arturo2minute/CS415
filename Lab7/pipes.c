#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define NUM_WORKERS 10

pthread_t *thread_ids;

pthread_mutex_t counter_lock;

pthread_mutex_t pipe_lock;

int counter = 0;

int pipe_fd[2];		// our pipe :)

void* simulate_work(void* arg);


int main(int argc, char* argv[]){

	// create our array of threads :)
	thread_ids = (pthread_t *)malloc(sizeof(pthread_t) * NUM_WORKERS);

	// initialize the locks
	pthread_mutex_init(&counter_lock, NULL);
	pthread_mutex_init(&pipe_lock, NULL);

	// init the pipe!
	if (pipe(pipe_fd) == -1) {
        perror("pipe failed");
        exit(EXIT_FAILURE);
    }

    // give some fake "ids"!
	int numbers[NUM_WORKERS] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

	// create thread, give job, and cast args to void pointer!
	for(int i = 0; i < NUM_WORKERS; i++){
		pthread_create(&thread_ids[i], NULL, simulate_work, (void*)&(numbers[i]));
	}

	// wait on our threads to rejoin main thread
	for (int j = 0; j < NUM_WORKERS; ++j){
		pthread_join(thread_ids[j], NULL);
	}

	// close write end of pipe
	close(pipe_fd[1]);

	// Read from the pipe
	char buffer[100];
    ssize_t bytes_read;
    while ((bytes_read = read(pipe_fd[0], buffer, sizeof(buffer))) > 0)
        fwrite(buffer, 1, bytes_read, stdout);

    // lock the critical section to prevent race condition!
	pthread_mutex_destroy(&counter_lock);
	pthread_mutex_destroy(&pipe_lock);

	// close read end
	close(pipe_fd[0]);
    free(thread_ids);



}


void* simulate_work(void* arg){

	// cast back to the type we are expecting!
	int *numbers = (int *)arg;

	
	for (int j = 0; j < 100; j++){
		usleep(100);		// will cause threads to share cs more often bc ordering is more random (just here to make output make more sense and see that its not happening in serial)
		pthread_mutex_lock(&counter_lock);	// lock the critical section to prevent race condition!
		counter++;
		pthread_mutex_unlock(&counter_lock);	
	}
	
	// lock the critical section to prevent race condition!
	pthread_mutex_lock(&pipe_lock);
	char message[100];
    snprintf(message, sizeof(message), "Thread %d finished work, counter = %d\n", *numbers, counter);

    // write to the pipe
    write(pipe_fd[1], message, strlen(message));	
    pthread_mutex_unlock(&pipe_lock);	

	pthread_exit(NULL);

}










