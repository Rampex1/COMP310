#ifndef PCB_H
#define PCB_H

#include "script.h"

typedef struct PCB {
    int pid;
    int pc;          // current logical line index (0-based within the program)
    int length;      // total lines in the program (= script->total_lines)
    ScriptInfo *script;
    int score;       // used by SJF / AGING
    struct PCB *next;
} PCB;

PCB *pcb_create(ScriptInfo *script);

#endif
