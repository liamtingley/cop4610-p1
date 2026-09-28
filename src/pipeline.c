#include "pipeline.h"
#include "execute.h"
#include "jobs.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int run_pipeline(char **tokens, int background, const char *original_line)
{
    size_t token_count = 0;
    while (tokens[token_count] != NULL) {
        token_count++;
    }

    /* Each command gets its own NULL-terminated argument array. */
    char **commands[3] = {NULL, NULL, NULL};
    int counts[3] = {0, 0, 0};
    int command_index = 0;
    char *found_paths[3] = {NULL, NULL, NULL};
    const char *executables[3] = {NULL, NULL, NULL};

    int accepted = 0;

    for (int i = 0; i < 3; i++) {
        commands[i] = malloc((token_count + 1) * sizeof(char *));
        if (commands[i] == NULL) {
            perror("malloc");
            goto cleanup;
        }
    }

    for (size_t i = 0; i < token_count; i++) {
        if (strcmp(tokens[i], "|") == 0) {
            if (counts[command_index] == 0) {
                fprintf(stderr, "pipe: missing command\n");
                goto cleanup;
            }
            if (command_index == 2) {
                fprintf(stderr, "pipe: more than two pipes\n");
                goto cleanup;
            }
            command_index++;
        } else if (strcmp(tokens[i], "<") == 0 ||
                   strcmp(tokens[i], ">") == 0) {
            fprintf(stderr, "pipe: redirection with pipes is unsupported\n");
            goto cleanup;
        } else {
            commands[command_index][counts[command_index]++] = tokens[i];
        }
    }

    if (counts[command_index] == 0) {
        fprintf(stderr, "pipe: missing command\n");
        goto cleanup;
    }

    int command_count = command_index + 1;

    for (int i = 0; i < command_count; i++) {
        commands[i][counts[i]] = NULL;

        executables[i] = commands[i][0];
        if (strchr(commands[i][0], '/') == NULL) {
            found_paths[i] = find_executable(commands[i][0]);

            if (found_paths[i] == NULL) {
                fprintf(stderr, "%s: command not found\n", commands[i][0]);
                goto cleanup;
            }
            executables[i] = found_paths[i];
        }
    }

    int previous_read = -1;
    pid_t children[3];
    int started = 0;

    for (int i = 0; i < command_count; i++) {
        int next_pipe[2] = {-1, -1};

        if (i < command_count - 1 && pipe(next_pipe) == -1) {
            perror("pipe");
            break;
        }

        pid_t pid = fork();

        if (pid == -1) {
            perror("fork");
            if (next_pipe[0] != -1) close(next_pipe[0]);
            if (next_pipe[1] != -1) close(next_pipe[1]);
            break;
        }

        if (pid == 0) {
            if (previous_read != -1 &&
                dup2(previous_read, STDIN_FILENO) == -1) {
                perror("dup2");
                _exit(1);
            }

            if (next_pipe[1] != -1 &&
                dup2(next_pipe[1], STDOUT_FILENO) == -1) {
                perror("dup2");
                _exit(1);
            }

            if (previous_read != -1) close(previous_read);
            if (next_pipe[0] != -1) close(next_pipe[0]);
            if (next_pipe[1] != -1) close(next_pipe[1]);

            execv(executables[i], commands[i]);
            perror(executables[i]);
            _exit(127);
        }

        children[started++] = pid;

        if (previous_read != -1) close(previous_read);
        if (next_pipe[1] != -1) close(next_pipe[1]);
        previous_read = next_pipe[0];
    }

    if (previous_read != -1) {
        close(previous_read);
    }
    
    accepted = (started == command_count);

    if (background && started == command_count &&
    add_background_job(children, started, original_line) == 0) {
    goto cleanup;
	}

    for (int i = 0; i < started; i++) {
        int status;
        while (waitpid(children[i], &status, 0) == -1) {
            if (errno != EINTR) {
                perror("waitpid");
                break;
            }
        }
    }

cleanup:
    for (int i = 0; i < 3; i++) {
        free(found_paths[i]);
        free(commands[i]);
    }
	return accepted;
}
