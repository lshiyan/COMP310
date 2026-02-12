#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "shellmemory.h"
#include "executor.h"


struct memory_struct {
    char *var;
    char *value;
};

struct memory_struct shellmemory[MEM_SIZE];

struct script_memory_struct{
    int used; //Denotes if this index is in use.
    char line[LINE_SIZE];
};

struct script_memory_struct scriptshellmemory[MEM_SIZE];

// Helper functions
int match(char *model, char *var) {
    int i, len = strlen(var), matchCount = 0;
    for (i = 0; i < len; i++) {
        if (model[i] == var[i]) matchCount++;
    }
    if (matchCount == len) {
        return 1;
    } else return 0;
}

// Shell memory functions

void mem_init(){
    int i;
    for (i = 0; i < MEM_SIZE; i++){		
        shellmemory[i].var   = "none";
        shellmemory[i].value = "none";
    }
}

// Set key value pair
void mem_set_value(char *var_in, char *value_in) {
    int i;

    for (i = 0; i < MEM_SIZE; i++){
        if (strcmp(shellmemory[i].var, var_in) == 0){
            shellmemory[i].value = strdup(value_in);
            return;
        } 
    }

    //Value does not exist, need to find a free spot.
    for (i = 0; i < MEM_SIZE; i++){
        if (strcmp(shellmemory[i].var, "none") == 0){
            shellmemory[i].var   = strdup(var_in);
            shellmemory[i].value = strdup(value_in);
            return;
        } 
    }

    return;
}

//get value based on input key
char *mem_get_value(char *var_in) {
    int i;

    for (i = 0; i < MEM_SIZE; i++){
        if (strcmp(shellmemory[i].var, var_in) == 0){
            return strdup(shellmemory[i].value);
        } 
    }
    return "Variable does not exist";
}

//Returns an integer corresponding to an allocated num_lines run of free memory. Returns -1 if no run found.
int alloc_lines(int num_lines){
    if (num_lines < 1 || num_lines > MEM_SIZE) return -1;

    int start = -1;
    int cur_length = 0;
    for(int i = 0; i < MEM_SIZE; i++){
        if (scriptshellmemory[i].used == 0){
            if (cur_length == 0){
                start = i;
            }
            cur_length++;

            if (cur_length == num_lines){
                return start;
            }
        }
        else{
            start = -1;
            cur_length = 0;
        }
    }

    return -1;

}

//Frees num_lines of memory starting from index start from shell memory.
void free_script_memory(int start, int num_lines){
    if (start < 0 || num_lines < 1 || start + num_lines > MEM_SIZE) return;

    for (int i = start; i < start + num_lines; i++){
        scriptshellmemory[i].used = 0; 
        scriptshellmemory[i].line[0] = '\0';
    }
}


//Adds a process to the script memory, returns the memory location of the start if successful. Depending on policy, will also add by line length.
int add_process_to_memory(char **line_list, int num_lines){
    int mem_start = alloc_lines(num_lines);

    if (mem_start == -1) return -1;

    //Denote all needed memory as used, copy script lines to memory.
    for (int i = mem_start; i < mem_start + num_lines; i++){
        scriptshellmemory[i].used = 1;
        strcpy(scriptshellmemory[i].line, line_list[i-mem_start]);
    }

    return mem_start;
}

//Returns a pointer to the instruction located at index mem_idx in scriptshellmemory.
char* get_instruction(int mem_idx){
    struct script_memory_struct *script_mem = &scriptshellmemory[mem_idx];

    if (script_mem->used == 0){
        printf("Error: script memory at location %d is unused.", mem_idx);
        return NULL;
    }

    return script_mem->line;
}

