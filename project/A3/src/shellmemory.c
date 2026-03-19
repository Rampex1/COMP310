#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "shellmemory.h"

// Variable store
struct memory_struct {
    char *var;
    char *value;
};
static struct memory_struct shellmemory[VAR_STORE_SIZE];

// Frame store
char *frame_store[FRAME_STORE_SIZE];
int frame_used[NUM_FRAMES];

// Helper
static int match(char *model, char *var) {
    int i, len = strlen(var), matchCount = 0;
    for (i = 0; i < len; i++) {
        if (model[i] == var[i]) matchCount++;
    }
    return matchCount == len;
}

void mem_init() {
    for (int i = 0; i < VAR_STORE_SIZE; i++) {
        shellmemory[i].var   = "none";
        shellmemory[i].value = "none";
    }
    for (int i = 0; i < FRAME_STORE_SIZE; i++) {
        frame_store[i] = NULL;
    }
    for (int i = 0; i < NUM_FRAMES; i++) {
        frame_used[i] = 0;
    }
}

void mem_set_value(char *var_in, char *value_in) {
    for (int i = 0; i < VAR_STORE_SIZE; i++) {
        if (strcmp(shellmemory[i].var, var_in) == 0) {
            shellmemory[i].value = strdup(value_in);
            return;
        }
    }
    for (int i = 0; i < VAR_STORE_SIZE; i++) {
        if (strcmp(shellmemory[i].var, "none") == 0) {
            shellmemory[i].var   = strdup(var_in);
            shellmemory[i].value = strdup(value_in);
            return;
        }
    }
}

char *mem_get_value(char *var_in) {
    for (int i = 0; i < VAR_STORE_SIZE; i++) {
        if (strcmp(shellmemory[i].var, var_in) == 0) {
            return strdup(shellmemory[i].value);
        }
    }
    return "Variable does not exist";
}

// Allocate one frame for a page.
// lines[0..count-1] are the lines of this page (count <= PAGE_SIZE).
// Returns frame index, or -1 if the frame store is full.
int frame_alloc(char **lines, int count) {
    for (int f = 0; f < NUM_FRAMES; f++) {
        if (!frame_used[f]) {
            frame_used[f] = 1;
            for (int i = 0; i < PAGE_SIZE; i++) {
                int idx = f * PAGE_SIZE + i;
                frame_store[idx] = (i < count) ? strdup(lines[i]) : NULL;
            }
            return f;
        }
    }
    return -1; // frame store full
}
