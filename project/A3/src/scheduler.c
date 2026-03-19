#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "scheduler.h"
#include "ready_queue.h"
#include "shellmemory.h"
#include "script.h"
#include "pcb.h"

extern int parseInput(char inp[]);

int scheduler_active = 0;

// MT state
int mt_enabled = 0;
pthread_t mt_workers[2];
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t work_cond   = PTHREAD_COND_INITIALIZER;
static pthread_cond_t all_done_cond = PTHREAD_COND_INITIALIZER;
static int active_workers = 0;
static int mt_shutdown    = 0;
static int mt_quantum     = 2;

// Translate logical PC to a frame_store pointer and update LRU.
// Returns NULL if the page is not loaded (page fault required).
static char *get_line(PCB *pcb, int pc) {
    int page   = pc / PAGE_SIZE;
    int offset = pc % PAGE_SIZE;
    int frame  = pcb->script->page_table[page];
    if (frame == -1) return NULL;  // page not in memory
    frame_touch(frame);            // record this frame as most recently used
    return frame_store[frame * PAGE_SIZE + offset];
}

// Check whether the page holding `pc` is loaded.
static int page_loaded(PCB *pcb, int pc) {
    return pcb->script->page_table[pc / PAGE_SIZE] != -1;
}

// ---------------- Multithreaded scheduler --------------
void *worker_thread(void *arg) {
    (void)arg;
    while (1) {
        pthread_mutex_lock(&queue_mutex);
        while (queue_empty() && !mt_shutdown)
            pthread_cond_wait(&work_cond, &queue_mutex);
        if (mt_shutdown && queue_empty()) { pthread_mutex_unlock(&queue_mutex); break; }
        if (queue_empty())               { pthread_mutex_unlock(&queue_mutex); continue; }

        PCB *pcb = dequeue();
        active_workers++;
        pthread_mutex_unlock(&queue_mutex);

        int executed = 0, faulted = 0;
        while (pcb->pc < pcb->length && executed < mt_quantum) {
            if (!page_loaded(pcb, pcb->pc)) {
                load_page_from_script(pcb->script, pcb->pc / PAGE_SIZE);
                pthread_mutex_lock(&queue_mutex);
                enqueue(pcb);
                pthread_mutex_unlock(&queue_mutex);
                faulted = 1;
                break;
            }
            parseInput(get_line(pcb, pcb->pc));
            pcb->pc++;
            executed++;
        }

        pthread_mutex_lock(&queue_mutex);
        active_workers--;
        if (!faulted) {
            if (pcb->pc < pcb->length) enqueue(pcb);
            else                       free(pcb);
        }
        if (queue_empty() && active_workers == 0) pthread_cond_signal(&all_done_cond);
        pthread_cond_broadcast(&work_cond);
        pthread_mutex_unlock(&queue_mutex);
    }
    return NULL;
}

void scheduler_run_mt(char *policy) {
    mt_quantum  = (strcmp(policy, "RR30") == 0) ? 30 : 2;
    mt_shutdown = 0; active_workers = 0;
    mt_enabled  = 1; scheduler_active = 1;
    pthread_create(&mt_workers[0], NULL, worker_thread, NULL);
    pthread_create(&mt_workers[1], NULL, worker_thread, NULL);
    pthread_mutex_lock(&queue_mutex);
    pthread_cond_broadcast(&work_cond);
    pthread_mutex_unlock(&queue_mutex);
}

void mt_join_workers() {
    if (!mt_enabled) return;
    pthread_mutex_lock(&queue_mutex);
    while (!queue_empty() || active_workers > 0)
        pthread_cond_wait(&all_done_cond, &queue_mutex);
    mt_shutdown = 1;
    pthread_cond_broadcast(&work_cond);
    pthread_mutex_unlock(&queue_mutex);
    pthread_join(mt_workers[0], NULL);
    pthread_join(mt_workers[1], NULL);
    mt_enabled = 0; scheduler_active = 0;
}

// --------------------- Single-threaded scheduler --------------
void scheduler_run(char *policy) {
    scheduler_active = 1;

    if (strcmp(policy, "RR") == 0 || strcmp(policy, "RR30") == 0) {
        int quantum = (strcmp(policy, "RR30") == 0) ? 30 : 2;

        while (!queue_empty()) {
            PCB *pcb = dequeue();
            int executed = 0, faulted = 0;

            while (pcb->pc < pcb->length && executed < quantum) {
                if (!page_loaded(pcb, pcb->pc)) {
                    load_page_from_script(pcb->script, pcb->pc / PAGE_SIZE);
                    enqueue(pcb);
                    faulted = 1;
                    break;
                }
                parseInput(get_line(pcb, pcb->pc));
                pcb->pc++;
                executed++;
            }

            if (!faulted) {
                if (pcb->pc < pcb->length) enqueue(pcb);
                else                       free(pcb);
            }
        }

    } else if (strcmp(policy, "AGING") == 0) {
        PCB *current = dequeue();
        while (current) {
            if (!page_loaded(current, current->pc)) {
                load_page_from_script(current->script, current->pc / PAGE_SIZE);
                enqueue_aging(current);
                age_queue();
                current = dequeue();
                continue;
            }
            parseInput(get_line(current, current->pc));
            current->pc++;

            if (current->pc >= current->length) {
                free(current);
                age_queue();
                current = dequeue();
            } else {
                age_queue();
                PCB *head_pcb = peek();
                if (head_pcb && head_pcb->score < current->score) {
                    enqueue_aging(current);
                    current = dequeue();
                }
            }
        }

    } else {
        // FCFS and SJF: run each process to completion (with page fault support)
        while (!queue_empty()) {
            PCB *pcb = dequeue();
            int faulted = 0;

            while (pcb->pc < pcb->length) {
                if (!page_loaded(pcb, pcb->pc)) {
                    load_page_from_script(pcb->script, pcb->pc / PAGE_SIZE);
                    enqueue(pcb);
                    faulted = 1;
                    break;
                }
                parseInput(get_line(pcb, pcb->pc));
                pcb->pc++;
            }

            if (!faulted && pcb->pc >= pcb->length) free(pcb);
        }
    }

    scheduler_active = 0;
}
