#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "string_parser.h"
#include "command.h"

#define _GNU_SOURCE

void interactive_mode(){

	//declear line_buffer
        size_t len = 128;
        char* line_buf = malloc (len);

	command_line large_token_buffer;
        command_line small_token_buffer;


        while(1){
                printf(">>> ");

                getline(&line_buf, &len, stdin);

		large_token_buffer = str_filler (line_buf, ";");

		//iterate through each large token
                for (int i = 0; large_token_buffer.command_list[i] != NULL; i++){
                        //smaller token is seperated by " "(space bar)
                        small_token_buffer = str_filler (large_token_buffer.command_list[i], " ");

                        //iterate through each smaller token to print
			int numToke = 0;
                        for (int j = 0; small_token_buffer.command_list[j] != NULL; j++){
                                numToke += 1;
                        }

			if (small_token_buffer.command_list[0] == NULL){
				free_command_line(&small_token_buffer);
				break;
			}

			// If chain to find command with small_token_buffer.command_list[0]
			if (strcmp(small_token_buffer.command_list[0], "exit") == 0){
                       		free_command_line(&small_token_buffer);
                        	memset (&small_token_buffer, 0, 0);
                		free_command_line(&large_token_buffer);
                		memset (&large_token_buffer, 0, 0);
				free(line_buf);
				exit(0);
			} else if(strcmp(small_token_buffer.command_list[0], "ls") == 0){
				if (numToke == 1){
					listDir();
				} else{
					printf ("Error: Unsupported parameters for command: ls\n");
				}
			} else if(strcmp(small_token_buffer.command_list[0], "pwd") == 0){
				if (numToke == 1){
					showCurrentDir();
				} else{
					printf ("Error: Unsupported parameters for command: pwd\n");
				}
			} else if(strcmp(small_token_buffer.command_list[0], "mkdir") == 0){
				if (numToke == 2){
					makeDir(small_token_buffer.command_list[1]);
				} else{
					printf ("Error: Unsupported parameters for command: mkdir\n");
				}
			} else if(strcmp(small_token_buffer.command_list[0], "cd") == 0){
                                if (numToke == 2){
                                        changeDir(small_token_buffer.command_list[1]);
                                } else{
                                        printf ("Error: Unsupported parameters for command: cd\n");
                                }
                        } else if(strcmp(small_token_buffer.command_list[0], "cp") == 0){
                                if (numToke == 3){
                                        copyFile(small_token_buffer.command_list[1], small_token_buffer.command_list[2]);
                                } else{
                                        printf ("Error: Unsupported parameters for command: cp\n");
                                }
                        } else if(strcmp(small_token_buffer.command_list[0], "mv") == 0){
                                if (numToke == 3){
                                        moveFile(small_token_buffer.command_list[1], small_token_buffer.command_list[2]);
                                } else{
                                        printf ("Error: Unsupported parameters for command: mv\n");
                                }
                        } else if(strcmp(small_token_buffer.command_list[0], "rm") == 0){
                                if (numToke == 2){
                                        deleteFile(small_token_buffer.command_list[1]);
                                } else{
                                        printf ("Error: Unsupported parameters for command: rm\n");
                                }
			} else if(strcmp(small_token_buffer.command_list[0], "cat") == 0){
                                if (numToke == 2){
                                        displayFile(small_token_buffer.command_list[1]);
                                } else{
                                        printf ("Error: Unsupported parameters for command: rm\n");
                                }
                        } else{
				printf ("Error: Unrecognized command: %s\n", small_token_buffer.command_list[0]);
			}
			//free smaller tokens and reset variable
                        free_command_line(&small_token_buffer);
                        memset (&small_token_buffer, 0, 0);
                }
		//free large token and reset variable
               	free_command_line(&large_token_buffer);
              	memset (&large_token_buffer, 0, 0);
        }

        free(line_buf);
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

	freopen("output.txt", "w+", stdout);

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

			//iterate through each smaller token to print
                        int numToke = 0;
                        for (int j = 0; small_token_buffer.command_list[j] != NULL; j++){
                                numToke += 1;
                        }

                        if (small_token_buffer.command_list[0] == NULL){
				free_command_line(&small_token_buffer);
                                break;
                        }

                        // If chain to find command with small_token_buffer.command_list[0]
                        if (strcmp(small_token_buffer.command_list[0], "exit") == 0){
                                free_command_line(&small_token_buffer);
                                memset (&small_token_buffer, 0, 0);
				free_command_line (&large_token_buffer);
                		memset (&large_token_buffer, 0, 0);
                                free(line_buf);
				fclose(inFPtr);
                                exit(0);
                        } else if(strcmp(small_token_buffer.command_list[0], "ls") == 0){
                                if (numToke == 1){
                                        listDir();
                                } else{
					const char *error_msg = "Error: Unsupported parameters for command: ls\n";
                                        write(STDOUT_FILENO, error_msg, strlen(error_msg));
                                }
                        } else if(strcmp(small_token_buffer.command_list[0], "pwd") == 0){
                                if (numToke == 1){
                                        showCurrentDir();
                                } else{
                                        const char *error_msg = "Error: Unsupported parameters for command: pwd\n";
					write(STDOUT_FILENO, error_msg, strlen(error_msg));
                                }
                        } else if(strcmp(small_token_buffer.command_list[0], "mkdir") == 0){
                                if (numToke == 2){
                                        makeDir(small_token_buffer.command_list[1]);
                                } else{
                                        const char *error_msg = "Error: Unsupported parameters for command: mkdir\n";
					write(STDOUT_FILENO, error_msg, strlen(error_msg));
                                }
                        } else if(strcmp(small_token_buffer.command_list[0], "cd") == 0){
                                if (numToke == 2){
                                        changeDir(small_token_buffer.command_list[1]);
                                } else{
                                        const char *error_msg = "Error: Unsupported parameters for command: cd\n";
                                	write(STDOUT_FILENO, error_msg, strlen(error_msg));
				}
			} else if(strcmp(small_token_buffer.command_list[0], "cp") == 0){
                                if (numToke == 3){
                                        copyFile(small_token_buffer.command_list[1], small_token_buffer.command_list[2]);
                                } else{
                                        const char *error_msg = "Error: Unsupported parameters for command: cp\n";
                                	write(STDOUT_FILENO, error_msg, strlen(error_msg));
				}
                        } else if(strcmp(small_token_buffer.command_list[0], "mv") == 0){
                                if (numToke == 3){
                                        moveFile(small_token_buffer.command_list[1], small_token_buffer.command_list[2]);
                                } else{
                                        const char *error_msg = "Error: Unsupported parameters for command: mv\n";
                                	write(STDOUT_FILENO, error_msg, strlen(error_msg));
				}
                        } else if(strcmp(small_token_buffer.command_list[0], "rm") == 0){
                                if (numToke == 2){
                                        deleteFile(small_token_buffer.command_list[1]);
                                } else{
                                        const char *error_msg = "Error: Unsupported parameters for command: rm\n";
                                	write(STDOUT_FILENO, error_msg, strlen(error_msg));
				}
                        } else if(strcmp(small_token_buffer.command_list[0], "cat") == 0){
                                if (numToke == 2){
                                        displayFile(small_token_buffer.command_list[1]);
                                } else{
                                        const char *error_msg = "Error: Unsupported parameters for command: rm\n";
                                	write(STDOUT_FILENO, error_msg, strlen(error_msg));
				}
			}  else{
                           	const char *error_msg = "Error: Unrecognized command: ";
				size_t total_len = strlen(error_msg) + strlen(small_token_buffer.command_list[0]) + 2;
				char *full_error_msg = (char *)malloc(total_len);
				sprintf(full_error_msg, "%s%s\n", error_msg, small_token_buffer.command_list[0]);
                        	write(STDOUT_FILENO, full_error_msg, strlen(full_error_msg));
				free(full_error_msg);
			}

			//free smaller tokens and reset variable
			free_command_line(&small_token_buffer);
			memset (&small_token_buffer, 0, 0);
		}

		//free large token and reset variable
		free_command_line (&large_token_buffer);
		memset (&large_token_buffer, 0, 0);
	}

	// Close and free buffer
	fclose(inFPtr);
	free (line_buf);

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

