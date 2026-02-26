#include <stdlib.h>
#include "pcb.h"

static int next_pid = 1;

// -------------- 1.2.1 - PCB ----------------
PCB *pcb_create(int start, int length) {
    PCB *pcb = malloc(sizeof(PCB));

    pcb->pid = next_pid++; // unique identifier
    pcb->start = start;   // script start
    pcb->length = length; // total lines
    pcb->pc = 0;          // current line
    pcb->score = length;  // scheduling priority
    pcb->next = NULL;     // queue link

    return pcb;
}
