#define MEM_SIZE 1000
#define LINE_SIZE 100

void mem_init();
char *mem_get_value(char *var);
void mem_set_value(char *var, char *value);
int add_process_to_memory(char **line_list, int num_lines);
char* get_instruction(int mem_idx);
void free_script_memory(int start, int num_lines);