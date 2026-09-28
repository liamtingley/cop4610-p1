#ifndef JOBS_H
#define JOBS_H

#include <sys/types.h>

int add_background_job(const pid_t *pids, int count, const char *line);
void check_background_jobs(void);
void print_jobs(void);
void wait_for_background_jobs(void);

#endif
