#include "expand.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int expand_tokens(tokenlist *tokens)
{
    for (size_t i = 0; i < tokens->size; i++) {
        char *original = tokens->items[i];
        char *replacement = NULL;

        if (original[0] == '$') {
            const char *value = getenv(original + 1);

            /* Treat an unset variable as an empty argument. */
            if (value == NULL) {
                value = "";
            }

            replacement = malloc(strlen(value) + 1);
            if (replacement == NULL) {
                perror("malloc");
                return -1;
            }
            strcpy(replacement, value);
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
