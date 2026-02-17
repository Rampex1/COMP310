#include <stdlib.h>
#include "pcb.h"

static int next_pid = 1;

PCB *pcb_create(int start, int length) {
    PCB *pcb = malloc(sizeof(PCB));

    pcb->pid = next_pid++;
    pcb->start = start;
    pcb->length = length;
    pcb->pc = 0;
    pcb->next = NULL;

    return pcb;
}
