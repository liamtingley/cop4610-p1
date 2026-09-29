#define _POSIX_C_SOURCE 200809L

#include "redirect.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static int is_redirect(const char *token)
{
    return strcmp(token, "<") == 0 || strcmp(token, ">") == 0;
}

char **parse_redirections(
    char **tokens,
    const char **input_file,
    const char **output_file)
{
    size_t count = 0;
    while (tokens[count] != NULL) {
        count++;
    }

    char **args = malloc((count + 1) * sizeof(char *));
    if (args == NULL) {
        perror("malloc");
        return NULL;
    }

    *input_file = NULL;
    *output_file = NULL;
    size_t arg_count = 0;

    for (size_t i = 0; i < count; i++) {
        if (is_redirect(tokens[i])) {
            if (i + 1 >= count || is_redirect(tokens[i + 1])) {
                fprintf(stderr, "redirection: missing filename\n");
                free(args);
                return NULL;
            }

            if (strcmp(tokens[i], "<") == 0) {
                *input_file = tokens[++i];
            } else {
                *output_file = tokens[++i];
            }
        } else {
            args[arg_count++] = tokens[i];
        }
    }

    args[arg_count] = NULL;

    if (arg_count == 0) {
        fprintf(stderr, "redirection: missing command\n");
        free(args);
        return NULL;
    }

    return args;
}

int apply_redirections(const char *input_file, const char *output_file)
{
    if (input_file != NULL) {
        int fd = open(input_file, O_RDONLY);
        if (fd == -1) {
            perror(input_file);
            return -1;
        }

        struct stat info;
        if (fstat(fd, &info) == -1 || !S_ISREG(info.st_mode)) {
            fprintf(stderr, "%s: not a regular input file\n", input_file);
            close(fd);
            return -1;
        }

        if (dup2(fd, STDIN_FILENO) == -1) {
            perror("dup2");
            close(fd);
            return -1;
        }
        close(fd);
    }

    if (output_file != NULL) {
        int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (fd == -1) {
            perror(output_file);
            return -1;
        }

        /* Also set the required permissions when overwriting a file. */
        if (fchmod(fd, 0600) == -1) {
            perror("fchmod");
            close(fd);
            return -1;
        }

        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("dup2");
            close(fd);
            return -1;
        }
        close(fd);
    }

    return 0;
}
