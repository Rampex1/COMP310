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
int   frame_used[NUM_FRAMES];

// LRU tracking
int frame_lru[NUM_FRAMES];
int lru_clock = 0;

void mem_init() {
    for (int i = 0; i < VAR_STORE_SIZE; i++) {
        shellmemory[i].var   = "none";
        shellmemory[i].value = "none";
    }
    for (int i = 0; i < FRAME_STORE_SIZE; i++) frame_store[i] = NULL;
    for (int i = 0; i < NUM_FRAMES; i++) { frame_used[i] = 0; frame_lru[i] = 0; }
    lru_clock = 0;
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
        if (strcmp(shellmemory[i].var, var_in) == 0)
            return strdup(shellmemory[i].value);
    }
    return "Variable does not exist";
}

// Find the first free frame, load `count` lines into it, and touch it for LRU.
int frame_alloc(char **lines, int count) {
    for (int f = 0; f < NUM_FRAMES; f++) {
        if (!frame_used[f]) {
            frame_used[f] = 1;
            for (int i = 0; i < PAGE_SIZE; i++) {
                int idx = f * PAGE_SIZE + i;
                frame_store[idx] = (i < count && lines[i]) ? strdup(lines[i]) : NULL;
            }
            frame_touch(f);  // give this frame a fresh LRU timestamp
            return f;
        }
    }
    return -1;
}

void frame_touch(int frame) {
    frame_lru[frame] = ++lru_clock;
}

// Return the index of the least recently used frame (minimum timestamp).
int frame_lru_victim(void) {
    int victim = -1, min_lru = lru_clock + 1;
    for (int f = 0; f < NUM_FRAMES; f++) {
        if (frame_used[f] && frame_lru[f] < min_lru) {
            min_lru = frame_lru[f];
            victim  = f;
        }
    }
    return victim;
}

// Release a frame: free its strings and mark it available.
void frame_free(int frame) {
    frame_used[frame] = 0;
    for (int i = 0; i < PAGE_SIZE; i++) {
        int idx = frame * PAGE_SIZE + i;
        if (frame_store[idx]) { free(frame_store[idx]); frame_store[idx] = NULL; }
    }
}
