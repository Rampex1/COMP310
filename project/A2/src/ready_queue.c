
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
