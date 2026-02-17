#include <stdlib.h>
#include <string.h>
#include "scheduler.h"
#include "ready_queue.h"
#include "shellmemory.h"
#include "pcb.h"

extern int parseInput(char inp[]);

int scheduler_active = 0;

void scheduler_run(char *policy) {

    scheduler_active = 1;

    if (strcmp(policy, "RR") == 0 || strcmp(policy, "RR30") == 0) {
        int quantum = (strcmp(policy, "RR30") == 0) ? 30 : 2;

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
    } else if (strcmp(policy, "AGING") == 0) {
        // SJF with Aging: quantum of 1, age waiting jobs each slice
        PCB *current = dequeue();

        while (current) {
            // Execute 1 instruction
            char *line = program_memory[current->start + current->pc];
            parseInput(line);
            current->pc++;

            if (current->pc >= current->length) {
                // Current job finished
                program_free(current->start, current->length);
                free(current);

                // Age remaining queue
                age_queue();

                // Get next job
                current = dequeue();
            } else {
                // Age waiting jobs
                age_queue();

                // Check if head of queue has strictly lower score
                PCB *head_pcb = peek();
                if (head_pcb && head_pcb->score < current->score) {
                    // Promotion: put current back, switch to new head
                    enqueue_aging(current);
                    current = dequeue();
                }
                // Otherwise current continues
            }
        }
    } else {
        // FCFS and SJF: both run each process to completion
        // (SJF sorts at enqueue time, so queue order is already correct)
        // Dequeue before running so nested exec doesn't corrupt queue head
        while (!queue_empty()) {
            PCB *pcb = dequeue();

            while (pcb->pc < pcb->length) {
                char *line = program_memory[pcb->start + pcb->pc];
                parseInput(line);
                pcb->pc++;
            }

            program_free(pcb->start, pcb->length);
            free(pcb);
        }
    }

    scheduler_active = 0;
}
