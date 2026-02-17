#ifndef READY_QUEUE_H
#define READY_QUEUE_H

#include "pcb.h"

void enqueue(PCB *pcb);
void enqueue_head(PCB *pcb);
void enqueue_sjf(PCB *pcb);
void enqueue_aging(PCB *pcb);
void age_queue();
PCB *dequeue();
PCB *peek();
int queue_empty();

#endif

