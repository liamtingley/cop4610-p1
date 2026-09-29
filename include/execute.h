#ifndef EXECUTE_H
#define EXECUTE_H

#include <sys/types.h>

/* Run a command (or pipeline) that is not a built-in. Returns 1 if it ran. */
int run_external(char **tokens, int background, const char *original_line);

/*
 * Return a malloc'd path to the executable for a command, searching $PATH
 * unless the command already contains a '/'. Returns NULL if not found.
 */
char *find_executable(const char *command);

/* Block until the given child process has exited. */
void wait_for_child(pid_t pid);

#endif
