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

// ---------------- 1.2.6 - Multithreaded scheduler --------------
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

// --------------------- 1.2.1 Scheduler Logic --------------
void scheduler_run(char *policy) {

    scheduler_active = 1;

    // ----------------- 1.2.3 - Round Robin Policy ----------------------
    // ----------------- 1.2.5 - Background Mode ----------------------
    if (strcmp(policy, "RR") == 0 || strcmp(policy, "RR30") == 0) {
        int quantum = (strcmp(policy, "RR30") == 0) ? 30 : 2;

        // Process Selection
        while (!queue_empty()) {
            PCB *pcb = dequeue();

            // Instruction execution
            int executed = 0;
            while (pcb->pc < pcb->length && executed < quantum) {
                char *line = program_memory[pcb->start + pcb->pc];
                parseInput(line);
                pcb->pc++;
                executed++;
            }

            // Post execution
            if (pcb->pc < pcb->length) {
                enqueue(pcb);  // Not finished
            } else {
                program_free(pcb->start, pcb->length);
                free(pcb);
            }
        }
    // ----------------- 1.2.4 - SJF with Aging Policy -----------------------
    } else if (strcmp(policy, "AGING") == 0) {
        PCB *current = dequeue();

        while (current) {
            // Execute one instruction
            char *line = program_memory[current->start + current->pc];
            parseInput(line);
            current->pc++;

            if (current->pc >= current->length) {
                // Current job finished
                program_free(current->start, current->length);
                free(current);
                age_queue();
                current = dequeue();
            } else {
                age_queue();

                // Check if head of queue has strictly lower score
                // Otherwise current continues
                PCB *head_pcb = peek();
                if (head_pcb && head_pcb->score < current->score) {
                    // Put current back, switch to new head
                    enqueue_aging(current);
                    current = dequeue();
                }
            }
        }
    } else {
        // -------------- 1.2.3 - SJF ---------------------------
        // Also FCFS Policy
        while (!queue_empty()) {
            PCB *pcb = dequeue();

            // run process to completion, feed each line to PARSE input
            while (pcb->pc < pcb->length) {
                char *line = program_memory[pcb->start + pcb->pc];
                parseInput(line);
                pcb->pc++;
            }

            // cleanup step, free program lines + pcb itself
            program_free(pcb->start, pcb->length);
            free(pcb);
        }
    }

    scheduler_active = 0;
}
