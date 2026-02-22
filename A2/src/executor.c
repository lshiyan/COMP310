#include "executor.h"
#include "shell.h"
#include "shellmemory.h"
#include <string.h>
#include <stdio.h>

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) < (b)) ? (b) : (a))

int exec_fcfs_block(struct execution_block *block_ptr);
int exec_rr_block(struct execution_block *block_ptr);
int exec_aging_block(struct execution_block *block_ptr);
void free_block(struct execution_block *block_ptr);

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
        if (strcmp(policy, "FCFS") == 0 || strcmp(policy, "RR") == 0 || strcmp(policy, "AGING") == 0){ // Keep original arrival order for these policies.
            struct script_pcb *cur_head = block->head_ptr;

            while (cur_head->next_pcb != NULL){
                cur_head = cur_head->next_pcb;
            }

            cur_head->next_pcb = new_pcb;
        }
        else{ //Need to sort for these policies.
            struct script_pcb *cur_head = block->head_ptr;

            if (cur_head->job_length > new_pcb->job_length){ //New process is shortest, add to head.
                new_pcb->next_pcb = cur_head;
                block->head_ptr = new_pcb;
            }
            else{
                while(cur_head->next_pcb != NULL && (cur_head->next_pcb)->job_length <= new_pcb->job_length){ // Stop when next pcb is NULL or next pcb has a greater job length.
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

    else if (strcmp(policy, "RR") == 0 || strcmp(policy, "RR30") == 0){
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

//Executes a block with the FCFS / SJF policy.
int exec_fcfs_block(struct execution_block *block_ptr){
    struct script_pcb *ptr = block_ptr->head_ptr;
    int errCode = 0;
    while (ptr != NULL){
        int start = ptr->start;
        int end = start + ptr->size;

        for (int mem_idx = start; mem_idx < end; mem_idx++){
            char* cur_instruct = get_instruction(mem_idx);
            int err = parseInput(cur_instruct); 
            if (err != 0){
                errCode = 1;
            }
        }

        ptr = ptr->next_pcb;
    }

    free_block(block_ptr);
    return errCode;
}

//Executes a block with the RR policy.
int exec_rr_block(struct execution_block *block_ptr){
    struct script_pcb *ptr = block_ptr->head_ptr;
    int errCode = 0;
    int processes_completed = 0;
    int RR_TIME = strcmp(block_ptr->block_policy, "RR") == 0 ?  2 : 30;

    while (processes_completed != block_ptr->num_processes){
        int start = ptr->cur_instruct;
        int end = min(ptr->cur_instruct + RR_TIME, ptr->start + ptr->size);

        for (int mem_idx = start; mem_idx < end; mem_idx++){
            char* cur_instruct = get_instruction(mem_idx);
            int err = parseInput(cur_instruct); 
            if (err != 0){
                errCode = 1;
            }
            (ptr->cur_instruct)++;
            if (ptr->cur_instruct == ptr->start + ptr->size){
                processes_completed++;
            }
        }
        
        if (ptr->next_pcb == NULL){
            ptr = block_ptr->head_ptr;
        }
        else{
            ptr = ptr->next_pcb;
        }
    }

    free_block(block_ptr);
    return errCode;
}

//Executes a block with the aging policy.
int exec_aging_block(struct execution_block *block_ptr){
    struct script_pcb *ptr = block_ptr->head_ptr;
    int errCode = 0;

    struct script_pcb *pcb_list[block_ptr->num_processes];
    int finished[block_ptr->num_processes];
    memset(finished, 0, sizeof(finished));
    int idx = 0;

    while (ptr != NULL){
        pcb_list[idx] = ptr;
        ptr = ptr->next_pcb;
        idx++;
    }

    int completed_processes = 0;
    int last_idx = 0;

    while(completed_processes < block_ptr->num_processes){

        int min_job = 10000;
        int min_idx = 0;


        //First find the minimum job length process.
        for (int i = 0; i < block_ptr->num_processes; i++){
            if (finished[i]) continue;

            int l = pcb_list[i]->job_length;

            if (l < min_job) {
                min_job = l;
                min_idx = i;
            } else if (l == min_job) { //Tiebreaks in favor of last_executed job.
                if (i == last_idx) {
                    min_idx = i;
                }
            }
        }
        last_idx = min_idx;

        //Execute one instruction for the lowest length job.            
        ptr = pcb_list[min_idx];

        char *next_instruct = get_instruction(ptr->cur_instruct);

        int err = parseInput(next_instruct);

        if (err != 0){
            errCode = 1;
        }

        (ptr->cur_instruct)++;

        //Age all the jobs that are not the current job.
        for (int i = 0; i < block_ptr->num_processes; i++){
            if (i != min_idx && !finished[i]){
                pcb_list[i]->job_length = max(0, pcb_list[i]->job_length - 1);
            }
        }

        //If this job just finished, set the finished value to 1.
        if (ptr->cur_instruct == ptr->start + ptr->size){
            finished[min_idx] = 1;
            completed_processes++;
        }
    }   

    free_block(block_ptr);
    return errCode;    
}

//Frees memory from all elements in block, including script memory.
void free_block(struct execution_block *block_ptr){

    struct script_pcb *pcb = block_ptr->head_ptr;

    while (pcb != NULL)
    {
        struct script_pcb *next_pcb = pcb->next_pcb;
        free_script_memory(pcb->start, pcb->size);
        free(pcb);
        pcb = next_pcb;
    }

    free(block_ptr->block_policy);

    free(block_ptr);
}
