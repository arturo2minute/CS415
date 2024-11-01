#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include<sys/wait.h>
#include "string_parser.h"

#define _GNU_SOURCE

void file_mode(char *filename){
        //opening file to read
        FILE *inFPtr;
        inFPtr = fopen (filename, "r");

        if (inFPtr == NULL){
                const char *error_msg = "Error: Invalid file\n";
                write(STDOUT_FILENO, error_msg, strlen(error_msg));
                exit(0);
        }

        //declear line_buffer
        size_t len = 128;
        char* line_buf = malloc (len);
        int line_count = 0;

        command_line large_token_buffer;

	int line_num = 0;

        // Count number of lines
        while (getline(&line_buf, &len, inFPtr) != -1) {
                line_count++;
        }
        pid_t *process = (pid_t *)malloc(line_count * sizeof(pid_t));
	rewind(inFPtr);

	//loop until the file is over
        while (getline (&line_buf, &len, inFPtr) != -1){
                //tokenize line buffer, large token is seperated by ";"
                large_token_buffer = str_filler (line_buf, " ");

                // fork
                process[line_num] = fork();

                if (process[line_num] < 0){
                        fprintf(stderr, "fork failed\n");
                        exit(-1);
                } else if (process[line_num] == 0){
                        // Child process: Set up to wait for SIGUSR1 signal
                        sigset_t sigset;
                        sigemptyset(&sigset);
                        sigaddset(&sigset, SIGUSR1);
			sigprocmask(SIG_BLOCK, &sigset, NULL);
                        int sig;
                        sigwait(&sigset, &sig);  // Wait for SIGUSR1

                        execvp(large_token_buffer.command_list[0], large_token_buffer.command_list);
                        exit(0);
                }
		line_num = line_num + 1;
                //free large token and reset variable
                free_command_line (&large_token_buffer);
                memset (&large_token_buffer, 0, 0);
        }

        // Make sure children dont get sent before signals get sent
        sleep(1);

        // Loop through proccess ID's and send SIGUSER1 signal to resume
        printf("Sending SIGUSR1 to all processes...\n");
	for (int i = 0; i < line_num; i++) {
                kill(process[i], SIGUSR1);
        }

        // Send SIGSTOP to all child processes to suspend them
	printf("Sending SIGSTOP to process...\n");
        for (int i = 0; i < line_num; i++) {
                kill(process[i], SIGSTOP);
        }

        // Send SIGCONT to wake each process
	printf("Sending SIGCONT to process...\n");
        for (int i = 0; i < line_num; i++) {
                kill(process[i], SIGCONT);
        }

        // Wait for all child processes to complete
        int status;
        for (int i = 0; i < line_num; i++) {
                waitpid(process[i], &status, 0);
        }

        // Close and free malloc'd memory
        free(process);
        fclose(inFPtr);
        free (line_buf);

}


int main(int argc, char* argv[]){
        // Check for file mode, or interactive mode
        if (argc == 3 && strcmp(argv[1], "-f") == 0){
                file_mode(argv[2]);
        }else{
              fprintf(stderr, "Usage: %s [-f filename\n", argv[0]);
              return EXIT_FAILURE;
        }

        return EXIT_FAILURE;
}

