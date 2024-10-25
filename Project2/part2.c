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

        command_line large_token_buffer;

	int line_num = 0;

        //loop until the file is over
        while (getline (&line_buf, &len, inFPtr) != -1){
                //tokenize line buffer, large token is seperated by ";"
                large_token_buffer = str_filler (line_buf, " ");

                // fork
                pid_t process = fork();
                if (process < 0){
                        fprintf(stderr, "fork failed\n");
                        exit(-1);
                } else if (process == 0){
                        execvp(large_token_buffer.command_list[0], large_token_buffer.command_list);
                        exit(0);
                }

                //free large token and reset variable
                free_command_line (&large_token_buffer);
                memset (&large_token_buffer, 0, 0);
        }

        while(wait(NULL) > 0);

        // Close and free buffer
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

