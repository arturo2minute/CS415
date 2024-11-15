 #include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include "account.h"
#include "string_parser.h"

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

	// Read the first line to get the integer
    if (getline(&line_buf, &len, inFPtr) != -1) {
    	// Convert first line to an integer
        int account_nums = atoi(line_buf);

        // Allocate memory for an array of `account` structs
	    account *accounts = (account *)malloc(account_nums * sizeof(account));
	    if (accounts == NULL) {
	        fprintf(stderr, "Error: Memory allocation failed\n");
	        exit(EXIT_FAILURE);
	    }
    }

	command_line large_token_buffer;
	command_line small_token_buffer;

	int line_num = 0;

	//loop until the file is over
	while (getline (&line_buf, &len, inFPtr) != -1){
		//tokenize line buffer, large token is seperated by ";"
		large_token_buffer = str_filler (line_buf, ";");

		//iterate through each large token
		for (int i = 0; large_token_buffer.command_list[i] != NULL; i++){
			//tokenize large buffer, smaller token is seperated by " "(space bar)
			small_token_buffer = str_filler (large_token_buffer.command_list[i], " ");

			// free if null
			if (small_token_buffer.command_list[0] == NULL){
				free_command_line(&small_token_buffer);
               	continue;
           	}

           	// TODO:
           	

           	//free smaller tokens and reset variable
			free_command_line(&small_token_buffer);
			memset (&small_token_buffer, 0, 0);

		}

		//free large token and reset variable
		free_command_line (&large_token_buffer);
		memset (&large_token_buffer, 0, 0);

	}

	// Close and free buffer and accounts
	free(accounts);
	fclose(inFPtr);
	free (line_buf);

}

int main(int argc, char *argv[]) {
	// Check for file mode, or interactive mode
    if (argc == 2){
		file_mode(argv[1]);
		fprintf(stderr, "Usage: %s filename\n", argv[0]);
    }else{
          fprintf(stderr, "Usage: %s filename\n", argv[0]);
          return EXIT_FAILURE;
    }

	return EXIT_FAILURE;
}