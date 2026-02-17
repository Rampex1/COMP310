#ifndef SHELLMEMORY_H
#define SHELLMEMORY_H

#include <stdio.h>

#define MEM_SIZE 1000

#define MAX_PROGRAM_LINES 1000
#define MAX_LINE_LENGTH 100

void mem_init();
char *mem_get_value(char *var);
void mem_set_value(char *var, char *value);

extern char *program_memory[MAX_PROGRAM_LINES];
extern int program_used[MAX_PROGRAM_LINES];

int program_load(FILE *fp);
void program_free(int start, int length);

#endif
