#define SHELL_MEMORY_SIZE 1000
#define FRAME_SIZE 3
#define VAR_STORE_SIZE 10
#define FRAME_STORE_SIZE (SHELL_MEMORY_SIZE - VAR_STORE_SIZE)
#define MAX_LOADED_SCRIPTS (FRAME_STORE_SIZE / FRAME_SIZE)
#define MAX_PAGES (FRAME_STORE_SIZE / FRAME_SIZE)
#define MAX_SCRIPT_LINES FRAME_STORE_SIZE
#define LINE_SIZE 100
#define SCRIPT_NAME_SIZE 256

void mem_init();
char *mem_get_value(char *var);
void mem_set_value(char *var, char *value);
int add_process_to_memory(const char *script_name, char **line_list, int num_lines, int page_table[MAX_PAGES], int *num_pages);
char* get_instruction(int mem_idx);
void free_script_memory(const char *script_name);
