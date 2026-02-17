#include <stdlib.h>
#include <string.h>
#include "scheduler.h"
#include "ready_queue.h"
#include "shellmemory.h"
#include "pcb.h"

extern int parseInput(char inp[]);

void scheduler_run(char *policy) {

    if (strcmp(policy, "RR") == 0) {
        // Round Robin: quantum of 2
        int quantum = 2;

        while (!queue_empty()) {
            PCB *pcb = dequeue();

            int executed = 0;
            while (pcb->pc < pcb->length && executed < quantum) {
                char *line = program_memory[pcb->start + pcb->pc];
                parseInput(line);
                pcb->pc++;
                executed++;
            }

            if (pcb->pc < pcb->length) {
                // Not finished, re-enqueue
                enqueue(pcb);
            } else {
                // Finished
                program_free(pcb->start, pcb->length);
                free(pcb);
            }
        }
    } else {
        // FCFS and SJF: both run each process to completion
        // (SJF sorts at enqueue time, so queue order is already correct)
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
}
