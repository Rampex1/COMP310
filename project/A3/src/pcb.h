#ifndef PCB_H
#define PCB_H

#define MAX_PAGES 1000

typedef struct PCB {
    int pid;
    int pc;                     // current line index (0-based within the program)
    int length;                 // total number of lines in the program
    int num_pages;              // number of pages
    int page_table[MAX_PAGES];  // page_table[p] = frame number in frame store
    int score;                  // scheduling priority (for SJF/AGING)
    struct PCB *next;
} PCB;

PCB *pcb_create(int length, int num_pages, int *page_table);

#endif
