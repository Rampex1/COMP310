#include <stdlib.h>
#include "scheduler.h"
#include "ready_queue.h"
#include "shellmemory.h"
#include "pcb.h"

extern int parseInput(char inp[]);

void scheduler_run() {

    while (!queue_empty()) {

        PCB *pcb = peek();

        while (pcb->pc < pcb->length) {

            char *line = program_memory[pcb->start + pcb->pc];

            parseInput(line);

            pcb->pc++;
        }

        PCB *done = dequeue();

        program_free(done->start, done->length);

        free(done);
    }
}
