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
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

int MAX_ARGS_SIZE = 7;

int badcommand() {
    printf("Unknown Command\n");
    return 1;
}

// For source command only
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
int badcommandFileDoesNotExist();

// Interpret commands and their arguments
int interpreter(char *command_args[], int args_size) {
    int i;

    if (args_size < 1 || args_size > MAX_ARGS_SIZE) {
        return badcommand();
    }

    for (i = 0; i < args_size; i++) {   // terminate args at newlines
        command_args[i][strcspn(command_args[i], "\r\n")] = 0;
    }

    if (strcmp(command_args[0], "help") == 0) {
        //help
        if (args_size != 1)
            return badcommand();
        return help();

    } else if (strcmp(command_args[0], "quit") == 0) {
        //quit
        if (args_size != 1)
            return badcommand();
        return quit();

    } else if (strcmp(command_args[0], "set") == 0) {
        //set
        if (args_size != 3)
            return badcommand();
        return set(command_args[1], command_args[2]);

    } else if (strcmp(command_args[0], "print") == 0) {
        if (args_size != 2)
            return badcommand();
        return print(command_args[1]);

    } else if (strcmp(command_args[0], "source") == 0) {
        if (args_size != 2)
            return badcommand();
        return source(command_args[1]);

    } else if (strcmp(command_args[0], "echo") == 0) {
        if (args_size != 2)
            return badcommand();
        return echo(command_args[1]);

    } else if (strcmp(command_args[0], "my_ls") == 0) {
        if (args_size != 1)
            return badcommand();
        return my_ls();

    } else if (strcmp(command_args[0], "my_mkdir") == 0) {
        if (args_size != 2)
          return badcommand();
        return my_mkdir(command_args[1]);

    } else if (strcmp(command_args[0], "my_touch") == 0) {
        if (args_size != 2)
            return badcommand();
        return my_touch(command_args[1]);

    } else if (strcmp(command_args[0], "my_cd") == 0) {
        if (args_size != 2)
            return badcommand();
        return my_cd(command_args[1]);

    } else if (strcmp(command_args[0], "run") == 0) {
        if (args_size < 2)
            return badcommand();
        return run(command_args, args_size);

    } else if (strcmp(command_args[0], "exec") == 0) {
        if (args_size < 3)
            return badcommand();
        return my_exec(command_args, args_size);

    } else
        return badcommand();
}

int help() {

    // note the literal tab characters here for alignment
    char help_string[] = "COMMAND			DESCRIPTION\n \
help			Displays all the commands\n \
quit			Exits / terminates the shell with “Bye!”\n \
set VAR STRING		Assigns a value to shell memory\n \
print VAR		Displays the STRING assigned to VAR\n \
source SCRIPT.TXT	Executes the file SCRIPT.TXT\n ";
    printf("%s\n", help_string);
    return 0;
}

int quit() {
    printf("Bye!\n");
    if (mt_enabled) {
        // Check if called from a worker thread
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

int source(char *script) {

    FILE *fp = fopen(script, "rt");
    if (!fp) return badcommandFileDoesNotExist();

    // Load program
    int start = program_load(fp);
    fclose(fp);

    if (start < 0) {
        printf("Error: program memory full\n");
        return 1;
    }

    // Create Procss Control Block
    int length = 0;
    for (int i = start; i < MAX_PROGRAM_LINES && program_used[i]; i++) {
        length++;
    }

    PCB *pcb = pcb_create(start, length);
    enqueue(pcb);
    scheduler_run("FCFS");

    return 0;
}

int echo(char *var) {
    if (var[0] == '$') {
        // Case 1: Skip $ and look for variable
        char *var_name = var + 1;
        char *value = mem_get_value(var_name);

        if (strcmp(value, "Variable does not exist") == 0) {
            printf("\n");
        } else {
            printf("%s\n", value);
        }
    } else {
        // Case 2: Print directly
        printf("%s\n", var);
    }
    return 0;
}

int cmpstringp(const void *p1, const void *p2) {
    /*
     *   Wrapper for qsort to compare two *char array elements
     */
    char *const *s1 = p1;
    char *const *s2 = p2;
    return strcmp(*s1, *s2);
}

int my_ls() {
    // Open current directory
    struct dirent *entry;
    DIR *dp = opendir(".");

    if (dp == NULL) {
        perror("opendir");
        return 1;
    }

    char *names[1024];
    int count = 0;

    // Read all directories
    while ((entry = readdir(dp)) != NULL) {
        names[count++] = strdup(entry->d_name);
    }

    closedir(dp);

    // Sort and print directories
    qsort(names, count, sizeof(char *), cmpstringp);

    for (int i = 0; i < count; i++) {
        printf("%s\n", names[i]);
        free(names[i]);
    }

    return 0;
}

int is_alphanumeric_string(char *s) {
    /*
     * Helper function that verifies string is alphanumeric
     */
    if (s == NULL || strlen(s) == 0) return 0;

    for (int i = 0; s[i]; i++) {
        if (!isalnum(s[i])) return 0;
    }
    return 1;
}

int my_mkdir(char *dirname) {
    char final_name[256];

    if (dirname[0] == '$') {
        // Case 1: Variable
        char *varname = dirname + 1; // Skip $
        char *value = mem_get_value(varname);

        // Check validity
        if (strcmp(value, "Variable does not exist") == 0 ||
            !is_alphanumeric_string(value)) {
            printf("Bad command: my_mkdir\n");
            return 1;
        }

        strcpy(final_name, value);
    }
    else {
        // Case 2: Literal value
        if (!is_alphanumeric_string(dirname)) {
            printf("Bad command: my_mkdir\n");
            return 1;
        }

        strcpy(final_name, dirname);
    }

    // Make directory
    if (mkdir(final_name, 0777) != 0) {
        printf("Bad command: my_mkdir\n");
        return 1;
    }

    return 0;
}

int my_touch(char *filename) {
    // Check validity
    if (!is_alphanumeric_string(filename)) {
        printf("Bad command: my_touch\n");
        return 1;
    }

    // Create file
    int fd = open(filename, O_CREAT | O_WRONLY, 0666);
    if (fd < 0) {
        printf("Bad command: my_touch\n");
        return 1;
    }

    close(fd);
    return 0;
}

int my_cd(char *dirname) {
    // Verify validity
    if (!is_alphanumeric_string(dirname)) {
        printf("Bad command: my_cd\n");
        return 1;
    }

    struct stat sb;

    // Verify dir exists
    if (stat(dirname, &sb) != 0 || !S_ISDIR(sb.st_mode)) {
        printf("Bad command: my_cd\n");
        return 1;
    }

    // Change directory
    if (chdir(dirname) != 0) {
        printf("Bad command: my_cd\n");
        return 1;
    }

    return 0;
}

// -------------- 1.2.2 - Exec Command --------------------
int my_exec(char *args[], int args_size) {
    // usage: exec file1 [file2] [file3] POLICY [#] [MT]
    int background = 0;
    int mt = 0;
    char *policy;
    int num_files;

    // Detect MT flag (always last if present)
    int last = args_size - 1;
    if (strcmp(args[last], "MT") == 0) {
        mt = 1;
        last--;
    }

    // Detect # background flag
    if (strcmp(args[last], "#") == 0) {
        background = 1;
        last--;
    }

    policy = args[last];
    num_files = last - 1;

    // Validate policy
    if (strcmp(policy, "FCFS") != 0 && strcmp(policy, "SJF") != 0 &&
        strcmp(policy, "RR") != 0 && strcmp(policy, "RR30") != 0 &&
        strcmp(policy, "AGING") != 0) {
        return badcommand();
    }

    // validate files and check if duplicates exist
    if (num_files < 1 || num_files > 3) {
        return badcommand();
    }
    for (int i = 1; i <= num_files; i++) {
        for (int j = i + 1; j <= num_files; j++) {
            if (strcmp(args[i], args[j]) == 0) {
                printf("Error: Duplicate file name\n");
                return 1;
            }
        }
    }

    // Loading
    int starts[3], lengths[3];

    for (int i = 0; i < num_files; i++) {
        FILE *fp = fopen(args[i + 1], "rt");
        if (!fp) {
            for (int j = 0; j < i; j++) {
                program_free(starts[j], lengths[j]);
            }
            return badcommandFileDoesNotExist();
        }

        int start = program_load(fp);
        fclose(fp);

        if (start < 0) {
            for (int j = 0; j < i; j++) {
                program_free(starts[j], lengths[j]);
            }
            printf("Error: program memory full\n");
            return 1;
        }

        int length = 0;
        for (int k = start; k < MAX_PROGRAM_LINES && program_used[k]; k++) {
            length++;
        }

        starts[i] = start;
        lengths[i] = length;
    }

    // ------------------- 1.2.5 - Background Mode ---------------------
    // Load remaining stdin as batch script
    int bg_start = -1, bg_length = 0;
    if (background) {
        bg_start = program_load(stdin);
        if (bg_start >= 0) {
            for (int k = bg_start; k < MAX_PROGRAM_LINES && program_used[k]; k++) {
                bg_length++;
            }
        }
    }
    // Enqueue exec'd programs (lock mutex if MT workers are running)
    if (mt_enabled) pthread_mutex_lock(&queue_mutex);

    // create a pcb for each processes and enques them using the correctly mapped function
    for (int i = 0; i < num_files; i++) {
        PCB *pcb = pcb_create(starts[i], lengths[i]);
        if (strcmp(policy, "SJF") == 0) {
            enqueue_sjf(pcb);
        } else if (strcmp(policy, "AGING") == 0) {
            enqueue_aging(pcb);
        } else {
            enqueue(pcb);
        }
    }

    // Background mode: insert batch script at head of queue (runs first)
    if (background && bg_length > 0) {
        PCB *bg_pcb = pcb_create(bg_start, bg_length);
        enqueue_head(bg_pcb);
    }

    if (mt_enabled) {
        pthread_cond_broadcast(&work_cond);
        pthread_mutex_unlock(&queue_mutex);
    }

    // Run phase: skip if scheduler is already active
    if (!scheduler_active) { // meaning not inside a nested exec
        if (mt) {
            scheduler_run_mt(policy);
        } else {
            scheduler_run(policy);
        }
    }

    return 0;
}

int run(char *args[], int args_size) {
    // Fork new process
    pid_t pid = fork();

    // Case 1: Error
    if (pid < 0) {
        perror("fork");
        return 1;
    }
    else if (pid == 0) {
        // Case 2: Child process
        char *cmd_args[args_size];
        for (int i = 1; i < args_size; i++) {
            cmd_args[i - 1] = args[i];
        }
        cmd_args[args_size - 1] = NULL;

        execvp(cmd_args[0], cmd_args);

        perror("execvp");
        _exit(1);
    }
    else {
        // Case 3: Parent process
        int status;
        waitpid(pid, &status, 0);
    }

    return 0;
}
