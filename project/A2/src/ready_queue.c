
#include <stdlib.h>
#include "ready_queue.h"

static PCB *head = NULL;
static PCB *tail = NULL;

void enqueue(PCB *pcb) {
    pcb->next = NULL;

    if (!tail) {
        head = tail = pcb;
    } else {
        tail->next = pcb;
        tail = pcb;
    }
}

void enqueue_sjf(PCB *pcb) {
    pcb->next = NULL;

    if (!head) {
        head = tail = pcb;
        return;
    }

    // Insert before head if shortest
    if (pcb->length < head->length) {
        pcb->next = head;
        head = pcb;
        return;
    }

    // Find insertion point
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
