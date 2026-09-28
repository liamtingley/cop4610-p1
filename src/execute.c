#define _POSIX_C_SOURCE 200809L
#include "pipeline.h"
#include "execute.h"
#include "jobs.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

char *find_executable(const char *command)
{
    const char *path = getenv("PATH");

    if (path == NULL) {
        return NULL;
    }

    const char *start = path;

    while (1) {
        const char *end = strchr(start, ':');
        size_t dir_length = end != NULL
            ? (size_t)(end - start)
            : strlen(start);
        size_t command_length = strlen(command);
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

        struct stat info;
        if (stat(candidate, &info) == 0 &&
            S_ISREG(info.st_mode) &&
            access(candidate, X_OK) == 0) {
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

static int is_redirect(const char *token)
{
    return strcmp(token, "<") == 0 || strcmp(token, ">") == 0;
}

/*
 * Make an argument list without the redirection tokens.
 * The strings still belong to the original tokenlist.
 */
static char **parse_redirections(
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

/* Called only by the child, before execv(). */
static int apply_redirections(
    const char *input_file,
    const char *output_file)
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

int run_external(char **tokens, int background, const char *original_line)
{
    const char *input_file;
    const char *output_file;
    for (size_t i = 0; tokens[i] != NULL; i++) {
    if (strcmp(tokens[i], "|") == 0) {
        return run_pipeline(tokens, background, original_line);
	    }
	}
    char **args = parse_redirections(tokens, &input_file, &output_file);

    if (args == NULL) {
        return 0;
    }

    char *found_path = NULL;
    const char *executable = args[0];

    if (strchr(args[0], '/') == NULL) {
        found_path = find_executable(args[0]);

        if (found_path == NULL) {
            fprintf(stderr, "%s: command not found\n", args[0]);
            free(args);
            return 0;
        }
        executable = found_path;
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        free(found_path);
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
    if (background &&
    	add_background_job(&pid, 1, original_line) == 0) {
    	free(found_path);
    	free(args);
    	return 1;
	}
    int status;
    while (waitpid(pid, &status, 0) == -1) {
        if (errno != EINTR) {
            perror("waitpid");
            break;
        }
    }

    free(found_path);
    free(args);
	return 1;
}
