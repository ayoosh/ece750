#ifndef PROJ3_H
#define PROJ3_H

#include <stdio.h>

/************************************************************************/
/* 				Data structures				*/
/************************************************************************/

/* All the information needed for a task */
typedef struct task {
	/* task level info */
	char  *name;		/* task name */
	double period;		/* task period */
	double exec;		/* worst case execution time */
	double deadline;	/* task relative deadline */
	/* job level info */
	double next_evt;	/* next absolute release or deadline */
	double last_rel;	/* most recent release time */
	double usage;		/* time current job in task has run */
	double cur_exec;	/* actual exec time */
	double util;		/* current utilization of this task */
	int    job;		/* what job number is this in the task */
	struct task *next;	/* next in queue */
} task_t;

/* frequency/voltage combinations */
typedef struct fv {
	double freq;		/* percent of max frequency */
	double voltage;		/* percent of max voltage */
} fv_t;

/************************************************************************/
/* 		        Global variables from io.c			*/
/************************************************************************/

extern const fv_t freqs[];
extern const int num_freqs;

/************************************************************************/
/* 		        Functions from io.c				*/
/************************************************************************/

int parse_input(FILE *in, task_t **tasks, int *n_tasks);
double compute_hyperperiod(task_t *tasks, int n_tasks);
int run_job(task_t *tsk, int job_num, double start, double end, double freq);


/************************************************************************/
/* 		        Functions from proj3.c				*/
/************************************************************************/

int schedule_tasks(task_t *tasks, int n_tasks);

#endif

