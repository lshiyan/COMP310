#include "executor.h"
#include "shell.h"
#include "shellmemory.h"
#include <string.h>
#include <stdio.h>

int exec_fcfs_block(struct execution_block *block_ptr);
int exec_rr_block(struct execution_block *block_ptr);
int exec_aging_block(struct execution_block *block_ptr);

//Adds a given process to an execution block. Creates a new PCB for the process then appends it to the tail of the block.
void add_process_to_block(char **line_list, struct execution_block *block, char *policy, int mem_start, int num_lines, int* pid_counter){

    struct script_pcb *new_pcb = malloc(sizeof *new_pcb); //NOTE: Must free this.
    *new_pcb = (struct script_pcb){0};

    new_pcb->pid = *pid_counter;
    (*pid_counter)++;

    new_pcb->start = mem_start;
    new_pcb->size = (size_t) num_lines;
    new_pcb->cur_instruct = mem_start;
    new_pcb->job_length = num_lines;

    if (block->head_ptr == NULL){
        block->head_ptr = new_pcb;
    }
    else{
        if (strcmp(policy, "FCFS") == 0 || strcmp(policy, "RR") == 0){ //Do not need to sort for these policies
            struct script_pcb *cur_head = block->head_ptr;

            while (cur_head->next_pcb != NULL){
                cur_head = cur_head->next_pcb;
            }

            cur_head->next_pcb = new_pcb;
        }
        else{ //Need to sort for these policies.
            struct script_pcb *cur_head = block->head_ptr;

            if (cur_head->job_length >= new_pcb->job_length){ //New process is shortest, add to head.
                new_pcb->next_pcb = cur_head;
                block->head_ptr = new_pcb;
            }
            else{
                while(cur_head->next_pcb != NULL && (cur_head->next_pcb)->job_length < new_pcb->job_length){ // Stop when next pcb is NULL or next pcb has a greater job length.
                    cur_head = cur_head->next_pcb;
                }

                struct script_pcb *tmp = cur_head->next_pcb;

                cur_head->next_pcb = new_pcb;
                new_pcb->next_pcb = tmp;
            }
        }
    }
}

//Executes all processes in a given block according to the policy.
int exec_block(struct execution_block *block_ptr){
    char *policy = block_ptr->block_policy;
    int errCode = 0;

    if (strcmp(policy, "FCFS") == 0){
        errCode = exec_fcfs_block(block_ptr);
    }

    else if (strcmp(policy, "RR") == 0){
        errCode = exec_rr_block(block_ptr);
    }

    else if (strcmp(policy, "SJF") == 0){ //Since jobs are already sorted by job length, we can just execute them in order like fcfs.
        errCode = exec_fcfs_block(block_ptr);
    }

    else if (strcmp(policy, "AGING") == 0){
       errCode =  exec_aging_block(block_ptr);
    }

    return errCode;
}

int exec_fcfs_block(struct execution_block *block_ptr){
    struct script_pcb *head_pcb = block_ptr->head_ptr;
    int errCode = 0;
    while (1){
        int start = head_pcb->start;
        int end = start + head_pcb->size;

        for (int mem_idx = start; mem_idx < end; mem_idx++){
            char* cur_instruct = get_instruction(mem_idx);
            printf("%s\n", cur_instruct);
            int err = parseInput(cur_instruct); 
            if (err != 0){
                errCode = 1;
            }
        }

        return 0;
    }
}

int exec_rr_block(struct execution_block *block_ptr){
    //TODO
    return 0;
}

int exec_aging_block(struct execution_block *block_ptr){
    //TODO
    return 0;
}