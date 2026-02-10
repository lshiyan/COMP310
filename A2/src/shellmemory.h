#define MEM_SIZE 1000
#define LINE_SIZE 100

struct script_pcb{
    int pid;
    int start;
    size_t size;
    int cur_instruct;

    struct script_pcb *next_pcb;
};

struct ready_queue{
    struct script_pcb *head_ptr;
};

extern struct ready_queue main_queue; //Global queue for all processes.

void mem_init();
char *mem_get_value(char *var);
void mem_set_value(char *var, char *value);
int add_process(char **line_list, int num_lines);
char* get_instruction(int mem_idx);
void free_mem(struct script_pcb *pcb_ptr);