 #include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include "account.h"
#include "string_parser.h"

void print_accounts(account *accounts, int account_nums) {
    for (int i = 0; i < account_nums; i++) {
        printf("Account %d:\n", i);
        printf("  Account Number: %s", accounts[i].account_number);  // `strncpy` will add '\0', so no extra newline needed
        printf("  Password: %s\n", accounts[i].password);
        printf("  Balance: %.2f\n", accounts[i].balance);
        printf("  Reward Rate: %.3f\n", accounts[i].reward_rate);
        printf("  Transaction Tracker: %.2f\n\n", accounts[i].transaction_tracter);
    }
}


void process_transaction(command_line large_token_buffer, account *accounts, int account_nums) {
    // Ensure the first token exists (transaction type)
    if (large_token_buffer.command_list[0] == NULL) {
        return;
    }

    // Determine transaction type by the first token
    char *transaction_type = large_token_buffer.command_list[0];


    // Transfer funds
    if (strcmp(transaction_type, "T") == 0) {
        char *src_account = large_token_buffer.command_list[1];
        char *password = large_token_buffer.command_list[2];
        char *dest_account = large_token_buffer.command_list[3];
        double transfer_amount = atof(large_token_buffer.command_list[4]);

        // // Print transfer details
        // printf("Transfer Transaction:\n");
        // printf("  Source Account: %s\n", src_account);
        // printf("  Destination Account: %s\n", dest_account);
        // printf("  Amount: %.2f\n", transfer_amount);

        // Find source account and destination account
        account *src = NULL;
        account *dest = NULL;
        for (int i = 0; i < account_nums; i++) {
            if (strcmp(accounts[i].account_number, src_account) == 0) {
                src = &accounts[i];
            }
            if (strcmp(accounts[i].account_number, dest_account) == 0) {
                dest = &accounts[i];
            }
        }

        // Verify password and perform transfer if accounts are found
        if (src && dest && strcmp(src->password, password) == 0) {
            pthread_mutex_lock(&src->ac_lock);
            pthread_mutex_lock(&dest->ac_lock);
            src->balance -= transfer_amount;
            dest->balance += transfer_amount;
            src->transaction_tracter += transfer_amount;
            pthread_mutex_unlock(&src->ac_lock);
            pthread_mutex_unlock(&dest->ac_lock);
        } else {
            printf("Transfer failed: invalid account or password\n");
        }

    // Check balance
    } else if (strcmp(transaction_type, "C") == 0) {
        char *account_num = large_token_buffer.command_list[1];
        char *password = large_token_buffer.command_list[2];

        // // Print check balance details
        // printf("Check Balance Transaction:\n");
        // printf("  Account: %s\n", account_num);

        // Find the account
        account *acc = NULL;
        for (int i = 0; i < account_nums; i++) {
            if (strcmp(accounts[i].account_number, account_num) == 0) {
                acc = &accounts[i];
                break;
            }
        }

        // Verify password and display balance
        if (acc && strcmp(acc->password, password) == 0) {
            pthread_mutex_lock(&acc->ac_lock);
            printf("Balance for account %s: %.2f\n", acc->account_number, acc->balance);
            pthread_mutex_unlock(&acc->ac_lock);
        } else {
            printf("Check balance failed: invalid account or password\n");
        }

   	// Deposit
    } else if (strcmp(transaction_type, "D") == 0) {
        char *account_num = large_token_buffer.command_list[1];
        char *password = large_token_buffer.command_list[2];
        double amount = atof(large_token_buffer.command_list[3]);

        // // Print deposit details
        // printf("Deposit Transaction:\n");
        // printf("  Account: %s\n", account_num);
        // printf("  Amount: %.2f\n", amount);

        // Find the account
        account *acc = NULL;
        for (int i = 0; i < account_nums; i++) {
            if (strcmp(accounts[i].account_number, account_num) == 0) {
                acc = &accounts[i];
                break;
            }
        }

        // Verify password and perform deposit
        if (acc && strcmp(acc->password, password) == 0) {
            pthread_mutex_lock(&acc->ac_lock);
            acc->balance += amount;
            acc->transaction_tracter += amount;
            pthread_mutex_unlock(&acc->ac_lock);
        } else {
            printf("Deposit failed: invalid account or password\n");
        }

  	// Withdraw
    } else if (strcmp(transaction_type, "W") == 0) {
        char *account_num = large_token_buffer.command_list[1];
        char *password = large_token_buffer.command_list[2];
        double amount = atof(large_token_buffer.command_list[3]);

        // // Print withdrawal details
        // printf("Withdrawal Transaction:\n");
        // printf("  Account: %s\n", account_num);
        // printf("  Amount: %.2f\n", amount);

        // Find the account
        account *acc = NULL;
        for (int i = 0; i < account_nums; i++) {
            if (strcmp(accounts[i].account_number, account_num) == 0) {
                acc = &accounts[i];
                break;
            }
        }

        // Verify password and perform withdrawal
        if (acc && strcmp(acc->password, password) == 0) {
            pthread_mutex_lock(&acc->ac_lock);
            acc->balance -= amount;
            acc->transaction_tracter += amount;
            pthread_mutex_unlock(&acc->ac_lock);
        } else {
            printf("Withdrawal failed: invalid account or password\n");
        }
        
    }
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
	account *accounts = NULL;
	int account_nums = 0;

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
    // print_accounts(accounts, account_nums);

	command_line large_token_buffer;

	int line_num = 0;

	//loop until the file is over
	while (getline (&line_buf, &len, inFPtr) != -1){
		//tokenize line buffer, large token is seperated by " "
		large_token_buffer = str_filler (line_buf, " ");

		// Ensure not empty line
		if (large_token_buffer.command_list[0] == NULL) {
	        // If the line is empty, skip it
	        free_command_line(&large_token_buffer);
	        continue;
	    }

	    // Call process_transaction with the tokenized line and account information
    	process_transaction(large_token_buffer, accounts, account_nums);

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
    }else{
          fprintf(stderr, "Usage: %s filename\n", argv[0]);
          return EXIT_FAILURE;
    }

	return EXIT_FAILURE;
}