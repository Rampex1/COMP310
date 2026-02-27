
#include <stdlib.h>
#include "ready_queue.h"

static PCB *head = NULL;
static PCB *tail = NULL;

// --------- 1.2.1 - Ready Queue  ---------------------
void enqueue(PCB *pcb) {
    pcb->next = NULL;

    if (!tail) {
        head = tail = pcb;
    } else {
        tail->next = pcb;
        tail = pcb;
    }
}

void enqueue_head(PCB *pcb) {
    pcb->next = head;
    head = pcb;
    if (!tail) tail = pcb;
}

void enqueue_sjf(PCB *pcb) {
    pcb->next = NULL;

    if (!head) {
        head = tail = pcb;
        return;
    }

    // insert before head, if shortest
    if (pcb->length < head->length) {
        pcb->next = head;
        head = pcb;
        return;
    }

    // find insertion pt
    PCB *cur = head;
    while (cur->next && cur->next->length <= pcb->length) {
        cur = cur->next;
    }
    pcb->next = cur->next;
    cur->next = pcb;

    if (!pcb->next) {
        tail = pcb;
    }
}

void enqueue_aging(PCB *pcb) {
    pcb->next = NULL;

    if (!head) {
        head = tail = pcb;
        return;
    }

    // again insert head if shortest score
    if (pcb->score < head->score) {
        pcb->next = head;
        head = pcb;
        return;
    }

    PCB *cur = head;
    while (cur->next && cur->next->score <= pcb->score) {
        cur = cur->next;
    }
    pcb->next = cur->next;
    cur->next = pcb;

    if (!pcb->next) {
        tail = pcb;
    }
}

void age_queue() {
    PCB *cur = head;
    while (cur) {
        if (cur->score > 0) {
            cur->score--;
        }
        cur = cur->next;
    }
}

PCB *dequeue() {
    if (!head) return NULL;

    PCB *tmp = head;
    head = head->next;

    if (!head) tail = NULL;

    return tmp;
}

PCB *peek() {
    return head;
}

int queue_empty() {
    return head == NULL;
}
