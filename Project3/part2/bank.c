 #include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include "account.h"
#include "string_parser.h"

#define NUM_WORKERS 10
pthread_t *thread_ids;
pthread_t bank_thread;


account *accounts = NULL;
int account_nums = 0;

int total_lines = 0;
int skipped_lines = 0;
int lines_per_threads = 0;

char *filename = "";

int numbers[NUM_WORKERS] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};


void print_accounts() {
    for (int i = 0; i < account_nums; i++) {
        printf("Account %d:\n", i);
        printf("  Account Number: %s", accounts[i].account_number);  // `strncpy` will add '\0', so no extra newline needed
        printf("  Password: %s\n", accounts[i].password);
        printf("  Balance: %.2f\n", accounts[i].balance);
        printf("  Reward Rate: %.3f\n", accounts[i].reward_rate);
        printf("  Transaction Tracker: %.2f\n\n", accounts[i].transaction_tracter);
    }
}

void print_final_balances(const char *output_filename) {
    FILE *outFPtr = fopen(output_filename, "w");
    if (outFPtr == NULL) {
        fprintf(stderr, "Error: Could not open output file %s\n", output_filename);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < account_nums; i++) {
        // Use tab after "balance:" and format to two decimal places
        fprintf(outFPtr, "%d balance:\t%.2f\n\n", i, accounts[i].balance);
    }

    fclose(outFPtr);
}

int count_total_lines(FILE *file) {
    int line_count = 0;
    size_t len = 0;
    char *line_buf = NULL;

    // Read each line and increment the count
    while (getline(&line_buf, &len, file) != -1) {
        line_count++;
    }

    // Free the buffer used by getline
    free(line_buf);

    // Reset file pointer to the beginning for future use
    rewind(file);

    return line_count;
}

void *update_balance(void* arg){

	// Apply rewards to all accounts
	for (int i = 0; i < account_nums; i++){
        pthread_mutex_lock(&(accounts[i].ac_lock));

		// Update balance based on reward rate and transaction tracker
        double reward = accounts[i].transaction_tracter * accounts[i].reward_rate;

        // Add reward to balance
        accounts[i].balance += reward;

        // Reset the transaction tracker after applying the reward
        accounts[i].transaction_tracter = 0.0;
        pthread_mutex_unlock(&(accounts[i].ac_lock));
	}

    pthread_exit(NULL);
}


void *process_transaction(void* arg) {
    command_line large_token_buffer;

    // cast back to the type we are expecting!
    int *id = (int *)arg;

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

    // Find starting pointer = id * 12000 + 51
    int starting = (*id * lines_per_threads) + skipped_lines;
    int ending = starting + 12000;

    // Loop and get pointer to correct line
    for(int i = 0; i < starting; i++){
        getline (&line_buf, &len, inFPtr);
    }

    // Loop from start pointer to end pointer
    for(int i = starting; i < ending; i++){
        getline (&line_buf, &len, inFPtr);

        large_token_buffer = str_filler (line_buf, " ");

        // Determine transaction type by the first token
        char *transaction_type = large_token_buffer.command_list[0];

        // Transfer funds
        if (strcmp(transaction_type, "T") == 0) {
            char *src_account = large_token_buffer.command_list[1];
            char *password = large_token_buffer.command_list[2];
            char *dest_account = large_token_buffer.command_list[3];
            double transfer_amount = atof(large_token_buffer.command_list[4]);

            // Find source account and destination account
            account *src = NULL;
            account *dest = NULL;
            int src_index = 0;
            int dst_index = 0;
            for (int i = 0; i < account_nums; i++) {
                if (strcmp(accounts[i].account_number, src_account) == 0) {
                    src = &accounts[i];
                    src_index = i;
                }
                if (strcmp(accounts[i].account_number, dest_account) == 0) {
                    dest = &accounts[i];
                    dst_index = i;
                }
            }

            // Grab lock here for smallest index first
            if (src_index < dst_index){
                pthread_mutex_lock(&(accounts[src_index].ac_lock));
                pthread_mutex_lock(&(accounts[dst_index].ac_lock));
            } else{
                pthread_mutex_lock(&(accounts[dst_index].ac_lock));
                pthread_mutex_lock(&(accounts[src_index].ac_lock));
            }

            // Verify password and perform transfer if accounts are found
            if (src && dest && strcmp(src->password, password) == 0) {
                src->balance -= transfer_amount;
                dest->balance += transfer_amount;
                src->transaction_tracter += transfer_amount;
            }

            pthread_mutex_unlock(&(accounts[src_index].ac_lock));
            pthread_mutex_unlock(&(accounts[dst_index].ac_lock));

        // Check balance
        } else if (strcmp(transaction_type, "C") == 0) {
            char *account_num = large_token_buffer.command_list[1];
            char *password = large_token_buffer.command_list[2];

            // Find the account
            account *acc = NULL;
            for (int i = 0; i < account_nums; i++) {
                if (strcmp(accounts[i].account_number, account_num) == 0) {
                    acc = &accounts[i];
                    break;
                }
            }
            

       	// Deposit
        } else if (strcmp(transaction_type, "D") == 0) {
            char *account_num = large_token_buffer.command_list[1];
            char *password = large_token_buffer.command_list[2];
            double amount = atof(large_token_buffer.command_list[3]);

            // Find the account
            int acc_index = 0;
            account *acc = NULL;
            for (int i = 0; i < account_nums; i++) {
                if (strcmp(accounts[i].account_number, account_num) == 0) {
                    pthread_mutex_lock(&(accounts[i].ac_lock));
                    acc = &accounts[i];
                    acc_index = i;
                    break;
                }
            }

            // Verify password and perform deposit
            if (acc && strcmp(acc->password, password) == 0) {
                acc->balance += amount;
                acc->transaction_tracter += amount;
            }
            pthread_mutex_unlock(&(accounts[acc_index].ac_lock));

      	// Withdraw
        } else if (strcmp(transaction_type, "W") == 0) {
            char *account_num = large_token_buffer.command_list[1];
            char *password = large_token_buffer.command_list[2];
            double amount = atof(large_token_buffer.command_list[3]);

            // Find the account
            int acc_index = 0;
            account *acc = NULL;
            for (int i = 0; i < account_nums; i++) {
                if (strcmp(accounts[i].account_number, account_num) == 0) {
                    pthread_mutex_lock(&(accounts[i].ac_lock));
                    acc = &accounts[i];
                    acc_index = i;
                    break;
                }
            }

            // Verify password and perform withdrawal
            if (acc && strcmp(acc->password, password) == 0) {
                acc->balance -= amount;
                acc->transaction_tracter += amount;
            }
            pthread_mutex_unlock(&(accounts[acc_index].ac_lock));
        }
    }

    free_command_line(&large_token_buffer);
    fclose(inFPtr);
    pthread_exit(NULL);
}


void file_mode(){
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

    // Find the total lines in the file
    total_lines = count_total_lines(inFPtr);

    // Find number of skipped lines
    skipped_lines = (account_nums * 5) + 1;

    // Find number of transactions per thread
    lines_per_threads = (total_lines - skipped_lines) / NUM_WORKERS;

	// Read the first line to get the integer
    if (getline(&line_buf, &len, inFPtr) != -1) {
    	// Convert first line to an integer
        account_nums = atoi(line_buf);

        // Allocate memory for an array of `account` structs
	    accounts = (account *)malloc(account_nums * sizeof(account));
	    if (accounts == NULL) {
	        fprintf(stderr, "Error: Memory allocation failed\n");
	        exit(EXIT_FAILURE);
	    }
    }

    // Loop to populate the accounts array (4 lines per account)
    for (int i = 0; i < account_nums; i++) {
        // Skip index line
        getline(&line_buf, &len, inFPtr);

        // Read account number
        if (getline(&line_buf, &len, inFPtr) != -1) {
            strncpy(accounts[i].account_number, line_buf, 16);
            accounts[i].account_number[16] = '\0'; // Ensure null termination
        }

        // Read password
        if (getline(&line_buf, &len, inFPtr) != -1) {
            strncpy(accounts[i].password, line_buf, 8);
            accounts[i].password[8] = '\0'; // Ensure null termination
        }

        // Read balance
        if (getline(&line_buf, &len, inFPtr) != -1) {
            accounts[i].balance = atof(line_buf);
        }

        // Read reward rate
        if (getline(&line_buf, &len, inFPtr) != -1) {
            accounts[i].reward_rate = atof(line_buf);
        }

        // Initialize other fields
        accounts[i].transaction_tracter = 0.0;
        pthread_mutex_init(&accounts[i].ac_lock, NULL);
    }

    // // Print accounts for testing
    // print_accounts();

	int line_num = 0;

    // create thread, give job, and cast args to void pointer!
    for(int i = 0; i < NUM_WORKERS; i++){
        pthread_create(&thread_ids[i], NULL, process_transaction, (void*)&(numbers[i]));
    }

    // wait on our threads to rejoin main thread
    for (int j = 0; j < NUM_WORKERS; ++j){
        pthread_join(thread_ids[j], NULL);
    }

    // Call Update Balance
	pthread_create(&bank_thread, NULL, update_balance, NULL);

    // Wait for bank_thread
    pthread_join(bank_thread, NULL);

    // Print final balances to an output file
    print_final_balances("output.txt");

	// Close and free buffer and accounts
	free(accounts);
	fclose(inFPtr);
	free (line_buf);

}

int main(int argc, char *argv[]) {
	// Check for file mode, or interactive mode
    if (argc == 2){
        filename = argv[1];
		file_mode();
    }else{
          fprintf(stderr, "Usage: %s filename\n", argv[0]);
          return EXIT_FAILURE;
    }

	return EXIT_FAILURE;
}