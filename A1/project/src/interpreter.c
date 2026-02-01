#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/wait.h>
#include "shellmemory.h"
#include "shell.h"
#include <sys/stat.h>
#include <ctype.h>
#include <unistd.h>

int MAX_ARGS_SIZE = 3;
int MAX_DIR_SIZE = 256;
int MAX_FILENAME_SIZE = 128;
int badcommand() {
    printf("Unknown Command\n");
    return 1;
}

// For source command only
int badcommandFileDoesNotExist() {
    printf("Bad command: File not found\n");
    return 3;
}

int help();
int quit();
int set(char *var, char *value);
int print(char *var);
int source(char *script);
int echo(char * string);
int my_ls();
int my_mkdir(char *dirname);
int my_touch(char *filename);
int my_cd(char *dirname);
int run(char *command_args[], int args_size);
int badcommandFileDoesNotExist();

// Interpret commands and their arguments
int interpreter(char *command_args[], int args_size) {
    int i;

    if (args_size < 1 || args_size > MAX_ARGS_SIZE) {
        return badcommand();
    }

    for (i = 0; i < args_size; i++) {   // terminate args at newlines
        command_args[i][strcspn(command_args[i], "\r\n")] = 0;
    }

    if (strcmp(command_args[0], "help") == 0) {
        //help
        if (args_size != 1)
            return badcommand();
        return help();

    } else if (strcmp(command_args[0], "quit") == 0) {
        //quit
        if (args_size != 1)
            return badcommand();
        return quit();

    } else if (strcmp(command_args[0], "set") == 0) {
        //set
        if (args_size != 3)
            return badcommand();
        return set(command_args[1], command_args[2]);

    } else if (strcmp(command_args[0], "print") == 0) {
        //print
        if (args_size != 2)
            return badcommand();
        return print(command_args[1]);

    } else if (strcmp(command_args[0], "source") == 0) {
        //source
        if (args_size != 2)
            return badcommand();
        return source(command_args[1]);

    } else if (strcmp(command_args[0], "echo") == 0) {
        //echo
        if (args_size != 2)
            return badcommand();
        return echo(command_args[1]);
    } else if (strcmp(command_args[0], "my_ls") == 0) {
        //ls
        if (args_size != 1)
            return badcommand();
        return my_ls();
    } else if (strcmp(command_args[0], "my_mkdir") == 0) {
        //mkdir
        if (args_size != 2)
            return badcommand();
        return my_mkdir(command_args[1]);
    } else if (strcmp(command_args[0], "my_touch") == 0) {
        //touch
        if (args_size != 2)
            return badcommand();
        return my_touch(command_args[1]);
    } else if (strcmp(command_args[0], "my_cd") == 0) {
        //cd
        if (args_size != 2)
            return badcommand();
        return my_cd(command_args[1]);
    } else if (strcmp(command_args[0], "run") == 0) {
        //run
        if (args_size < 2)
            return badcommand();
        return run(command_args, args_size);
    }   
    else {return badcommand();}
}

int help() {

    // note the literal tab characters here for alignment
    char help_string[] = "COMMAND			DESCRIPTION\n \
help			Displays all the commands\n \
quit			Exits / terminates the shell with “Bye!”\n \
set VAR STRING		Assigns a value to shell memory\n \
print VAR		Displays the STRING assigned to VAR\n \
source SCRIPT.TXT	Executes the file SCRIPT.TXT\n ";
    printf("%s\n", help_string);
    return 0;
}

int quit() {
    printf("Bye!\n");
    exit(0);
}

int set(char *var, char *value) {
    // Challenge: allow setting VAR to the rest of the input line,
    // possibly including spaces.

    // Hint: Since "value" might contain multiple tokens, you'll need to loop
    // through them, concatenate each token to the buffer, and handle spacing
    // appropriately. Investigate how `strcat` works and how you can use it
    // effectively here.

    mem_set_value(var, value);
    return 0;
}


int print(char *var) {
    printf("%s\n", mem_get_value(var));
    return 0;
}

int source(char *script) {
    int errCode = 0;
    char line[MAX_USER_INPUT];
    FILE *p = fopen(script, "rt");      // the program is in a file

    if (p == NULL) {
        return badcommandFileDoesNotExist();
    }

    fgets(line, MAX_USER_INPUT - 1, p);
    while (1) {
        errCode = parseInput(line);     // which calls interpreter()
        memset(line, 0, sizeof(line));

        if (feof(p)) {
            break;
        }
        fgets(line, MAX_USER_INPUT - 1, p);
    }

    fclose(p);

    return errCode;
}

int echo(char *string){
    if (string[0] == '$') {
        char tmp[100] = "";

        for (int i = 1; i < strlen(string); i++){
            tmp[i-1] = string[i];
        }
        char* INVALID_STRING = "Variable does not exist";
        char* mem_result = mem_get_value(tmp);

        if (strcmp(mem_result, INVALID_STRING) == 0){
            mem_result = "";
        }
        printf("%s", mem_result);
        printf("\n");
    }
    else{
        printf("%s", string);
        printf("\n");
    }
    return 0;
}

//Helper function for string comparisons.
int compare(const void *a, const void *b) {
    const char *sa = (const char *)a;
    const char *sb = (const char *)b;
    return strcmp(sa, sb);
}


int my_ls(){
    DIR *dirp;
    struct dirent *entry;

    char files[MAX_DIR_SIZE][MAX_FILENAME_SIZE];
    dirp = opendir(".");

    int i = 0;
    while(1){
        entry = readdir(dirp);

        if (entry != NULL){
            strcpy(files[i], entry->d_name);
            i += 1;
            continue;
        }

        break;
    }

    qsort(files, i, sizeof(files[0]), compare);
    for (int j = 0; j < i; j++){
        printf("%s\n", files[j]);
    }

    closedir(dirp);
    return 0;
}

// helper: returns 1 if s is a non-empty string of only letter/number, else 0
int alphanumeric(const char *s){
    if (s == NULL || s[0] == '\0') return 0;

    for (int i = 0; s[i] != '\0'; i++) {
        if (!isalnum(s[i])) {
            return 0;
        }
    }
    return 1;
}

int my_mkdir(char *dirname){
    const char *final_dir = NULL;

    // Case 1: normal directory name
    if (dirname[0] != '$') {
        if (!alphanumeric(dirname)) {
            printf("Bad command: my_mkdir\n");
            return 0;
        }
        final_dir = dirname;
    }
    // Case 2: dirname starts with '$'
    else {
        const char *var = dirname + 1;   // skip '$'
        // variable name after '$' must be alphanumeric
        if (!alphanumeric(var)) {
            printf("Bad command: my_mkdir\n");
            return 0;
        }
        char* INVALID_STRING = "Variable does not exist";
        char *val = mem_get_value((char*)var);
        if (strcmp(val, INVALID_STRING) == 0){
            printf("Bad command: my_mkdir\n");
            return 0;
        }
        // value must be a single alphanumeric token too
        if (!alphanumeric(val)) {
            printf("Bad command: my_mkdir\n");
            return 0;
        }
        final_dir = val;
    }
    // Try to create the directory
    if (mkdir(final_dir, 0777) != 0) {
        printf("Bad command: my_mkdir\n");
        return 0;
    }
    return 0;
}

int my_touch(char *filename){
    if (alphanumeric(filename)){
        FILE *f = fopen(filename, "a");
        fclose(f);
    }
    return 0;
}

int my_cd(char *dirname){
    if (!alphanumeric(dirname)){
        printf("Bad command: my_cd\n");
        return 0;
    }
    // chdir(dirname) tries to move into dirname inside current directory
    if (chdir(dirname) != 0) {
        // directory doesn't exist or not accessible
        printf("Bad command: my_cd\n");
        return 0;
    }
    // success
    return 0;
}


int run(char *command_args[], int args_size){
    //Allocate space for command and command args.
    const char* command = command_args[1];
    char *args[args_size];

    //Create full command filename
    char full_command[100] = "/bin/";
    strcat(full_command, command);
    int child_status;

    //Create execv args.
    for (int i = 1; i < args_size; i++){
        args[i - 1] = command_args[i];
    }
    args[args_size - 1] = NULL;

    //Fork exec wait
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("Fork failed\n");
        return 1;
    }
    if (pid == 0){
        execv(full_command, args);
        perror("execv");
        _exit(1);
    }

    int status;
    //Check error for wait.
    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        return 1;
    }

    // Child process succeeded
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        return 0;
    }

    return 1;
}