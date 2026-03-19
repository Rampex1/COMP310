#ifndef SCRIPT_H
#define SCRIPT_H

#include "shellmemory.h"

// Represents a loaded script: its backing store (all lines) and page table.
// Multiple PCBs running the same script share one ScriptInfo.
typedef struct ScriptInfo {
    char filename[256];
    char **lines;              // backing store: all lines (strdup'd from file)
    int  total_lines;
    int  num_pages;
    int  page_table[MAX_PAGES]; // page_table[p] = frame index, or -1 if not in frame store
} ScriptInfo;

// Load the missing page (page_idx) of script into the frame store.
// Evicts a random frame if the store is full (prints appropriate message).
// Updates script->page_table and the frame→owner reverse mapping.
// Returns the frame number used.
int load_page_from_script(ScriptInfo *script, int page_idx);

#endif
