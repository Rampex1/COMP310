#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "shellmemory.h"

struct memory_struct {
    char *var;
    char *value;
};

struct memory_struct shellmemory[MEM_SIZE];

char *program_memory[MAX_PROGRAM_LINES];
int program_used[MAX_PROGRAM_LINES];

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

    for (int i = 0; i < MAX_PROGRAM_LINES; i++) {
        program_memory[i] = NULL;
        program_used[i] = 0;
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

int program_load(FILE *fp) {
    char line[MAX_LINE_LENGTH];
    int start = -1;
    int count = 0;

    for (int i = 0; i < MAX_PROGRAM_LINES; i++) {
        if (!program_used[i]) {
            start = i;
            break;
        }
    }

    if (start == -1) return -1;

    int idx = start;

    while (fgets(line, MAX_LINE_LENGTH, fp)) {
        if (idx >= MAX_PROGRAM_LINES) return -1;

        program_memory[idx] = strdup(line);
        program_used[idx] = 1;

        idx++;
        count++;
    }

    return start; // return starting index
}


void program_free(int start, int length) {
    for (int i = start; i < start + length; i++) {
        if (program_used[i]) {
            free(program_memory[i]);
            program_memory[i] = NULL;
            program_used[i] = 0;
        }
    }
}
