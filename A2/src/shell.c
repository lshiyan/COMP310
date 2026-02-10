#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <unistd.h>
#include <pthread.h>
#include "shell.h"
#include "interpreter.h"
#include "shellmemory.h"

int parseInput(char ui[]);

//Global queue for all processes.
struct ready_queue main_queue = {0};

//Thread that executes processes in the ready queue depending on scheduling policy.
void* start_script_execution_thread(void* args){
    while (1){
        if (main_queue.head_ptr != NULL){
            struct script_pcb *head_pcb = main_queue.head_ptr;
            int start_idx = head_pcb->start;
            int end_idx = start_idx + head_pcb->size;

            for (int idx = start_idx; idx < end_idx; idx++){
                char* cur_instruct = get_instruction(idx);
                int errCode = parseInput(cur_instruct);

                if (errCode != 0){
                    printf("Error with executing instruction: %s\n", cur_instruct);
                }

                head_pcb->cur_instruct++;
            }
            free_mem(head_pcb);
            main_queue.head_ptr = (main_queue.head_ptr)->next_pcb;
        }
        sleep(5);
    }
    
}

// Start of everything
int main(int argc, char *argv[]) {
    printf("Shell version 1.5 created Dec 2025\n");

    char prompt = '$';  				// Shell prompt
    char userInput[MAX_USER_INPUT];		// user's input stored here
    int errorCode = 0;					// zero means no error, default

    //init user input
    for (int i = 0; i < MAX_USER_INPUT; i++) {
        userInput[i] = '\0';
    }
    
    pthread_t script_execution_thread;

    pthread_create(&script_execution_thread, NULL, start_script_execution_thread, NULL);

    //init shell memory
    mem_init();
    while(1) {				
        int is_terminal = isatty(0); //Detects whether stdin is a terminal or not, 0 for batch mode execution.
        if (is_terminal){
            printf("%c ", prompt);
        }
        // here you should check the unistd library 
        // so that you can find a way to not display $ in the batch mode
        fgets(userInput, MAX_USER_INPUT-1, stdin);
        if (userInput[0] == '\n' || userInput[0] == '\0'){
            break;
        }
        errorCode = parseInput(userInput);
        if (errorCode == -1) exit(99);	// ignore all other errors
        memset(userInput, 0, sizeof(userInput));
    }

    pthread_join(script_execution_thread, NULL);

    return 0;
}

int wordEnding(char c) {
    // You may want to add ';' to this at some point,
    // or you may want to find a different way to implement chains.
    return c == '\0' || c == '\n' || c == ' ';
}

int parseInput(char inp[]) {
    // One-liner support
    if (strchr(inp, ';') != NULL) {
        char *copy = strdup(inp);      // make modifiable copy
        char *command = strtok(copy, ";"); // get first command
        int errCode = 0;

        while (command != NULL) {
            // skip leading spaces in each chunk
            while (*command == ' ') command++;
            errCode = parseInput(command);

            command = strtok(NULL, ";");
        }

        free(copy);
        return errCode;
    }

    char tmp[200], *words[100];                            
    int ix = 0, w = 0;
    int wordlen;
    int errorCode;
    for (ix = 0; inp[ix] == ' ' && ix < 1000; ix++); // skip white spaces
    while (inp[ix] != '\n' && inp[ix] != '\0' && ix < 1000) {
        // extract a word
        for (wordlen = 0; !wordEnding(inp[ix]) && ix < 1000; ix++, wordlen++) {
            tmp[wordlen] = inp[ix];                        
        }
        tmp[wordlen] = '\0';
        words[w] = strdup(tmp);
        w++;
        if (inp[ix] == '\0') break;
        ix++; 
    }
    errorCode = interpreter(words, w);
    return errorCode;
}
