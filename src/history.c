#include "history.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *recent[3];
static int count = 0;

void add_history(const char *line)
{
    char *copy = malloc(strlen(line) + 1);
    if (copy == NULL) {
        perror("malloc");
        return;
    }
    strcpy(copy, line);

    if (count == 3) {
        free(recent[0]);
        recent[0] = recent[1];
        recent[1] = recent[2];
        count = 2;
    }

    recent[count++] = copy;
}

void print_history(void)
{
    if (count == 0) {
        puts("No valid commands.");
    } else if (count < 3) {
        puts("Last valid command:");
        printf("[1]: %s\n", recent[count - 1]);
    } else {
        puts("Last (3) valid commands:");
        for (int i = 0; i < 3; i++) {
            printf("[%d]: %s\n", i + 1, recent[i]);
        }
    }
}

void free_history(void)
{
    for (int i = 0; i < count; i++) {
        free(recent[i]);
    }
    count = 0;
}
