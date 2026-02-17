#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <pthread.h>

extern int scheduler_active;
extern int mt_enabled;
extern pthread_t mt_workers[2];
extern pthread_mutex_t queue_mutex;
extern pthread_cond_t work_cond;

void scheduler_run(char *policy);
void scheduler_run_mt(char *policy);
void mt_join_workers();

#endif
