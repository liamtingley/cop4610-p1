#define _POSIX_C_SOURCE 200809L

#include "pipeline.h"
#include "execute.h"
#include "jobs.h"
#include "redirect.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/* One stage of a pipeline, such as "grep foo > out.txt". */
typedef struct {
    char **args;             /* argument list; strings owned by the tokenlist */
    const char *input_file;  /* file after "<", or NULL */
    const char *output_file; /* file after ">", or NULL */
    char *executable;        /* full path returned by find_executable() */
} Command;

static void free_commands(Command *commands, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        free(commands[i].args);
        free(commands[i].executable);
    }
    free(commands);
}

/*
 * Split the tokens at every "|" into any number of commands. Each command may
 * have its own "<" and ">" redirections. Returns NULL on a syntax error or if
 * a command cannot be found.
 */
static Command *parse_pipeline(char **tokens, size_t *command_count)
{
    size_t token_count = 0;
    size_t pipe_count = 0;

    while (tokens[token_count] != NULL) {
        if (strcmp(tokens[token_count], "|") == 0) {
            pipe_count++;
        }
        token_count++;
    }

    /* A copy of the token pointers where each "|" is replaced by NULL. */
    char **work = malloc((token_count + 1) * sizeof(char *));
    Command *commands = calloc(pipe_count + 1, sizeof(Command));
    size_t count = 0;
    size_t start = 0;

    if (work == NULL || commands == NULL) {
        perror("malloc");
        free(work);
        free(commands);
        return NULL;
    }

    for (size_t i = 0; i <= token_count; i++) {
        if (i < token_count && strcmp(tokens[i], "|") != 0) {
            work[i] = tokens[i];
            continue;
        }

        work[i] = NULL;

        if (i == start) {
            fprintf(stderr, "pipe: missing command\n");
            goto fail;
        }

        Command *command = &commands[count++];
        command->args = parse_redirections(
            work + start, &command->input_file, &command->output_file
        );
        if (command->args == NULL) {
            goto fail;
        }

        command->executable = find_executable(command->args[0]);
        if (command->executable == NULL) {
            fprintf(stderr, "%s: command not found\n", command->args[0]);
            goto fail;
        }

        start = i + 1;
    }

    free(work);
    *command_count = count;
    return commands;

fail:
    free(work);
    free_commands(commands, count);
    return NULL;
}

int run_pipeline(char **tokens, int background, const char *original_line)
{
    size_t count;
    Command *commands = parse_pipeline(tokens, &count);

    if (commands == NULL) {
        return 0;
    }

    pid_t *children = malloc(count * sizeof(pid_t));
    if (children == NULL) {
        perror("malloc");
        free_commands(commands, count);
        return 0;
    }

    size_t started = 0;
    int previous_read = -1;

    for (size_t i = 0; i < count; i++) {
        int next_pipe[2] = {-1, -1};

        if (i < count - 1 && pipe(next_pipe) == -1) {
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

            /* Applied after the pipe so an explicit "<" or ">" wins. */
            if (apply_redirections(commands[i].input_file,
                                   commands[i].output_file) == -1) {
                _exit(1);
            }

            execv(commands[i].executable, commands[i].args);
            perror(commands[i].executable);
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

    int accepted = (started == count);

    if (!accepted || !background ||
        add_background_job(children, (int)started, original_line) != 0) {
        for (size_t i = 0; i < started; i++) {
            wait_for_child(children[i]);
        }
    }

    free(children);
    free_commands(commands, count);
    return accepted;
}
