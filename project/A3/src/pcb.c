#include <stdlib.h>
#include <string.h>
#include "pcb.h"

static int next_pid = 1;

PCB *pcb_create(int length, int num_pages, int *page_table) {
    PCB *pcb = malloc(sizeof(PCB));
    pcb->pid       = next_pid++;
    pcb->pc        = 0;
    pcb->length    = length;
    pcb->num_pages = num_pages;
    memcpy(pcb->page_table, page_table, num_pages * sizeof(int));
    pcb->score     = length;  // used by SJF / AGING
    pcb->next      = NULL;
    return pcb;
}
