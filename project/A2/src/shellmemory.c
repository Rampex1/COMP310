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

// Helper functions ----------------------------------------------------
int match(char *model, char *var) {
    int i, len = strlen(var), matchCount = 0;
    for (i = 0; i < len; i++) {
        if (model[i] == var[i]) matchCount++;
    }
    if (matchCount == len) {
        return 1;
    } else return 0;
}

// Shell memory functions ----------------------------------------------------
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

// ----------- 1.2.1Code loading  ----------------
int program_load(FILE *fp) {
    char line[MAX_LINE_LENGTH];
    int start = -1;
    int count = 0;

    // find the first slot that is NOT USED (since 1 means USED, we look for 0 in this conditional)
    for (int i = 0; i < MAX_PROGRAM_LINES; i++) {
        if (!program_used[i]) {
            start = i;
            break;
        }
    }

    if (start == -1) return -1;

    int idx = start;
    // read FILE line by line
    while (fgets(line, MAX_LINE_LENGTH, fp)) {
        if (idx >= MAX_PROGRAM_LINES) return -1;

        // store a duplicate of the line -> program memory at line index
        program_memory[idx] = strdup(line);
        program_used[idx] = 1;

        idx++;
        count++;
    }

    return start; // return starting index
}


void program_free(int start, int length) {
    // from start, ensure that all slots (lengtht-away) from start, has program_used = 0, if 1: clean up also the memory
    // Indices:      0  1  2 [3] 4 5 6
    // used before:  1  1  1 [1] 0 0 0
    // mem before:   p0 p1 p2 [p3] . . .     (p3 is a strdup'd line pointer)
    //
    // After program_free(3, 1):
    // used after:   1  1  1 [0] 0 0 0
    // mem after:    p0 p1 p2 [NULL] . . .   (freed + pointer cleared)

    for (int i = start; i < start + length; i++) {
        if (program_used[i]) {
            free(program_memory[i]);
            program_memory[i] = NULL;
            program_used[i] = 0;
        }
    }
}
