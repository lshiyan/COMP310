#include <stdlib.h>

struct script_pcb{
    int pid;
    int start;
    size_t size;
    int cur_instruct;
    int job_length;

    struct script_pcb *next_pcb;
};

struct execution_block{ //Consists of a block of pcbs with a singular policy to execute.
    struct script_pcb *head_ptr;
    struct execution_block *next_block;

    char* block_policy;
};

void add_process_to_block(char **line_list, struct execution_block *block, char *policy, int mem_start, int num_lines, int* pid_counter);
int exec_block(struct execution_block *head_ptr);