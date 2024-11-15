 #include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include "account.h"
#include "string_parser.h"

int main(int argc, char *argv[]) {
	// Check for file mode, or interactive mode
    if (argc == 2){
		//file_mode(argv[1]);
		fprintf(stderr, "Usage: %s filename\n", argv[0]);
    }else{
          fprintf(stderr, "Usage: %s filename\n", argv[0]);
          return EXIT_FAILURE;
    }

	return EXIT_FAILURE;
}