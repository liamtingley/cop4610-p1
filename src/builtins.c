#define _POSIX_C_SOURCE 200809L

#include "builtins.h"
#include "jobs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int handle_builtin(tokenlist *tokens)
{
    if (strcmp(tokens->items[0], "jobs") == 0) {
        check_background_jobs();
        print_jobs();
        return 1;
    }

    if (strcmp(tokens->items[0], "cd") != 0) {
        return 0;
    }

    if (tokens->size > 2) {
        fprintf(stderr, "cd: too many arguments\n");
        return -1;
    }

    const char *destination;

    if (tokens->size == 1) {
        destination = getenv("HOME");
        if (destination == NULL) {
            fprintf(stderr, "cd: HOME is not set\n");
            return -1;
        }
    } else {
        destination = tokens->items[1];
    }

    if (chdir(destination) == -1) {
        perror("cd");
        return -1;
    }

    char cwd[4096];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        if (setenv("PWD", cwd, 1) == -1) {
            perror("setenv");
        }
    } else {
        perror("getcwd");
    }

    return 1;
}
