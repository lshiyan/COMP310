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

struct memory_struct shellmemory[VAR_STORE_SIZE];

struct script_memory_struct{
    int used; //Denotes if this index is in use.
    char line[LINE_SIZE];
};

struct loaded_script {
    int used;
    char name[SCRIPT_NAME_SIZE];
    int num_lines;
    int num_pages;
    int page_table[MAX_PAGES];
    int ref_count;
};

struct script_memory_struct scriptshellmemory[FRAME_STORE_SIZE];
static struct loaded_script loaded_scripts[MAX_LOADED_SCRIPTS];

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
    for (i = 0; i < VAR_STORE_SIZE; i++){
        shellmemory[i].var   = "none";
        shellmemory[i].value = "none";
    }
}

// Set key value pair
void mem_set_value(char *var_in, char *value_in) {
    int i;

    for (i = 0; i < VAR_STORE_SIZE; i++){
        if (strcmp(shellmemory[i].var, var_in) == 0){
            shellmemory[i].value = strdup(value_in);
            return;
        } 
    }

    //Value does not exist, need to find a free spot.
    for (i = 0; i < VAR_STORE_SIZE; i++){
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

    for (i = 0; i < VAR_STORE_SIZE; i++){
        if (strcmp(shellmemory[i].var, var_in) == 0){
            return strdup(shellmemory[i].value);
        } 
    }
    return "Variable does not exist";
}

static int alloc_frame(void){
    for (int frame = 0; frame < MAX_PAGES; frame++){
        int frame_start = frame * FRAME_SIZE;
        int free_frame = 1;

        for (int offset = 0; offset < FRAME_SIZE; offset++){
            if (scriptshellmemory[frame_start + offset].used){
                free_frame = 0;
                break;
            }
        }

        if (free_frame){
            return frame;
        }
    }

    return -1;
}

static struct loaded_script *find_loaded_script_by_name(const char *script_name){
    for (int i = 0; i < MAX_LOADED_SCRIPTS; i++){
        if (loaded_scripts[i].used && strcmp(loaded_scripts[i].name, script_name) == 0){
            return &loaded_scripts[i];
        }
    }

    return NULL;
}

static struct loaded_script *alloc_loaded_script_slot(void){
    for (int i = 0; i < MAX_LOADED_SCRIPTS; i++){
        if (!loaded_scripts[i].used){
            return &loaded_scripts[i];
        }
    }

    return NULL;
}

static void clear_frame(int frame_number){
    if (frame_number < 0 || frame_number >= MAX_PAGES){
        return;
    }

    int frame_start = frame_number * FRAME_SIZE;
    for (int offset = 0; offset < FRAME_SIZE; offset++){
        scriptshellmemory[frame_start + offset].used = 0;
        scriptshellmemory[frame_start + offset].line[0] = '\0';
    }
}

//Frees num_lines of memory starting from index start from shell memory.
void free_script_memory(const char *script_name){
    struct loaded_script *script = find_loaded_script_by_name(script_name);
    if (script == NULL){
        return;
    }

    script->ref_count--;
    if (script->ref_count > 0){
        return;
    }

    for (int page = 0; page < script->num_pages; page++){
        clear_frame(script->page_table[page]);
        script->page_table[page] = -1;
    }

    script->used = 0;
    script->name[0] = '\0';
    script->num_lines = 0;
    script->num_pages = 0;
    script->ref_count = 0;
}


//Adds a process to the script memory, returns the memory location of the start if successful. Depending on policy, will also add by line length.
int add_process_to_memory(const char *script_name, char **line_list, int num_lines, int page_table[MAX_PAGES], int *num_pages){
    struct loaded_script *existing_script = find_loaded_script_by_name(script_name);
    if (existing_script != NULL){
        existing_script->ref_count++;
        memcpy(page_table, existing_script->page_table, sizeof(int) * MAX_PAGES);
        *num_pages = existing_script->num_pages;
        return 0;
    }

    int required_pages = (num_lines + FRAME_SIZE - 1) / FRAME_SIZE;
    if (required_pages > MAX_PAGES){
        return -1;
    }

    for (int i = 0; i < MAX_PAGES; i++){
        page_table[i] = -1;
    }

    for (int page = 0; page < required_pages; page++){
        int frame_number = alloc_frame();
        if (frame_number == -1){
            for (int prev_page = 0; prev_page < page; prev_page++){
                clear_frame(page_table[prev_page]);
            }
            return -1;
        }

        page_table[page] = frame_number;
        int frame_start = frame_number * FRAME_SIZE;
        for (int offset = 0; offset < FRAME_SIZE; offset++){
            int line_idx = page * FRAME_SIZE + offset;
            scriptshellmemory[frame_start + offset].used = 1;
            if (line_idx < num_lines){
                strcpy(scriptshellmemory[frame_start + offset].line, line_list[line_idx]);
            } else {
                scriptshellmemory[frame_start + offset].line[0] = '\0';
            }
        }
    }

    struct loaded_script *new_script = alloc_loaded_script_slot();
    if (new_script == NULL){
        for (int page = 0; page < required_pages; page++){
            clear_frame(page_table[page]);
            page_table[page] = -1;
        }
        return -1;
    }

    new_script->used = 1;
    strncpy(new_script->name, script_name, SCRIPT_NAME_SIZE - 1);
    new_script->name[SCRIPT_NAME_SIZE - 1] = '\0';
    new_script->num_lines = num_lines;
    new_script->num_pages = required_pages;
    memcpy(new_script->page_table, page_table, sizeof(int) * MAX_PAGES);
    new_script->ref_count = 1;
    *num_pages = required_pages;

    return 0;
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
