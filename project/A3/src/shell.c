#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "shell.h"
#include "interpreter.h"
#include "shellmemory.h"
#include "scheduler.h"

int parseInput(char ui[]);

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, 0); // disable buffering edge case

    printf("Shell version 1.5 created Dec 2025\n");

    char prompt = '$';  				// Shell prompt
    char userInput[MAX_USER_INPUT];		// user's input stored here
    int errorCode = 0;					// zero means no error, default

    int interactive = isatty(fileno(stdin));

    //init user input
    for (int i = 0; i < MAX_USER_INPUT; i++) {
        userInput[i] = '\0';
    }

    //init shell memory
    mem_init();
    while(1) {
        if (interactive) {
            printf("%c ", prompt);
        }
        // here you should check the unistd library
        // so that you can find a way to not display $ in the batch mode
        if (fgets(userInput, MAX_USER_INPUT - 1, stdin) == NULL) {
            // EOF reached - join MT workers if active
            mt_join_workers();
            exit(0);
        }

        errorCode = parseInput(userInput);
        if (errorCode == -1) exit(99);	// ignore all other errors
        memset(userInput, 0, sizeof(userInput));
    }

    return 0;
}

int wordEnding(char c) {
    return c == '\0' || c == '\n' || c == ' ';
}

int parseInput(char inp[]) {
    char *commands[10];    // max 10 chained commands
    int num_commands = 0;  // command pointer

    // Split tokens by semicolon and whitespace
    char *saveptr;
    char *token = strtok_r(inp, ";", &saveptr);
    while (token != NULL && num_commands < 10) {
        while (*token == ' ') token++;
        commands[num_commands++] = token;
        token = strtok_r(NULL, ";", &saveptr);
    }

    int errorCode = 0;

    for (int i = 0; i < num_commands; i++) {
        // Now split command into words
        char tmp[200], *words[100];
        int ix = 0, w = 0;
        int wordlen;

        char *cmd = commands[i];

        for (ix = 0; cmd[ix] == ' ' && cmd[ix] != '\0'; ix++); // skip white spaces
        while (cmd[ix] != '\0') {
            // extract a word
            for (wordlen = 0; !wordEnding(cmd[ix]) && cmd[ix] != '\0'; ix++, wordlen++) {
                tmp[wordlen] = cmd[ix];
            }
            tmp[wordlen] = '\0';
            words[w] = strdup(tmp);
            w++;
            if (cmd[ix] == '\0') break;
            ix++;
        }
        errorCode = interpreter(words, w);
        for (int k = 0; k < w; k++) free(words[k]);
        if (errorCode == -1) return -1;
    }

    return errorCode;
}
