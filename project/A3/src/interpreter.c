#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shellmemory.h"
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

int badcommand() {
    printf("Unknown Command\n");
    return 1;
}

int badcommandFileDoesNotExist() {
    printf("Bad command: File not found\n");
    return 3;
}

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

// Interpret commands and their arguments
int interpreter(char *command_args[], int args_size) {
    int i;

    if (args_size < 1 || args_size > MAX_ARGS_SIZE) {
        return badcommand();
    }

    for (i = 0; i < args_size; i++) {
        command_args[i][strcspn(command_args[i], "\r\n")] = 0;
    }

    if (strcmp(command_args[0], "help") == 0) {
        if (args_size != 1) return badcommand();
        return help();

    } else if (strcmp(command_args[0], "quit") == 0) {
        if (args_size != 1) return badcommand();
        return quit();

    } else if (strcmp(command_args[0], "set") == 0) {
        if (args_size != 3) return badcommand();
        return set(command_args[1], command_args[2]);

    } else if (strcmp(command_args[0], "print") == 0) {
        if (args_size != 2) return badcommand();
        return print(command_args[1]);

    } else if (strcmp(command_args[0], "source") == 0) {
        if (args_size != 2) return badcommand();
        return source(command_args[1]);

    } else if (strcmp(command_args[0], "echo") == 0) {
        if (args_size != 2) return badcommand();
        return echo(command_args[1]);

    } else if (strcmp(command_args[0], "my_ls") == 0) {
        if (args_size != 1) return badcommand();
        return my_ls();

    } else if (strcmp(command_args[0], "my_mkdir") == 0) {
        if (args_size != 2) return badcommand();
        return my_mkdir(command_args[1]);

    } else if (strcmp(command_args[0], "my_touch") == 0) {
        if (args_size != 2) return badcommand();
        return my_touch(command_args[1]);

    } else if (strcmp(command_args[0], "my_cd") == 0) {
        if (args_size != 2) return badcommand();
        return my_cd(command_args[1]);

    } else if (strcmp(command_args[0], "run") == 0) {
        if (args_size < 2) return badcommand();
        return run(command_args, args_size);

    } else if (strcmp(command_args[0], "exec") == 0) {
        if (args_size < 3) return badcommand();
        return my_exec(command_args, args_size);

    } else
        return badcommand();
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
        if (pthread_self() == mt_workers[0] || pthread_self() == mt_workers[1]) {
            return 0;
        }
        mt_join_workers();
    }
    exit(0);
}

int set(char *var, char *value) {
    mem_set_value(var, value);
    return 0;
}

int print(char *var) {
    printf("%s\n", mem_get_value(var));
    return 0;
}

int echo(char *var) {
    if (var[0] == '$') {
        char *var_name = var + 1;
        char *value = mem_get_value(var_name);
        if (strcmp(value, "Variable does not exist") == 0) {
            printf("\n");
        } else {
            printf("%s\n", value);
        }
    } else {
        printf("%s\n", var);
    }
    return 0;
}

int cmpstringp(const void *p1, const void *p2) {
    char *const *s1 = p1;
    char *const *s2 = p2;
    return strcmp(*s1, *s2);
}

int my_ls() {
    struct dirent *entry;
    DIR *dp = opendir(".");
    if (dp == NULL) { perror("opendir"); return 1; }

    char *names[1024];
    int count = 0;
    while ((entry = readdir(dp)) != NULL) names[count++] = strdup(entry->d_name);
    closedir(dp);

    qsort(names, count, sizeof(char *), cmpstringp);
    for (int i = 0; i < count; i++) { printf("%s\n", names[i]); free(names[i]); }
    return 0;
}

int is_alphanumeric_string(char *s) {
    if (s == NULL || strlen(s) == 0) return 0;
    for (int i = 0; s[i]; i++) if (!isalnum(s[i])) return 0;
    return 1;
}

int my_mkdir(char *dirname) {
    char final_name[256];
    if (dirname[0] == '$') {
        char *varname = dirname + 1;
        char *value = mem_get_value(varname);
        if (strcmp(value, "Variable does not exist") == 0 || !is_alphanumeric_string(value)) {
            printf("Bad command: my_mkdir\n"); return 1;
        }
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
    close(fd);
    return 0;
}

int my_cd(char *dirname) {
    if (!is_alphanumeric_string(dirname)) { printf("Bad command: my_cd\n"); return 1; }
    struct stat sb;
    if (stat(dirname, &sb) != 0 || !S_ISDIR(sb.st_mode)) { printf("Bad command: my_cd\n"); return 1; }
    if (chdir(dirname) != 0) { printf("Bad command: my_cd\n"); return 1; }
    return 0;
}

// --------- Paging infrastructure (1.2.1) ---------

// Registry: tracks scripts already loaded into the frame store.
// Allows two processes with the same script name to share frames.
#define MAX_LOADED_SCRIPTS 100

typedef struct {
    char filename[256];
    int  page_table[MAX_PAGES];
    int  num_pages;
    int  length;      // total lines
} LoadedScript;

static LoadedScript loaded_scripts[MAX_LOADED_SCRIPTS];
static int num_loaded_scripts = 0;

// Load a program from an open FILE* into the frame store.
// Fills page_table_out[0..num_pages-1] with frame numbers.
// Returns 0 on success, -1 if frame store is full.
static int load_from_fp(FILE *fp, int *page_table_out, int *num_pages_out, int *length_out) {
    char line[MAX_LINE_LENGTH];
    char *page_lines[PAGE_SIZE];
    int  page_count  = 0;
    int  total_lines = 0;

    while (!feof(fp)) {
        int count = 0;
        for (int i = 0; i < PAGE_SIZE; i++) {
            if (fgets(line, MAX_LINE_LENGTH, fp)) {
                page_lines[i] = strdup(line);
                count++;
            } else {
                page_lines[i] = NULL;
            }
        }
        if (count == 0) break;

        int frame = frame_alloc(page_lines, count);
        for (int i = 0; i < count; i++) { free(page_lines[i]); page_lines[i] = NULL; }

        if (frame < 0) {
            printf("Error: frame store full\n");
            return -1;
        }

        page_table_out[page_count++] = frame;
        total_lines += count;
    }

    *num_pages_out = page_count;
    *length_out    = total_lines;
    return 0;
}

// Load a named script into the frame store.
// If the script was already loaded, share the existing frames.
// Returns 0 on success, -1 if file not found, -2 if frame store full.
static int load_program(const char *filename, int *page_table_out, int *num_pages_out, int *length_out) {
    // Check registry
    for (int i = 0; i < num_loaded_scripts; i++) {
        if (strcmp(loaded_scripts[i].filename, filename) == 0) {
            memcpy(page_table_out, loaded_scripts[i].page_table,
                   loaded_scripts[i].num_pages * sizeof(int));
            *num_pages_out = loaded_scripts[i].num_pages;
            *length_out    = loaded_scripts[i].length;
            return 0;
        }
    }

    FILE *fp = fopen(filename, "rt");
    if (!fp) return -1;

    int page_table[MAX_PAGES];
    int num_pages, length;
    int ret = load_from_fp(fp, page_table, &num_pages, &length);
    fclose(fp);
    if (ret < 0) return -2;

    // Register so future loads of the same file share frames
    if (num_loaded_scripts < MAX_LOADED_SCRIPTS) {
        LoadedScript *ls = &loaded_scripts[num_loaded_scripts++];
        strncpy(ls->filename, filename, 255);
        ls->filename[255] = '\0';
        memcpy(ls->page_table, page_table, num_pages * sizeof(int));
        ls->num_pages = num_pages;
        ls->length    = length;
    }

    memcpy(page_table_out, page_table, num_pages * sizeof(int));
    *num_pages_out = num_pages;
    *length_out    = length;
    return 0;
}

// source command
int source(char *script) {
    int page_table[MAX_PAGES], num_pages, length;
    int ret = load_program(script, page_table, &num_pages, &length);
    if (ret == -1) return badcommandFileDoesNotExist();
    if (ret == -2) return 1;  // error already printed

    PCB *pcb = pcb_create(length, num_pages, page_table);
    enqueue(pcb);
    scheduler_run("FCFS");
    return 0;
}

// exec command
int my_exec(char *args[], int args_size) {
    int background = 0;
    int mt = 0;
    char *policy;
    int num_files;

    int last = args_size - 1;
    if (strcmp(args[last], "MT") == 0) { mt = 1; last--; }
    if (strcmp(args[last], "#") == 0)  { background = 1; last--; }

    policy    = args[last];
    num_files = last - 1;

    if (strcmp(policy, "FCFS") != 0 && strcmp(policy, "SJF") != 0 &&
        strcmp(policy, "RR")   != 0 && strcmp(policy, "RR30") != 0 &&
        strcmp(policy, "AGING") != 0) {
        return badcommand();
    }

    if (num_files < 1 || num_files > 3) return badcommand();

    // Load each script (duplicates share frames via the registry)
    int page_tables[3][MAX_PAGES];
    int num_pages_arr[3], lengths[3];

    for (int i = 0; i < num_files; i++) {
        int ret = load_program(args[i + 1], page_tables[i], &num_pages_arr[i], &lengths[i]);
        if (ret == -1) return badcommandFileDoesNotExist();
        if (ret == -2) return 1;
    }

    // Background mode: load remaining stdin into frames
    int bg_page_table[MAX_PAGES], bg_num_pages = 0, bg_length = 0;
    if (background) {
        load_from_fp(stdin, bg_page_table, &bg_num_pages, &bg_length);
    }

    if (mt_enabled) pthread_mutex_lock(&queue_mutex);

    for (int i = 0; i < num_files; i++) {
        PCB *pcb = pcb_create(lengths[i], num_pages_arr[i], page_tables[i]);
        if (strcmp(policy, "SJF") == 0) {
            enqueue_sjf(pcb);
        } else if (strcmp(policy, "AGING") == 0) {
            enqueue_aging(pcb);
        } else {
            enqueue(pcb);
        }
    }

    if (background && bg_length > 0) {
        PCB *bg_pcb = pcb_create(bg_length, bg_num_pages, bg_page_table);
        enqueue_head(bg_pcb);
    }

    if (mt_enabled) {
        pthread_cond_broadcast(&work_cond);
        pthread_mutex_unlock(&queue_mutex);
    }

    if (!scheduler_active) {
        if (mt) {
            scheduler_run_mt(policy);
        } else {
            scheduler_run(policy);
        }
    }

    return 0;
}

int run(char *args[], int args_size) {
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }
    else if (pid == 0) {
        char *cmd_args[args_size];
        for (int i = 1; i < args_size; i++) cmd_args[i - 1] = args[i];
        cmd_args[args_size - 1] = NULL;
        execvp(cmd_args[0], cmd_args);
        perror("execvp");
        _exit(1);
    } else {
        int status;
        waitpid(pid, &status, 0);
    }
    return 0;
}
