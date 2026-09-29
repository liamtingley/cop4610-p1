#define _POSIX_C_SOURCE 200809L

#include "execute.h"
#include "jobs.h"
#include "pipeline.h"
#include "redirect.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static int is_executable_file(const char *path)
{
    struct stat info;

    return stat(path, &info) == 0 &&
           S_ISREG(info.st_mode) &&
           access(path, X_OK) == 0;
}

char *find_executable(const char *command)
{
    /* A command such as ./bin/shell is used as given, not searched for. */
    if (strchr(command, '/') != NULL) {
        if (!is_executable_file(command)) {
            return NULL;
        }

        char *copy = malloc(strlen(command) + 1);
        if (copy == NULL) {
            perror("malloc");
            return NULL;
        }
        strcpy(copy, command);
        return copy;
    }

    const char *path = getenv("PATH");

    if (path == NULL) {
        return NULL;
    }

    const char *start = path;
    size_t command_length = strlen(command);

    while (1) {
        const char *end = strchr(start, ':');
        size_t dir_length = end != NULL
            ? (size_t)(end - start)
            : strlen(start);
        size_t length = dir_length + (dir_length > 0 ? 1 : 0)
            + command_length + 1;

        char *candidate = malloc(length);
        if (candidate == NULL) {
            perror("malloc");
            return NULL;
        }

        memcpy(candidate, start, dir_length);
        size_t position = dir_length;

        if (dir_length > 0) {
            candidate[position++] = '/';
        }
        memcpy(candidate + position, command, command_length + 1);

        if (is_executable_file(candidate)) {
            return candidate;
        }

        free(candidate);

        if (end == NULL) {
            break;
        }
        start = end + 1;
    }

    return NULL;
}

void wait_for_child(pid_t pid)
{
    int status;

    while (waitpid(pid, &status, 0) == -1) {
        if (errno != EINTR) {
            perror("waitpid");
            break;
        }
    }
}

int run_external(char **tokens, int background, const char *original_line)
{
    for (size_t i = 0; tokens[i] != NULL; i++) {
        if (strcmp(tokens[i], "|") == 0) {
            return run_pipeline(tokens, background, original_line);
        }
    }

    const char *input_file;
    const char *output_file;
    char **args = parse_redirections(tokens, &input_file, &output_file);

    if (args == NULL) {
        return 0;
    }

    char *executable = find_executable(args[0]);

    if (executable == NULL) {
        fprintf(stderr, "%s: command not found\n", args[0]);
        free(args);
        return 0;
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        free(executable);
        free(args);
        return 0;
    }

    if (pid == 0) {
        if (apply_redirections(input_file, output_file) == -1) {
            _exit(1);
        }

        execv(executable, args);
        perror(executable);
        _exit(127);
    }

    /* Fall back to running in the foreground if the job table is full. */
    if (!background || add_background_job(&pid, 1, original_line) != 0) {
        wait_for_child(pid);
    }

    free(executable);
    free(args);
    return 1;
}
