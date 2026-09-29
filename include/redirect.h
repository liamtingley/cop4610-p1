#ifndef REDIRECT_H
#define REDIRECT_H

/*
 * Build an argument list from a NULL-terminated token list with the "<" and ">"
 * tokens (and their filenames) removed. The returned array must be freed by the
 * caller, but the strings still belong to the original token list.
 * Returns NULL (after printing an error) if the redirections are malformed.
 */
char **parse_redirections(
    char **tokens,
    const char **input_file,
    const char **output_file);

/* Called only by a child process, before execv(). Returns 0 or -1 on error. */
int apply_redirections(const char *input_file, const char *output_file);

#endif
