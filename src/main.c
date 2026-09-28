#define _POSIX_C_SOURCE 200809L

#include "builtins.h"
#include "execute.h"
#include "expand.h"
#include "history.h"
#include "jobs.h"
#include "lexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void print_prompt(void)
{
    char hostname[256];
    char cwd[4096];
    const char *user = getenv("USER");

    if (gethostname(hostname, sizeof(hostname)) == -1) {
        perror("gethostname");
        return;
    }
    hostname[sizeof(hostname) - 1] = '\0';

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd");
        return;
    }

    printf("%s@%s:%s>",
           user != NULL ? user : "unknown",
           hostname,
           cwd);
    fflush(stdout);
}

int main(void)
{
    while (1) {
        check_background_jobs();
        print_prompt();

        char *input = get_input();

        /* The supplied get_input() returns an empty string at EOF. */
        if (feof(stdin) && input[0] == '\0') {
            putchar('\n');
            wait_for_background_jobs();
            free(input);
            break;
        }

        tokenlist *tokens = get_tokens(input);

        if (tokens->size > 0 && expand_tokens(tokens) == 0) {
            int background = 0;

            if (strcmp(tokens->items[tokens->size - 1], "&") == 0) {
                background = 1;
                free(tokens->items[tokens->size - 1]);
                tokens->items[--tokens->size] = NULL;
            }

            if (tokens->size == 0) {
                fprintf(stderr, "missing command before &\n");
            } else if (strcmp(tokens->items[0], "exit") == 0) {
                wait_for_background_jobs();
                print_history();

                free_tokens(tokens);
                free(input);
                break;
            } else {
                int result = handle_builtin(tokens);

                if (result == 0) {
                    result = run_external(
                        tokens->items, background, input
                    );
                }

                if (result > 0) {
                    add_history(input);
                }
            }
        }

        free_tokens(tokens);
        free(input);
    }

    free_history();
    return 0;
}
