 #include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include "account.h"
#include <sys/wait.h>
#include <sys/stat.h>
#include "string_parser.h"

#include <sys/mman.h> // For mmap, PROT_READ, PROT_WRITE, MAP_SHARED, MAP_ANONYMOUS
#include <fcntl.h>    // For file control options, like O_CREAT, O_RDWR
#include <unistd.h>   // For close, ftruncate
account *shared_accounts = NULL;
pid_t pid;

pthread_mutex_t process_transaction_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t update_counters = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int processed_transactions = 0; // Shared counter
int total_transactions = 0;

pthread_mutex_t thread_counters = PTHREAD_MUTEX_INITIALIZER;
int exited_threads = 0;

int update_ready = 0; // Flag to indicate when the bank thread can update balances

#define NUM_WORKERS 10
#define TOTAL_VALID_TRANSACTIONS 90000
pthread_t *thread_ids;
pthread_t bank_thread;

// Initialize the barrier
pthread_barrier_t barrier;

account *accounts = NULL;
int account_nums = 0;

int total_lines = 0;
int skipped_lines = 0;
int lines_per_threads = 0;

char *filename = "";

int numbers[NUM_WORKERS] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

int pipe_fd[2]; // Pipe for communication between Duck Bank and Auditor

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

void auditor_process() {
    close(pipe_fd[1]); // Close write end of the pipe in the Auditor process

    FILE *ledger = fopen("ledger.txt", "w");
    if (ledger == NULL) {
        perror("Error opening ledger file");
        exit(EXIT_FAILURE);
    }

    char buffer[256];
    ssize_t bytes_read;
    while ((bytes_read = read(pipe_fd[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0'; // Null-terminate the buffer
        fprintf(ledger, "%s", buffer);
        memset(buffer, 0, sizeof(buffer)); // Optional, but not strictly needed now
    }

    fclose(ledger);
    close(pipe_fd[0]); // Close read end of the pipe in the Auditor process
    exit(EXIT_SUCCESS);
}

void *update_balance(void* arg){
    pthread_barrier_wait(&barrier);
    //printf("BANK: Got past barrier\n");
    char log_entry[128];

    while(1){
        pthread_mutex_lock(&process_transaction_lock);
        //printf("BANK: Locked transaction\n");

        // Wait until signal to update
        while (!update_ready) {
            //printf("BANK: %d\n", total_transactions);
            //printf("BANK: waiting cond wait\n");
            pthread_cond_wait(&cond, &process_transaction_lock);
            //printf("BANK: after cond wait\n");
        }
        //printf("BANK: updating\n");

        // Update balances and reset counters
        update_ready = 0;
        processed_transactions = 0;

        for (int i = 0; i < account_nums; i++) {
            pthread_mutex_lock(&(accounts[i].ac_lock));

            double reward = accounts[i].transaction_tracter * accounts[i].reward_rate;
            accounts[i].balance += reward;
            accounts[i].transaction_tracter = 0.0;

            // Append to act_#.txt
            char filename[64];
            snprintf(filename, sizeof(filename), "act_%d.txt", i);
            FILE *outFPtr = fopen(filename, "a");
            if (outFPtr) {
                fprintf(outFPtr, "%.2f\n", accounts[i].balance);
                fclose(outFPtr);
            }

            pthread_mutex_unlock(&(accounts[i].ac_lock));

            kill(pid, SIGUSR1);

            // Log applied interest to the pipe
            time_t now = time(NULL);
            snprintf(log_entry, sizeof(log_entry), 
                "Applied Interest to account %s. New Balance: %.2f. Time of Update: %s", 
                accounts[i].account_number, accounts[i].balance, ctime(&now));
            write(pipe_fd[1], log_entry, strlen(log_entry));
        }

        // Signal all worker threads to resume
        pthread_cond_broadcast(&cond);

        pthread_mutex_unlock(&process_transaction_lock);

        // Check if last update
        if (total_transactions == TOTAL_VALID_TRANSACTIONS){
            //printf("BREAK\n");
            break;
        }

    }

    pthread_exit(NULL);
}


void *process_transaction(void* arg) {
    //printf("worker waiting at barrier\n");
    pthread_barrier_wait(&barrier);
    //printf("worker past barrier\n");

    int check_balance_count = 0; // Counter for "Check Balance" transactions
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
                pthread_mutex_lock(&update_counters);
                processed_transactions++;
                total_transactions++;
                pthread_mutex_unlock(&update_counters);
            }

            pthread_mutex_unlock(&(accounts[src_index].ac_lock));
            pthread_mutex_unlock(&(accounts[dst_index].ac_lock));

        // Check balance
        } else if (strcmp(transaction_type, "C") == 0) {
            check_balance_count++;
            if (check_balance_count % 500 == 0) {
                char *account_num = large_token_buffer.command_list[1];
                char *password = large_token_buffer.command_list[2];

                account *acc = NULL;
                for (int j = 0; j < account_nums; j++) {
                    if (strcmp(accounts[j].account_number, account_num) == 0) {
                        acc = &accounts[j];
                        break;
                    }
                }

                if (acc && strcmp(acc->password, password) == 0) {
                    time_t now = time(NULL);
                    char log_entry[128];
                    snprintf(log_entry, sizeof(log_entry), 
                        "Worker checked balance of Account %s. Balance: %.2f. Check occurred at %s", 
                        acc->account_number, acc->balance, ctime(&now));
                    write(pipe_fd[1], log_entry, strlen(log_entry));
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
                pthread_mutex_lock(&update_counters);
                processed_transactions++;
                total_transactions++;
                pthread_mutex_unlock(&update_counters);
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
                pthread_mutex_lock(&update_counters);
                processed_transactions++;
                total_transactions++;
                pthread_mutex_unlock(&update_counters);
            }
            pthread_mutex_unlock(&(accounts[acc_index].ac_lock));
        }
        free_command_line(&large_token_buffer);

        // Check if we need to pause processing
        pthread_mutex_lock(&process_transaction_lock);
        while (processed_transactions >= 5000) {
            update_ready = 1; // Notify the bank thread
            pthread_cond_signal(&cond);
            pthread_cond_wait(&cond, &process_transaction_lock); // Worker thread pauses
        }
        pthread_mutex_unlock(&process_transaction_lock);

    }

    // Check if last thread to exit
    pthread_mutex_lock(&process_transaction_lock);
    pthread_mutex_lock(&thread_counters);
    if (exited_threads == NUM_WORKERS - 1){
        update_ready = 1; // Notify the bank thread
        pthread_cond_signal(&cond);
        pthread_cond_wait(&cond, &process_transaction_lock); // Worker thread pauses
    }
    pthread_mutex_unlock(&thread_counters);
    pthread_mutex_unlock(&process_transaction_lock);

    //free_command_line(&large_token_buffer);
    free(line_buf);
    fclose(inFPtr);

    //Update exited threads
    pthread_mutex_lock(&thread_counters);
    exited_threads++;
    pthread_mutex_unlock(&thread_counters);

    pthread_exit(NULL);
}


void file_mode(){
    pthread_barrier_init(&barrier, NULL, NUM_WORKERS + 1);

    if (pipe(pipe_fd) == -1) {
        perror("Pipe creation failed");
        exit(EXIT_FAILURE);
    }

    close(pipe_fd[0]); // Close read end of the pipe in the Duck Bank process
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
    fclose(inFPtr);

    //opening file to read
    inFPtr = fopen (filename, "r");

    // Find the total lines in the file
    total_lines = count_total_lines(inFPtr);

    // Find number of skipped lines
    skipped_lines = (account_nums * 5) + 1;

    // Find number of transactions per thread
    lines_per_threads = (total_lines - skipped_lines) / NUM_WORKERS;

    int line_num = 0;

    // Shared memory mapping and copying
    // Map shared memory
    size_t shared_mem_size = sizeof(account) * account_nums;
    shared_accounts = (account *)mmap(NULL, shared_mem_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    memcpy(shared_accounts, accounts, shared_mem_size);

    // Set up initial balances
    for (int i = 0; i < account_nums; i++) {
        shared_accounts[i].puddles_balance = accounts[i].balance * 0.20;
    }

    // Fork
    pid = fork();
    if (pid == -1) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Sigwait
        sigset_t sigset;
        sigemptyset(&sigset);
        sigaddset(&sigset, SIGUSR1);
        sigprocmask(SIG_BLOCK, &sigset, NULL);
        int sig;

        mkdir("savings", 0777);

        while(1){
            // Wait for a signal from Duck Bank
            sigwait(&sigset, &sig);

            // Process each account in shared memory
            for (int i = 0; i < account_nums; i++) {
                double reward = shared_accounts[i].puddles_balance * shared_accounts[i].reward_rate;

                // Apply the reward to the Puddles Bank savings balance
                shared_accounts[i].puddles_balance += reward;

                // Create or update the savings file
                char savings_file[128];
                snprintf(savings_file, sizeof(savings_file), "savings/account_%s.txt", shared_accounts[i].account_number);
                FILE *savings_fp = fopen(savings_file, "w");
                if (savings_fp) {
                    fprintf(savings_fp, "Balance: %.2f\nReward Rate: %.2f\n",
                            shared_accounts[i].puddles_balance, shared_accounts[i].reward_rate);
                    fclose(savings_fp);
                }
            }

        }

        // Cleanup
        munmap(shared_accounts, shared_mem_size);

        exit(EXIT_SUCCESS);
    }

    // create thread, give job, and cast args to void pointer!
    thread_ids = malloc(NUM_WORKERS * sizeof(pthread_t));
    if (thread_ids == NULL) {
        perror("Failed to allocate memory for thread IDs");
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < NUM_WORKERS; i++){
        pthread_create(&thread_ids[i], NULL, process_transaction, (void*)&(numbers[i]));
    }

    // Call Update Balance
    pthread_create(&bank_thread, NULL, update_balance, NULL);

    // wait on our threads to rejoin main thread
    for (int j = 0; j < NUM_WORKERS; ++j){
        pthread_join(thread_ids[j], NULL);
    }

    // Wait for bank_thread
    pthread_join(bank_thread, NULL);

    // Print final balances to an output file
    print_final_balances("output.txt");

    // Close and free buffer and accounts
    fclose(inFPtr);
    free(accounts);
    free(shared_accounts);
    free (line_buf);
    free(thread_ids);
    close(pipe_fd[1]); // Close write end of the pipe in the Duck Bank process

    // Cleanup
    munmap(shared_accounts, shared_mem_size);
    
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