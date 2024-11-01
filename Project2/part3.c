#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include<sys/wait.h>
#include "string_parser.h"

#define _GNU_SOURCE

pid_t *process;
int line_num;
int curr_process;

void signal_handler(int sig){
        int i = 0;

        // Stop whats currenlty running
        kill(process[curr_process], SIGSTOP);
        


        // Loop through proccesses
        for (i = curr_process + 1; i < line_num + curr_process + 1; i++) {

                // Get mod
                int j = i%line_num;

                int ret = kill(process[j], 0); // Null signal

                if (ret == 0){
                        // Hasn't finished running
                        kill(process[j], SIGCONT);
                        break;
                }
        }

        alarm(1);

        // Update current signal
        curr_process = j;
}

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

        line_num = 0;
        curr_process = 0;

        // Count number of lines
        while (getline(&line_buf, &len, inFPtr) != -1) {
                line_count++;
        }
        process = (pid_t *)malloc(line_count * sizeof(pid_t));
        rewind(inFPtr);

        //loop until the file is over
        while (getline (&line_buf, &len, inFPtr) != -1){
                //tokenize line buffer, large token is seperated by ";"
                large_token_buffer = str_filler (line_buf, " ");

                // fork parent process
                process[line_num] = fork();

                if (process[line_num] < 0){
                        fprintf(stderr, "fork failed\n");
                        exit(-1);

                } else if (process[line_num] == 0){
                        // Child process: Set up to wait for SIGUSR1 signal
                        sigset_t sigset;
                        sigemptyset(&sigset);
                        sigaddset(&sigset, SIGCONT);
                        sigprocmask(SIG_BLOCK, &sigset, NULL);
                        int sig;
                        sigwait(&sigset, &sig);  // Wait for SIGUSR1
                        printf("Child waiting for signal...\n");
                        execvp(large_token_buffer.command_list[0], large_token_buffer.command_list);
                        exit(0);

                } else{ // Parent Process
                        signal(SIGALRM, signal_handler);
                        alarm(1);
                }

                line_num = line_num + 1;
                //free large token and reset variable
                free_command_line (&large_token_buffer);
                memset (&large_token_buffer, 0, 0);
        }

        // Wait for all child processes to complete
        while(wait(NULL) > 0);

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