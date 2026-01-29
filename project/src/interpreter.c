#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shellmemory.h"
#include "shell.h"
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>

int MAX_ARGS_SIZE = 3;

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
    exit(0);
}

int set(char *var, char *value) {
    // Challenge: allow setting VAR to the rest of the input line,
    // possibly including spaces.

    // Hint: Since "value" might contain multiple tokens, you'll need to loop
    // through them, concatenate each token to the buffer, and handle spacing
    // appropriately. Investigate how `strcat` works and how you can use it
    // effectively here.

    mem_set_value(var, value);
    return 0;
}


int print(char *var) {
    printf("%s\n", mem_get_value(var));
    return 0;
}

int source(char *script) {
    int errCode = 0;
    char line[MAX_USER_INPUT];
    FILE *p = fopen(script, "rt");      // the program is in a file

    if (p == NULL) {
        return badcommandFileDoesNotExist();
    }

    fgets(line, MAX_USER_INPUT - 1, p);
    while (1) {
        errCode = parseInput(line);     // which calls interpreter()
        memset(line, 0, sizeof(line));

        if (feof(p)) {
            break;
        }
        fgets(line, MAX_USER_INPUT - 1, p);
    }

    fclose(p);

    return errCode;
}

int echo(char *var) {
    if (var[0] == '$') {
        char *var_name = var + 1;  // Skip $
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

    if (dp == NULL) {
        perror("opendir");
        return 1;
    }

    char *names[1024];
    int count = 0;

    while ((entry = readdir(dp)) != NULL) {
        names[count++] = strdup(entry->d_name);
    }

    closedir(dp);

    qsort(names, count, sizeof(char *), cmpstringp);

    for (int i = 0; i < count; i++) {
        printf("%s\n", names[i]);
        free(names[i]);
    }

    return 0;
}

int is_alphanumeric_string(char *s) {
    if (s == NULL || strlen(s) == 0) return 0;

    for (int i = 0; s[i]; i++) {
        if (!isalnum(s[i])) return 0;
    }
    return 1;
}

int my_mkdir(char *dirname) {
    char final_name[256];

    if (dirname[0] == '$') {
        // Variable case
        char *varname = dirname + 1;
        char *value = mem_get_value(varname);

        if (strcmp(value, "Variable does not exist") == 0 ||
            !is_alphanumeric_string(value)) {
            printf("Bad command: my_mkdir\n");
            return 1;
        }

        strcpy(final_name, value);
    } 
    else {
        // Normal name case
        if (!is_alphanumeric_string(dirname)) {
            printf("Bad command: my_mkdir\n");
            return 1;
        }

        strcpy(final_name, dirname);
    }

    if (mkdir(final_name, 0777) != 0) {
        printf("Bad command: my_mkdir\n");
        return 1;
    }

    return 0;
}

int my_touch(char *filename) {
    if (!is_alphanumeric_string(filename)) {
        printf("Bad command: my_touch\n");
        return 1;
    }

    int fd = open(filename, O_CREAT | O_WRONLY, 0666);
    if (fd < 0) {
        printf("Bad command: my_touch\n");
        return 1;
    }

    close(fd);
    return 0;
}

int my_cd(char *dirname) {
    if (!is_alphanumeric_string(dirname)) {
        printf("Bad command: my_cd\n");
        return 1;
    }

    struct stat sb;

    if (stat(dirname, &sb) != 0 || !S_ISDIR(sb.st_mode)) {
        printf("Bad command: my_cd\n");
        return 1;
    }

    if (chdir(dirname) != 0) {
        printf("Bad command: my_cd\n");
        return 1;
    }

    return 0;
}
