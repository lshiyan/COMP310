#include <stdlib.h>

#include "shellmemory.h"

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) < (b)) ? (b) : (a))

struct script_pcb{
    int pid;
    char script_name[SCRIPT_NAME_SIZE];
    size_t size;
    int cur_instruct;
    int job_length;
    int quantum;
    int num_pages;
    int page_table[MAX_PAGES];

    struct script_pcb *next_pcb;
    struct loaded_script *script;
};

struct execution_block{ //Consists of a block of pcbs with a singular policy to execute.
    struct script_pcb *head_ptr;

    int num_processes;
    char* block_policy;
};

void add_process_to_block(struct execution_block *block, char *policy, const char *script_name, int num_lines, int num_pages, int page_table[MAX_PAGES], int* pid_counter);
int exec_block(struct execution_block *head_ptr);
