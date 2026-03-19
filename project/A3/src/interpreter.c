#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shellmemory.h"
#include "script.h"
#include "shell.h"
#include "pcb.h"
#include "ready_queue.h"
#include "scheduler.h"
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <pthread.h>

int MAX_ARGS_SIZE = 7;

// ------------------------------------------------------------------
// Script registry and frame→owner reverse mapping (demand paging)
// ------------------------------------------------------------------

#define MAX_LOADED_SCRIPTS 100

static ScriptInfo loaded_scripts[MAX_LOADED_SCRIPTS];
static int        num_loaded_scripts = 0;

// Reverse mapping: for each frame, which ScriptInfo owns it and which page.
static ScriptInfo *frame_owner[NUM_FRAMES];
static int         frame_page_num[NUM_FRAMES];  // which page of frame_owner[f]

// Load the missing page (page_idx) of `script` into the frame store.
// Evicts a random frame (rand() % NUM_FRAMES) if the store is full.
// Prints "Page fault!" messages as required.
// Updates script->page_table[page_idx] and the reverse mapping.
// Returns the frame number used.
int load_page_from_script(ScriptInfo *script, int page_idx) {
    int start_line = page_idx * PAGE_SIZE;
    char *page_lines[PAGE_SIZE];
    int count = 0;
    for (int i = 0; i < PAGE_SIZE; i++) {
        int li = start_line + i;
        page_lines[i] = (li < script->total_lines) ? script->lines[li] : NULL;
        if (page_lines[i]) count++;
    }

    int frame = frame_alloc(page_lines, count);

    if (frame < 0) {
        // Frame store is full: evict the least recently used frame
        int victim = frame_lru_victim();

        printf("Page fault! Victim page contents:\n\n");
        // Print from backing store (unmodified by parseInput)
        if (frame_owner[victim]) {
            int vpage = frame_page_num[victim];
            for (int i = 0; i < PAGE_SIZE; i++) {
                int li = vpage * PAGE_SIZE + i;
                if (li < frame_owner[victim]->total_lines)
                    printf("%s", frame_owner[victim]->lines[li]);
            }
        }
        printf("\nEnd of victim page contents.\n");

        // Invalidate victim's page table entry
        if (frame_owner[victim]) {
            frame_owner[victim]->page_table[frame_page_num[victim]] = -1;
            frame_owner[victim] = NULL;
        }

        // Free the victim frame, then allocate into it
        frame_free(victim);
        frame = frame_alloc(page_lines, count);
        // frame == victim (only free slot)
    } else {
        printf("Page fault!\n");
    }

    // Update page table and reverse mapping
    script->page_table[page_idx] = frame;
    frame_owner[frame]    = script;
    frame_page_num[frame] = page_idx;

    return frame;
}

// ------------------------------------------------------------------
// Script loading
// ------------------------------------------------------------------

// Read all lines from fp into *lines_out (malloc'd array of strdup'd strings).
// Caller receives the total line count.
static int read_all_lines(FILE *fp, char ***lines_out) {
    char line[MAX_LINE_LENGTH];
    int capacity = 64;
    char **lines = malloc(capacity * sizeof(char *));
    int count = 0;

    while (fgets(line, MAX_LINE_LENGTH, fp)) {
        if (count == capacity) {
            capacity *= 2;
            lines = realloc(lines, capacity * sizeof(char *));
        }
        lines[count++] = strdup(line);
    }
    *lines_out = lines;
    return count;
}

// Load a named script.  If already in the registry, return the cached entry.
// On first load: read ALL lines into the backing store, load the first 2 pages
// (or 1 page if the program has fewer than 3 lines) into the frame store,
// mark remaining pages as -1.
// Returns pointer to the ScriptInfo entry, or NULL on error.
static ScriptInfo *load_program(const char *filename) {
    // Check registry for existing entry
    for (int i = 0; i < num_loaded_scripts; i++) {
        if (strcmp(loaded_scripts[i].filename, filename) == 0)
            return &loaded_scripts[i];
    }

    if (num_loaded_scripts >= MAX_LOADED_SCRIPTS) return NULL;

    FILE *fp = fopen(filename, "rt");
    if (!fp) return NULL;

    char **lines;
    int total_lines = read_all_lines(fp, &lines);
    fclose(fp);

    int num_pages = (total_lines + PAGE_SIZE - 1) / PAGE_SIZE;

    // Register the script
    ScriptInfo *si = &loaded_scripts[num_loaded_scripts++];
    strncpy(si->filename, filename, 255);
    si->filename[255] = '\0';
    si->lines       = lines;
    si->total_lines = total_lines;
    si->num_pages   = num_pages;

    // All pages start as not loaded
    for (int p = 0; p < num_pages; p++) si->page_table[p] = -1;

    // In PAGING_MODE (A3): load only the first 2 pages; the rest are demand-loaded.
    // Without PAGING_MODE (A2 compatibility): load all pages upfront.
#ifdef PAGING_MODE
    int pages_to_load = (total_lines < PAGE_SIZE) ? 1 : 2;
    pages_to_load = (pages_to_load < num_pages) ? pages_to_load : num_pages;
#else
    int pages_to_load = num_pages;
#endif

    for (int p = 0; p < pages_to_load; p++) {
        int start_line = p * PAGE_SIZE;
        char *page_lines[PAGE_SIZE];
        int count = 0;
        for (int i = 0; i < PAGE_SIZE; i++) {
            int li = start_line + i;
            page_lines[i] = (li < total_lines) ? lines[li] : NULL;
            if (page_lines[i]) count++;
        }
        int frame = frame_alloc(page_lines, count);
        if (frame < 0) {
            printf("Error: frame store full\n");
            return si;  // Partially loaded; page faults will handle the rest
        }
        si->page_table[p] = frame;
        frame_owner[frame]    = si;
        frame_page_num[frame] = p;
    }

    return si;
}

// ------------------------------------------------------------------
// Error helpers
// ------------------------------------------------------------------
int badcommand() {
    printf("Unknown Command\n");
    return 1;
}

int badcommandFileDoesNotExist() {
    printf("Bad command: File not found\n");
    return 3;
}

// ------------------------------------------------------------------
// Command implementations
// ------------------------------------------------------------------
int help();
int quit();
int set(char *var, char *value);
int print(char *var);
int source(char *script);
int echo(char *var);
int my_ls();
int my_mkdir(char *dirname);
int my_touch(char *filename);
int my_cd(char *dirname);
int run(char *args[], int args_size);
int my_exec(char *args[], int args_size);

int interpreter(char *command_args[], int args_size) {
    if (args_size < 1 || args_size > MAX_ARGS_SIZE) return badcommand();

    for (int i = 0; i < args_size; i++)
        command_args[i][strcspn(command_args[i], "\r\n")] = 0;

    if      (strcmp(command_args[0], "help")     == 0) { if (args_size != 1) return badcommand(); return help(); }
    else if (strcmp(command_args[0], "quit")     == 0) { if (args_size != 1) return badcommand(); return quit(); }
    else if (strcmp(command_args[0], "set")      == 0) { if (args_size != 3) return badcommand(); return set(command_args[1], command_args[2]); }
    else if (strcmp(command_args[0], "print")    == 0) { if (args_size != 2) return badcommand(); return print(command_args[1]); }
    else if (strcmp(command_args[0], "source")   == 0) { if (args_size != 2) return badcommand(); return source(command_args[1]); }
    else if (strcmp(command_args[0], "echo")     == 0) { if (args_size != 2) return badcommand(); return echo(command_args[1]); }
    else if (strcmp(command_args[0], "my_ls")    == 0) { if (args_size != 1) return badcommand(); return my_ls(); }
    else if (strcmp(command_args[0], "my_mkdir") == 0) { if (args_size != 2) return badcommand(); return my_mkdir(command_args[1]); }
    else if (strcmp(command_args[0], "my_touch") == 0) { if (args_size != 2) return badcommand(); return my_touch(command_args[1]); }
    else if (strcmp(command_args[0], "my_cd")    == 0) { if (args_size != 2) return badcommand(); return my_cd(command_args[1]); }
    else if (strcmp(command_args[0], "run")      == 0) { if (args_size <  2) return badcommand(); return run(command_args, args_size); }
    else if (strcmp(command_args[0], "exec")     == 0) { if (args_size <  3) return badcommand(); return my_exec(command_args, args_size); }
    else return badcommand();
}

int help() {
    char help_string[] = "COMMAND			DESCRIPTION\n \
help			Displays all the commands\n \
quit			Exits / terminates the shell with \"Bye!\"\n \
set VAR STRING		Assigns a value to shell memory\n \
print VAR		Displays the STRING assigned to VAR\n \
source SCRIPT.TXT	Executes the file SCRIPT.TXT\n ";
    printf("%s\n", help_string);
    return 0;
}

int quit() {
    printf("Bye!\n");
    if (mt_enabled) {
        if (pthread_self() == mt_workers[0] || pthread_self() == mt_workers[1]) return 0;
        mt_join_workers();
    }
    exit(0);
}

int set(char *var, char *value) { mem_set_value(var, value); return 0; }

int print(char *var) { printf("%s\n", mem_get_value(var)); return 0; }

int echo(char *var) {
    if (var[0] == '$') {
        char *value = mem_get_value(var + 1);
        printf("%s\n", (strcmp(value, "Variable does not exist") == 0) ? "" : value);
    } else {
        printf("%s\n", var);
    }
    return 0;
}

int cmpstringp(const void *p1, const void *p2) {
    return strcmp(*(char *const *)p1, *(char *const *)p2);
}

int my_ls() {
    struct dirent *entry; DIR *dp = opendir(".");
    if (!dp) { perror("opendir"); return 1; }
    char *names[1024]; int count = 0;
    while ((entry = readdir(dp)) != NULL) names[count++] = strdup(entry->d_name);
    closedir(dp);
    qsort(names, count, sizeof(char *), cmpstringp);
    for (int i = 0; i < count; i++) { printf("%s\n", names[i]); free(names[i]); }
    return 0;
}

int is_alphanumeric_string(char *s) {
    if (!s || !strlen(s)) return 0;
    for (int i = 0; s[i]; i++) if (!isalnum(s[i])) return 0;
    return 1;
}

int my_mkdir(char *dirname) {
    char final_name[256];
    if (dirname[0] == '$') {
        char *value = mem_get_value(dirname + 1);
        if (strcmp(value, "Variable does not exist") == 0 || !is_alphanumeric_string(value))
            { printf("Bad command: my_mkdir\n"); return 1; }
        strcpy(final_name, value);
    } else {
        if (!is_alphanumeric_string(dirname)) { printf("Bad command: my_mkdir\n"); return 1; }
        strcpy(final_name, dirname);
    }
    if (mkdir(final_name, 0777) != 0) { printf("Bad command: my_mkdir\n"); return 1; }
    return 0;
}

int my_touch(char *filename) {
    if (!is_alphanumeric_string(filename)) { printf("Bad command: my_touch\n"); return 1; }
    int fd = open(filename, O_CREAT | O_WRONLY, 0666);
    if (fd < 0) { printf("Bad command: my_touch\n"); return 1; }
    close(fd); return 0;
}

int my_cd(char *dirname) {
    if (!is_alphanumeric_string(dirname)) { printf("Bad command: my_cd\n"); return 1; }
    struct stat sb;
    if (stat(dirname, &sb) != 0 || !S_ISDIR(sb.st_mode)) { printf("Bad command: my_cd\n"); return 1; }
    if (chdir(dirname) != 0) { printf("Bad command: my_cd\n"); return 1; }
    return 0;
}

int source(char *script) {
    ScriptInfo *si = load_program(script);
    if (!si) return badcommandFileDoesNotExist();

    PCB *pcb = pcb_create(si);
    enqueue(pcb);
    scheduler_run("FCFS");
    return 0;
}

int my_exec(char *args[], int args_size) {
    int background = 0, mt = 0;
    int last = args_size - 1;

    if (strcmp(args[last], "MT") == 0) { mt = 1; last--; }
    if (strcmp(args[last], "#")  == 0) { background = 1; last--; }

    char *policy  = args[last];
    int num_files = last - 1;

    if (strcmp(policy, "FCFS") != 0 && strcmp(policy, "SJF")   != 0 &&
        strcmp(policy, "RR")   != 0 && strcmp(policy, "RR30")  != 0 &&
        strcmp(policy, "AGING") != 0)
        return badcommand();

    if (num_files < 1 || num_files > 3) return badcommand();

    // Load each script (duplicates share frames via the registry)
    ScriptInfo *scripts[3];
    for (int i = 0; i < num_files; i++) {
        scripts[i] = load_program(args[i + 1]);
        if (!scripts[i]) return badcommandFileDoesNotExist();
    }

    // Background mode: read remaining stdin into a temporary backing store
    // and create a PCB from it.
    ScriptInfo bg_si;
    int bg_valid = 0;
    if (background) {
        char **bg_lines;
        int bg_total = read_all_lines(stdin, &bg_lines);
        if (bg_total > 0) {
            bg_valid = 1;
            memset(&bg_si, 0, sizeof(bg_si));
            strncpy(bg_si.filename, "<stdin>", 255);
            bg_si.lines       = bg_lines;
            bg_si.total_lines = bg_total;
            bg_si.num_pages   = (bg_total + PAGE_SIZE - 1) / PAGE_SIZE;
            for (int p = 0; p < bg_si.num_pages; p++) bg_si.page_table[p] = -1;
            // Load first 2 pages
            int to_load = (bg_total < PAGE_SIZE) ? 1 : 2;
            to_load = (to_load < bg_si.num_pages) ? to_load : bg_si.num_pages;
            for (int p = 0; p < to_load; p++) {
                int start = p * PAGE_SIZE;
                char *pl[PAGE_SIZE]; int cnt = 0;
                for (int i = 0; i < PAGE_SIZE; i++) {
                    int li = start + i;
                    pl[i] = (li < bg_total) ? bg_lines[li] : NULL;
                    if (pl[i]) cnt++;
                }
                int frame = frame_alloc(pl, cnt);
                if (frame >= 0) {
                    bg_si.page_table[p] = frame;
                    frame_owner[frame]    = &bg_si;
                    frame_page_num[frame] = p;
                }
            }
        }
    }

    if (mt_enabled) pthread_mutex_lock(&queue_mutex);

    for (int i = 0; i < num_files; i++) {
        PCB *pcb = pcb_create(scripts[i]);
        if      (strcmp(policy, "SJF")   == 0) enqueue_sjf(pcb);
        else if (strcmp(policy, "AGING") == 0) enqueue_aging(pcb);
        else                                   enqueue(pcb);
    }

    if (background && bg_valid) {
        PCB *bg_pcb = pcb_create(&bg_si);
        enqueue_head(bg_pcb);
    }

    if (mt_enabled) {
        pthread_cond_broadcast(&work_cond);
        pthread_mutex_unlock(&queue_mutex);
    }

    if (!scheduler_active) {
        if (mt) scheduler_run_mt(policy);
        else    scheduler_run(policy);
    }

    return 0;
}

int run(char *args[], int args_size) {
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }
    if (pid == 0) {
        char *cmd_args[args_size];
        for (int i = 1; i < args_size; i++) cmd_args[i-1] = args[i];
        cmd_args[args_size-1] = NULL;
        execvp(cmd_args[0], cmd_args);
        perror("execvp"); _exit(1);
    } else {
        int status; waitpid(pid, &status, 0);
    }
    return 0;
}
