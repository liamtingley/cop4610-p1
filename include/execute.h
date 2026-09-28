#ifndef EXECUTE_H
#define EXECUTE_H

int run_external(char **tokens, int background, const char *original_line);
char *find_executable(const char *command);

#endif
