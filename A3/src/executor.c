#include "executor.h"
#include "shell.h"
#include "shellmemory.h"
#include <string.h>
#include <stdio.h>

int exec_fcfs_block(struct execution_block *block_ptr);
int exec_rr_block(struct execution_block *block_ptr);
int exec_aging_block(struct execution_block *block_ptr);
void free_block(struct execution_block *block_ptr);
static int get_physical_index(struct script_pcb *pcb, int logical_idx);
void handle_page_fault(struct execution_block *block_ptr, struct script_pcb *pcb_ptr);
void move_pcb_to_back(struct execution_block *block_ptr, struct script_pcb *pcb);

//Adds a given process to an execution block. Creates a new PCB for the process then appends it to the tail of the block.
void add_process_to_block(struct execution_block *block, char *policy, const char *script_name, int num_lines, int num_pages, int page_table[MAX_PAGES], int* pid_counter){

    struct script_pcb *new_pcb = malloc(sizeof *new_pcb); //NOTE: Must free this.
    *new_pcb = (struct script_pcb){0};

    new_pcb->pid = *pid_counter;
    (*pid_counter)++;

    strncpy(new_pcb->script_name, script_name, SCRIPT_NAME_SIZE - 1);
    new_pcb->script_name[SCRIPT_NAME_SIZE - 1] = '\0';
    new_pcb->size = (size_t) num_lines;
    new_pcb->cur_instruct = 0;
    new_pcb->job_length = num_lines;
    new_pcb->quantum = strcmp(policy, "RR30") == 0 ? 30 : 2;
    new_pcb->num_pages = num_pages;
    memcpy(new_pcb->page_table, page_table, sizeof(int) * MAX_PAGES);
    new_pcb->script = get_script_by_name(script_name);

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

//Returns index of physical memory location.
static int get_physical_index(struct script_pcb *pcb, int logical_idx){
    int page_number = logical_idx / FRAME_SIZE;

    if (page_number >= MAX_PAGES){
        return -1;
    }

    int offset = logical_idx % FRAME_SIZE;
    int frame_number = pcb->script->page_table[page_number];

    if (frame_number == -1){
        return -1;
    }

    pcb->page_table[page_number] = frame_number;
    touch_frame(frame_number);

    return frame_number * FRAME_SIZE + offset;
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
        int end = (int)ptr->size;

        for (int logical_idx = 0; logical_idx < end; logical_idx++){
            char* cur_instruct = get_instruction(get_physical_index(ptr, logical_idx));
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
int exec_rr_block(struct execution_block *block_ptr) {
    struct script_pcb *ptr = block_ptr->head_ptr;

    char** line_list = (ptr->script)->line_list;

    int errCode = 0;
    int processes_completed = 0;
    int RR_TIME = strcmp(block_ptr->block_policy, "RR") == 0 ? 2 : 30;

    while (processes_completed != block_ptr->num_processes) {
        int start = ptr->cur_instruct;
        int end = min(ptr->cur_instruct + RR_TIME, (int)ptr->size);

        for (int logical_idx = start; logical_idx < end; logical_idx++) {
            int physical_index = get_physical_index(ptr, logical_idx);
            char *cur_instruct = get_instruction(physical_index);
            
            if (cur_instruct == NULL) {
                handle_page_fault(block_ptr, ptr);
                move_pcb_to_back(block_ptr, ptr);
                break;
            }

            int err = parseInput(cur_instruct);
            if (err != 0) {
                errCode = 1;
            }

            ptr->cur_instruct++;

            if (ptr->cur_instruct == (int)ptr->size) {
                processes_completed++;
                break;
            }
        }

        ptr = ptr->next_pcb;

        if (ptr == NULL) {
            ptr = block_ptr->head_ptr;
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

        char *next_instruct = get_instruction(get_physical_index(ptr, ptr->cur_instruct));

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
        if (ptr->cur_instruct == (int)ptr->size){
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
        free_script_memory(pcb->script_name);
        free(pcb);
        pcb = next_pcb;
    }

    free(block_ptr->block_policy);

    free(block_ptr);
}

//Handles page faults, modifies page tables and frees up memory if necessary.
void handle_page_fault(struct execution_block *block_ptr, struct script_pcb *pcb_ptr){
    (void)block_ptr;
    int free_frame = alloc_frame();

    if (free_frame == -1){
        printf("Page fault! ");
        free_frame = find_lru_frame();
        struct loaded_script *evicted_page_script = get_script_by_frame(free_frame);

        if (evicted_page_script != NULL){
            for (int i = 0; i < MAX_PAGES; i++){
                if ((evicted_page_script->page_table)[i] == free_frame){
                    (evicted_page_script->page_table)[i] = -1;
                }
            }
        }

        char* evicted_lines[FRAME_SIZE] = {0};

        for (int offset = 0; offset < FRAME_SIZE; offset++) {
            char* instruction = get_instruction(free_frame*FRAME_SIZE + offset);

            if (instruction == NULL){
                continue;
            }

            evicted_lines[offset] = malloc(strlen(instruction) + 1);
            strcpy(evicted_lines[offset], instruction);
        }

        printf("Victim page contents:\n\n");
        for (int i = 0; i < FRAME_SIZE; i ++){
            if (evicted_lines[i] != NULL){
                printf("%s", evicted_lines[i]);
            }
        }
        printf("\nEnd of victim page contents.\n");

        clear_frame(free_frame);

        for (int i = 0; i < FRAME_SIZE; i++) {
            free(evicted_lines[i]);
        }
    } else {
        printf("Page fault!\n");
    }

    struct loaded_script *script = (pcb_ptr->script);

    int cur_page = pcb_ptr->cur_instruct / FRAME_SIZE;
    int start_line = cur_page * FRAME_SIZE;

    for (int offset = 0; offset < FRAME_SIZE; offset++){
        if(start_line + offset < pcb_ptr->size){
            load_line_into_memory((script->line_list)[start_line + offset], free_frame*FRAME_SIZE + offset);
        }
    }

    map_frame_to_script(free_frame, script);
    touch_frame(free_frame);
    (script->page_table)[cur_page] = free_frame;
    (pcb_ptr->page_table)[cur_page] = free_frame;
}

//Moves pcb to the back of the ready queue.
void move_pcb_to_back(struct execution_block *block_ptr, struct script_pcb *pcb) {
    if (block_ptr == NULL || pcb == NULL || block_ptr->head_ptr == NULL) {
        return;
    }

    if (pcb == block_ptr->head_ptr && pcb->next_pcb == NULL) {
        return;
    }

    struct script_pcb *prev = NULL;
    struct script_pcb *cur = block_ptr->head_ptr;

    while (cur != NULL && cur != pcb) {
        prev = cur;
        cur = cur->next_pcb;
    }

    if (cur == NULL) {
        return;
    }

    if (cur->next_pcb == NULL) {
        return;
    }

    if (prev == NULL) {
        block_ptr->head_ptr = cur->next_pcb;
    } else {
        prev->next_pcb = cur->next_pcb;
    }

    struct script_pcb *tail = block_ptr->head_ptr;
    while (tail->next_pcb != NULL) {
        tail = tail->next_pcb;
    }

    tail->next_pcb = cur;
    cur->next_pcb = NULL;
}
