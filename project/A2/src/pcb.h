#ifndef PCB_H
#define PCB_H

typedef struct PCB {
    int pid;
    int start;
    int length;
    int pc;
    struct PCB *next;
} PCB;

PCB *pcb_create(int start, int length);

#endif
