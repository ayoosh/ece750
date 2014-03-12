/*
 * io.c
 *
 * Routines to read in input tasks and write schedules to stdout.
 */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "proj3.h"

/************************************************************************/
/* 				Macro Definitions			*/
/************************************************************************/

#define TASK_INCR 5		/* amount to increment tasks array  */
#define TASK_NAME_SIZE 32	/* first guess at task name length */

/************************************************************************/
/* 				Global Variables			*/
/************************************************************************/

const fv_t freqs[] = {
	{1.0,  1.0},
	{0.75, 0.8},
	{0.50, 0.6},
	{0.25, 0.4}
};	
const int  num_freqs = (sizeof(freqs) / sizeof(fv_t));

task_t *g_tasks;

/************************************************************************/
/* 				Function Prototypes			*/
/************************************************************************/

static int read_name(FILE *in, char **name);

/************************************************************************/
/* 				Function Definitions			*/
/************************************************************************/

int main(int argc, char *argv[])
{
	int    n_tasks;


	parse_input(stdin, &g_tasks, &n_tasks);
	schedule_tasks(g_tasks, n_tasks);

	return 0;
}

/*
 * parse_input
 *
 * Read input from in and turn it into a task list.  Not the most
 * efficient implementation, but good enough.  This version will
 * handle any number of tasks and any length task names, until the
 * system runs out of memory.  For this project I would accept a
 * solution that assume reasonable maximums for the length of the name
 * and the length of a line.
 */
int parse_input(FILE *in, task_t **task_buf, int *n_tasks)
{
	int     max_tasks;	/* number of tasks that will fit in task_buf */
	int     status;		/* return value from system calls, etc */
	int     task;		/* current index into task_buf */
	task_t *tasks;		/* pointer to array of tasks */
	float   period;		/* period read from input */
	float   exec;		/* execution time read from input */
	float   deadline;	/* deadline read from input */

	*n_tasks = 0;
	max_tasks = 0;
	task = 0;
	*task_buf = NULL;
	while (!feof(in)) {
		if (max_tasks <= *n_tasks) {
			/* not enough space in task_buf, allocate some more */
			max_tasks += TASK_INCR;
			tasks = (task_t *)realloc(*task_buf,
						  sizeof(task_t) * max_tasks);
			if (tasks == NULL) {
				perror("cannot allocate more task space\n");
				free(*task_buf);
				return -ENOMEM;
			}
			*task_buf = tasks;
		}
		/* There's enough space for the task, so read task info */
		if ((status = read_name(in, &tasks[task].name)) < 0) {
			fprintf(stderr, "Error reading name of task %d\n",
				*n_tasks);
			return status;
		}
		status = fscanf(in, " ( %f , %f , %f ) \n",
				&period, &exec, &deadline);
		tasks[task].period = period;
		tasks[task].exec = exec;
		tasks[task].deadline = deadline;
		tasks[task].last_rel = 0;
		tasks[task].next_evt = deadline;
		tasks[task].usage = 0;
		tasks[task].util = exec / deadline;
		tasks[task].job = 0;
		if (status < 3) {
			fprintf(stderr, "Error reading task %s data\n",
				tasks[task].name);
			return -EIO;
		}
		task++;
		(*n_tasks)++;
	}
	/* shrink task_buf to the minimum size */
	if (*n_tasks < max_tasks) {
		tasks = (task_t *)realloc(*task_buf,
					  sizeof(task_t) * *n_tasks);
		if (tasks == NULL) {
			perror("cannot allocate shrink task buffer\n");
			free(*task_buf);
			return -ENOMEM;
		}
		*task_buf = tasks;
	}
	
	return 0;
}

/*
 * read_name
 *
 * Read a task name from the file in.  The pointer returned in name is
 * allocated with malloc and must be freed by the caller later.
 */
static int read_name(FILE *in, char **name)
{
	char *buf;		/* buffer to store name in  */
	char *resize;		/* pointer used when buffer reallocated */
	int   max_len;		/* size of buf */
	int   len;		/* length of name read so far */
	int   ch;		/* next character in name */

	len = 0;
	max_len = TASK_NAME_SIZE;
	if ((buf = (char *)malloc((sizeof(char) * max_len))) == NULL) {
		fprintf(stderr, "Cannot allocate task name\n");
		return -ENOMEM;
	}
	/* read first character; only a letter or '_' are allowed */
	ch = fgetc(in);
	if (!isalpha(ch) && ch != '_') {
		fprintf(stderr, "First char in name must be a letter or '_'\n");
		return -EIO;
	}
	buf[len++] = ch;
	/* read the rest of the name; letters, number and '_'s are allowed */
	while(((ch = fgetc(in)) != EOF)
	      && (isdigit(ch) || isalpha(ch) || ch == '_')) {
		buf[len++] = ch;
		if (len >= max_len) {
			resize = (char *)realloc(buf, max_len + TASK_NAME_SIZE);
			if (resize == NULL) {
				fprintf(stderr, "cannot grow buffer\n");
				return -ENOMEM;
			}
			buf = resize;
			max_len += TASK_NAME_SIZE;
		}
	}
	buf[len++] = '\0';
	/* shrink the buffer to the length of the name */
	if ((resize = (char *)realloc(buf, len)) == NULL) {
		fprintf(stderr, "cannot shrink buffer\n");
		return -ENOMEM;
	}

	*name = resize;
	return 0;
}

/*
 * TODO: There may be fancier ways to compute LCM of a large number of numbers
 */
double compute_hyperperiod(task_t *tasks, int n_tasks)
{
	double *lcm;
	int  tsk;
	int  eq;
	double  max_lcm;

	if ( (lcm = (double *)malloc(n_tasks*sizeof(double))) == NULL) {
		return -ENOMEM;
	}
	max_lcm = 0;
	for(tsk = 0 ; tsk < n_tasks ; tsk++) {
		lcm[tsk] = tasks[tsk].period;
		if (lcm[tsk] > max_lcm)
			max_lcm = lcm[tsk];
	}
	/* Each task requires at least one period.  Add time until all
	 * periods are equal.
	 */
	eq = 0;
	while(!eq) {
		eq = 1;
		for (tsk = 0 ; tsk < n_tasks ; tsk++) {
			while (lcm[tsk] < max_lcm) {
				lcm[tsk] += tasks[tsk].period;
			}
			if (lcm[tsk] != max_lcm) {
				eq = 0;
			}
			if (lcm[tsk] > max_lcm) {
				max_lcm = lcm[tsk];
			}
		}
	}
	free(lcm);

	return max_lcm;
}

/*
 * run_job
 *
 * Description:
 *	Simulate job number job_num in tsk running on the processor
 *	starting at time start and ending at time end with the
 *	processor frequency set to freq.
 *
 * Inputs:
 *	tsk	- task from which to run job
 *	job_num	- job number of job in tsk
 *	start	- time at which to start running
 *	end	- time at which to stop running
 *	freq	- frequency at which to run
 *
 * Outputs:
 *	none
 *
 * Returns:
 *	run_job returns 0 upon sucessful completion.
 *
 * Side Effects:
 *	Job parameters are printed to stdout.
 */
int run_job(task_t *tsk, int job_num, double start, double end, double freq)
{
	printf("%8s.%d, (%6.2f, %6.2f) f = %6.2f",
	       tsk->name, job_num, start, end, freq);
	if (end > tsk->last_rel + tsk->deadline) {
		printf(" missed deadline at t = %6.2f\n",
		       tsk->last_rel + tsk->deadline);
	} else {
		printf("\n");
	}
	return 0;
}
