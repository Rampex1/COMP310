#include <stdlib.h>
#include "pcb.h"

static int next_pid = 1;

PCB *pcb_create(ScriptInfo *script) {
    PCB *pcb    = malloc(sizeof(PCB));
    pcb->pid    = next_pid++;
    pcb->pc     = 0;
    pcb->length = script->total_lines;
    pcb->script = script;
    pcb->score  = script->total_lines;  // initial priority for SJF / AGING
    pcb->next   = NULL;
    return pcb;
}
