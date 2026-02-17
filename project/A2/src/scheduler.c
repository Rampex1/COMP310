#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "scheduler.h"
#include "ready_queue.h"
#include "shellmemory.h"
#include "pcb.h"

extern int parseInput(char inp[]);

int scheduler_active = 0;

// MT state
int mt_enabled = 0;
pthread_t mt_workers[2];
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t work_cond = PTHREAD_COND_INITIALIZER;
static pthread_cond_t all_done_cond = PTHREAD_COND_INITIALIZER;
static int active_workers = 0;
static int mt_shutdown = 0;
static int mt_quantum = 2;

void *worker_thread(void *arg) {
    (void)arg;

    while (1) {
        pthread_mutex_lock(&queue_mutex);

        while (queue_empty() && !mt_shutdown) {
            pthread_cond_wait(&work_cond, &queue_mutex);
        }

        if (mt_shutdown && queue_empty()) {
            pthread_mutex_unlock(&queue_mutex);
            break;
        }

        if (queue_empty()) {
            pthread_mutex_unlock(&queue_mutex);
            continue;
        }

        PCB *pcb = dequeue();
        active_workers++;
        pthread_mutex_unlock(&queue_mutex);

        // Execute quantum instructions
        int executed = 0;
        while (pcb->pc < pcb->length && executed < mt_quantum) {
            char *line = program_memory[pcb->start + pcb->pc];
            parseInput(line);
            pcb->pc++;
            executed++;
        }

        pthread_mutex_lock(&queue_mutex);
        active_workers--;

        if (pcb->pc < pcb->length) {
            enqueue(pcb);
        } else {
            program_free(pcb->start, pcb->length);
            free(pcb);
        }

        if (queue_empty() && active_workers == 0) {
            pthread_cond_signal(&all_done_cond);
        }
        pthread_cond_broadcast(&work_cond);
        pthread_mutex_unlock(&queue_mutex);
    }

    return NULL;
}

void scheduler_run_mt(char *policy) {
    mt_quantum = (strcmp(policy, "RR30") == 0) ? 30 : 2;
    mt_shutdown = 0;
    active_workers = 0;
    mt_enabled = 1;
    scheduler_active = 1;

    pthread_create(&mt_workers[0], NULL, worker_thread, NULL);
    pthread_create(&mt_workers[1], NULL, worker_thread, NULL);

    // Wake workers and return immediately (non-blocking)
    pthread_mutex_lock(&queue_mutex);
    pthread_cond_broadcast(&work_cond);
    pthread_mutex_unlock(&queue_mutex);
}

void mt_join_workers() {
    if (!mt_enabled) return;

    // Wait for all work to finish
    pthread_mutex_lock(&queue_mutex);
    while (!queue_empty() || active_workers > 0) {
        pthread_cond_wait(&all_done_cond, &queue_mutex);
    }
    mt_shutdown = 1;
    pthread_cond_broadcast(&work_cond);
    pthread_mutex_unlock(&queue_mutex);

    pthread_join(mt_workers[0], NULL);
    pthread_join(mt_workers[1], NULL);

    mt_enabled = 0;
    scheduler_active = 0;
}

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
