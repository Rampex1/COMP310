#ifndef PCB_H
#define PCB_H

typedef struct PCB {
    int pid;
    int start;
    int length;
    int pc;
    int score;
    struct PCB *next;
} PCB;

PCB *pcb_create(int start, int length);

#endif
