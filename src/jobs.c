#include "jobs.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#define MAX_JOBS 10

typedef struct {
    int active;
    int number;
    pid_t *pids;    /* one PID per command in the pipeline */
    int *finished;  /* finished[i] is 1 once pids[i] has been reaped */
    int count;
    char *command;
} Job;

static Job jobs[MAX_JOBS];
static int next_job_number = 1;

static void finish_job(Job *job)
{
    printf("[%d] + done %s\n", job->number, job->command);
    free(job->command);
    free(job->pids);
    free(job->finished);
    job->command = NULL;
    job->pids = NULL;
    job->finished = NULL;
    job->active = 0;
}

int add_background_job(const pid_t *pids, int count, const char *line)
{
    int slot = -1;

    for (int i = 0; i < MAX_JOBS; i++) {
        if (!jobs[i].active) {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        fprintf(stderr, "jobs: too many background jobs\n");
        return -1;
    }

    char *copy = malloc(strlen(line) + 1);
    pid_t *pid_copy = malloc(count * sizeof(pid_t));
    int *finished = malloc(count * sizeof(int));

    if (copy == NULL || pid_copy == NULL || finished == NULL) {
        perror("malloc");
        free(copy);
        free(pid_copy);
        free(finished);
        return -1;
    }
    strcpy(copy, line);

    /* Store the command without its trailing &. */
    size_t end = strlen(copy);
    while (end > 0 && isspace((unsigned char)copy[end - 1])) end--;
    if (end > 0 && copy[end - 1] == '&') end--;
    while (end > 0 && isspace((unsigned char)copy[end - 1])) end--;
    copy[end] = '\0';

    Job *job = &jobs[slot];
    job->active = 1;
    job->number = next_job_number++;
    job->count = count;
    job->command = copy;
    job->pids = pid_copy;
    job->finished = finished;

    for (int i = 0; i < count; i++) {
        job->pids[i] = pids[i];
        job->finished[i] = 0;
    }

    printf("[%d] %ld\n", job->number, (long)pids[count - 1]);
    fflush(stdout);
    return 0;
}

void check_background_jobs(void)
{
    for (int i = 0; i < MAX_JOBS; i++) {
        Job *job = &jobs[i];
        if (!job->active) continue;

        int all_finished = 1;

        for (int j = 0; j < job->count; j++) {
            if (!job->finished[j]) {
                int status;
                pid_t result = waitpid(job->pids[j], &status, WNOHANG);

                if (result == job->pids[j] ||
                    (result == -1 && errno == ECHILD)) {
                    job->finished[j] = 1;
                } else if (result == -1) {
                    perror("waitpid");
                }
            }

            if (!job->finished[j]) all_finished = 0;
        }

        if (all_finished) finish_job(job);
    }
}

void print_jobs(void)
{
    int found = 0;

    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].active) {
            printf("[%d]+ %ld %s\n",
                   jobs[i].number,
                   (long)jobs[i].pids[jobs[i].count - 1],
                   jobs[i].command);
            found = 1;
        }
    }

    if (!found) puts("No active jobs.");
}

void wait_for_background_jobs(void)
{
    for (int i = 0; i < MAX_JOBS; i++) {
        Job *job = &jobs[i];
        if (!job->active) continue;

        for (int j = 0; j < job->count; j++) {
            if (job->finished[j]) continue;

            int status;
            while (waitpid(job->pids[j], &status, 0) == -1) {
                if (errno == EINTR) continue;
                if (errno != ECHILD) perror("waitpid");
                break;
            }
            job->finished[j] = 1;
        }

        finish_job(job);
    }
}
