#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "string_parser.h"
#include "command.h"

#define _GNU_SOURCE

char *trim(char *str){
	char *end;

	// Trim leading space
	while(isspace((unsigned char)*str)) str++;

	// If all spaces, return empty str
	if (*str == 0){
		return str;
	}

	// Trim trailing space
	end = str + strlen(str) - 1;
	while(end > str && isspace((unsigned char)*end)) end--;

	// Write new null terminator
	*(end + 1) = '\0';

	return str;
}

void execute_command(const char *cmd){
	char *command = strdup(cmd);
	char *token = strtok(command, " ");

	//printf("%s\n", token);
	//return;
	if (strcmp(token, "exit") == 0){
		free(command);
		exit(0);

	} else if(strcmp(token, "ls") == 0){
		ls();

	} else if(strcmp(token, "pwd") == 0){
		pwd();

	} else if(strcmp(token, "mkdir") == 0){
		token = strtok(NULL, " ");
		if (token != NULL){
			mmkdir(token);
		} else{
			fprintf(stderr, "mkdir error: missing directory name\n");
		}
	} else if(strcmp(token, "cd") == 0){
		token = strtok(NULL, " ");
                if (token != NULL){
                        cd(token);
                } else{
                        fprintf(stderr, "cd error: missing path\n");
                }
	} else if(strcmp(token, "rm") == 0){
		token = strtok(NULL, " ");
                if (token != NULL){
                        rm(token);
                } else{
                        fprintf(stderr, "rm error: missing filename\n");
                }
	} else if(strcmp(token, "cat") == 0){
		token = strtok(NULL, " ");
                if (token != NULL){
                        cat(token);
                } else{
                        fprintf(stderr, "cat error: missing file name\n");
                }
	} else if(strcmp(token, "mv") == 0){
		char *src = strtok(NULL, " ");
		char *dst = strtok(NULL, " ");
                if (src != NULL && dst != NULL){
                        mv(src, dst);
                } else{
                        fprintf(stderr, "mv error: missing arguments\n");
                }
	} else if(strcmp(token, "cp") == 0){
                char *src = strtok(NULL, " ");
                char *dst = strtok(NULL, " ");
                if (src != NULL && dst != NULL){
                        cp(src, dst);
                } else{
                        fprintf(stderr, "cp error: missing arguments\n");
                }
	} else{
		fprintf(stderr, "Unknown command: %s\n", token);
	}

	free(command);
}

void file_mode(char *filename){

	//opening file to read
	FILE *inFPtr;
	inFPtr = fopen (filename, "r");

	//declear line_buffer
	size_t len = 128;
	char* line_buf = malloc (len);

	command_line large_token_buffer;
	command_line small_token_buffer;

	int line_num = 0;

	//loop until the file is over
	while (getline (&line_buf, &len, inFPtr) != -1)
	{
		printf ("Line %d:\n", ++line_num);

		//tokenize line buffer
		//large token is seperated by ";"
		large_token_buffer = str_filler (line_buf, ";");
		//iterate through each large token
		for (int i = 0; large_token_buffer.command_list[i] != NULL; i++)
		{
			printf ("\tLine segment %d:\n", i + 1);

			//tokenize large buffer
			//smaller token is seperated by " "(space bar)
			small_token_buffer = str_filler (large_token_buffer.command_list[i], " ");

			//iterate through each smaller token to print
			for (int j = 0; small_token_buffer.command_list[j] != NULL; j++)
			{
				printf ("\t\tToken %d: %s\n", j + 1, small_token_buffer.command_list[j]);
			}

			//free smaller tokens and reset variable
			free_command_line(&small_token_buffer);
			memset (&small_token_buffer, 0, 0);
		}

		//free smaller tokens and reset variable
		free_command_line (&large_token_buffer);
		memset (&large_token_buffer, 0, 0);
	}
	fclose(inFPtr);
	//free line buffer
	free (line_buf);
}

void interactive_mode(){
	//ls();
	//pwd();
	//mmkdir("testDIR");
	//cd("..");
	//cp("README.txt", "test/test.txt");
	//mv("README.txt", "test.txt");
	//rm("test/test.txt");
	//cat("test/test.txt");

	char *line = NULL;
	size_t len = 0;
	ssize_t read;

	while(1){
		printf(">>> ");

		read = getline(&line, &len, stdin);
		if (read == -1){
			free(line);
			break;
		}

		// Remove trailing newline character
		if (line[read - 1] == '\n'){
			line[read - 1] = '\0';
		}

		// Split commands by semicolon (;)
		char *cmd = strtok(line, ";");
		while (cmd != NULL){
			cmd = trim(cmd);

			if (strlen(cmd) > 0){
				//printf("%s\n", cmd);
				execute_command(cmd);
			}

			cmd = strtok(NULL, ";");
		}
	}

	free(line);
}

int main(int argc, char* argv[]){
        // Check for file mode, or interactive mode
        if (argc == 3 && strcmp(argv[1], "-f") == 0){
              file_mode(argv[2]);
        }else if (argc == 1){
              interactive_mode();
        }else{
              fprintf(stderr, "Usage: %s [-f filename\n", argv[0]);
              return EXIT_FAILURE;
        }

        return EXIT_FAILURE;
}

