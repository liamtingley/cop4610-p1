#include "expand.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int is_name_char(char c)
{
    return isalnum((unsigned char)c) || c == '_';
}

/*
 * Expand a token such as "$HOME" or "$HOME/dir". The variable name is the
 * run of letters, digits and underscores after the '$'; anything after it is
 * kept as-is. Returns a new string, or NULL if there is no name after '$'.
 */
static char *expand_variable(const char *token, int *error)
{
    const char *name = token + 1;
    size_t name_length = 0;

    while (is_name_char(name[name_length])) {
        name_length++;
    }

    if (name_length == 0) {
        return NULL;
    }

    char *name_copy = malloc(name_length + 1);
    if (name_copy == NULL) {
        perror("malloc");
        *error = 1;
        return NULL;
    }
    memcpy(name_copy, name, name_length);
    name_copy[name_length] = '\0';

    const char *value = getenv(name_copy);
    free(name_copy);

    /* Treat an unset variable as empty. */
    if (value == NULL) {
        value = "";
    }

    const char *rest = name + name_length;
    char *replacement = malloc(strlen(value) + strlen(rest) + 1);
    if (replacement == NULL) {
        perror("malloc");
        *error = 1;
        return NULL;
    }

    strcpy(replacement, value);
    strcat(replacement, rest);
    return replacement;
}

int expand_tokens(tokenlist *tokens)
{
    for (size_t i = 0; i < tokens->size; i++) {
        char *original = tokens->items[i];
        char *replacement = NULL;
        int error = 0;

        if (original[0] == '$') {
            replacement = expand_variable(original, &error);
            if (error) {
                return -1;
            }
        }
        else if (original[0] == '~' &&
                 (original[1] == '\0' || original[1] == '/')) {
            const char *home = getenv("HOME");

            if (home == NULL) {
                fprintf(stderr, "HOME is not set\n");
                return -1;
            }

            /* Replace the leading ~ with HOME. */
            replacement = malloc(strlen(home) + strlen(original + 1) + 1);
            if (replacement == NULL) {
                perror("malloc");
                return -1;
            }

            strcpy(replacement, home);
            strcat(replacement, original + 1);
        }

        if (replacement != NULL) {
            free(original);
            tokens->items[i] = replacement;
        }
    }

    return 0;
}
