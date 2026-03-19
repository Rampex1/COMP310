#ifndef SHELLMEMORY_H
#define SHELLMEMORY_H

#include <stdio.h>

#define PAGE_SIZE 3

#ifndef FRAME_STORE_SIZE
#define FRAME_STORE_SIZE 1000
#endif

#ifndef VAR_STORE_SIZE
#define VAR_STORE_SIZE 1000
#endif

#define NUM_FRAMES (FRAME_STORE_SIZE / PAGE_SIZE)

#define MAX_PAGES 1000
#define MAX_LINE_LENGTH 100

void mem_init();
char *mem_get_value(char *var);
void mem_set_value(char *var, char *value);

extern char *frame_store[FRAME_STORE_SIZE];
extern int frame_used[NUM_FRAMES];

// Allocate one frame: stores `count` lines (count <= PAGE_SIZE).
// Returns the frame index, or -1 if the frame store is full.
int frame_alloc(char **lines, int count);

#endif
