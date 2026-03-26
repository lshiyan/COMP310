#pragma once

#define SHELL_MEMORY_SIZE 1000
#define FRAME_SIZE 3

#ifndef VAR_STORE_SIZE
#define VAR_STORE_SIZE 10
#endif

#ifndef FRAME_STORE_SIZE
#define FRAME_STORE_SIZE 300
#endif

#define MAX_LOADED_SCRIPTS 100
#define MAX_PAGES ((MAX_SCRIPT_LINES + FRAME_SIZE - 1) / FRAME_SIZE)
#define MAX_SCRIPT_LINES 100
#define LINE_SIZE 100
#define SCRIPT_NAME_SIZE 256


struct loaded_script {
    int used;
    char name[SCRIPT_NAME_SIZE];
    int num_lines;
    char* line_list[MAX_SCRIPT_LINES];
    int num_pages;
    int page_table[MAX_PAGES];
    int ref_count;
};

void mem_init();
char *mem_get_value(char *var);
void mem_set_value(char *var, char *value);
int add_process_to_memory(const char *script_name, char **line_list, int num_lines, int page_table[MAX_PAGES], int *num_pages);
char* get_instruction(int mem_idx);
void free_script_memory(const char *script_name);
int alloc_frame();
void clear_frame(int frame_number);
struct loaded_script* get_script_by_frame(int frame_number);
int load_line_into_memory(const char* line, int memory_idx);
struct loaded_script* get_script_by_name(const char* script_name);