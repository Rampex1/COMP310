#ifndef READY_QUEUE_H
#define READY_QUEUE_H

#include "pcb.h"

void enqueue(PCB *pcb);
PCB *dequeue();
PCB *peek();
int queue_empty();

#endif

