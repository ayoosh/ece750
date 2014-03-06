#include <stdlib.h>

#include "proj3.h"

void enqueue(task_t **queue, task_t *tsk);
task_t *dequeue(task_t **queue);

static task_t *ready;		/* tasks ready to run */
static task_t *wait;		/* tasks not yet released */
double freq;			/* current relative frequency */
double volts;			/* current relative voltage */

static task_t *tasks;		/* tasks to schedule */
static int     ntask;		/* number of tasks */

/*
 * release
 *
 * Description:
 *		Release a new job
 * Inputs: 	
 *		tsk	- task from which to release the job
 * Outputs:	
 *		tsk	- updated task structure
 * Returns:
 *		nothing
 * Side Effects:
 *		none
 */
void release(task_t *tsk)
{
	tsk->usage = 0;
	/* actual execution time is random... */
	/* you might want to comment out the random variation in
	 * execution time for testing purposes, but make sure it is
	 * still there in the final version you submit.
	 */
	tsk->cur_exec = tsk->exec * random() / (double)RAND_MAX;
}

/*
 * completion
 *
 * Description:
 *		Simulate a job running to completion
 * Inputs: 	
 *		now	- current time before job runs
 * Outputs:	
 *		none
 * Returns:
 *		completion returns the time at which the job completes
 * Side Effects:
 *		The current job is removed from the ready queue and placed
 *		in the wait queue in order of next release time.
 */
double completion(double now)
{
	task_t *cur;		/* pointer to current task */
	double exec_left;	/* amount of time left before job completes */

	/* simulate job running to completion */
	exec_left = (ready->cur_exec - ready->usage);
	run_job(ready, ready->job, now, now + exec_left/freq, freq);
	now += exec_left/freq;

	/* remove job from ready queue and insert in wait queue */
	(ready->job)++;
	cur = dequeue(&ready);
	cur->last_rel += cur->period;
	cur->next_evt = cur->last_rel;
	enqueue(&wait, cur);

	return now;
}

/*
 * schedule_tasks
 *
 * Description:
 *		Schedule tasks using EDF.
 *
 * Inputs:
 *		sched_task	- an array of tasks to schedule
 *		n_tasks		- number of tasks to schedule
 *
 * Outputs:
 *		none
 *
 * Returns:
 *		schedule_tasks return 0 upon success.
 *
 * Side Effects
 *		Job related fields in sched_task are modified.
 */
int schedule_tasks(task_t *sched_task, int n_tasks)
{
	double now;		/* current time */
	double end;		/* time to end simulation */
	double energy;		/* total energy used */
	int    index;

	task_t *cur;		/* pointer to current task */

	tasks = sched_task;
	ntask = n_tasks;
	srandom(599);

	/* all tasks are initially released at t = 0 */
	for (index = 0 ; index < n_tasks ; index++) {
		enqueue(&ready, &tasks[index]);
		release(&tasks[index]);
	}
	wait = NULL;

	freq = 1.0;
	volts = 1.0;
	energy = 0.0;

	/* run simulation for 2x the hyperperiod */
	end = compute_hyperperiod(tasks, n_tasks) * 2;
	now = 0.0;
	while(now < end) {
		/* run next job in ready queue */
		if (wait == NULL
		    || (now + (ready->cur_exec - ready->usage)/freq
			<= wait->next_evt)) {
			/* job runs to completion (i.e., it's not preempted) */
			energy += (ready->cur_exec - ready->usage)
			  * freq * volts * volts; 
			now = completion(now);
		} else {
			/* scheduling decision at next_evt */
			run_job(ready, ready->job, now, wait->next_evt, freq);
			ready->usage += (wait->next_evt - now) * freq;
			energy += (wait->next_evt-now) * freq * volts * volts;
			now = wait->next_evt;
		}
		if (ready == NULL) {
			/* skip idle time until next release */
			now = wait->next_evt;
		}
		/* release jobs in wait queue that are at release time */
		while(wait && wait->next_evt <= now) {
			cur = dequeue(&wait);
			cur->next_evt += cur->deadline;
			release(cur);
			enqueue(&ready, cur);
		}
	}
	printf("energy used = %f\n", energy);
	
	return 0;
}

/*
 * enqueue
 *
 * Description: enqueue tsk in queue sorted by tsk->next_evt
 * Inputs: 	
 *		queue	- queue in which to insert tsk
 *		tsk	- task to insert in queue
 * Outputs:	
 *		none
 * Returns:
 *		nothing
 * Side Effects:
 *		queue is updated such that tsk is inserted in order
 *		based on the next_evt field.
 */
void enqueue(task_t **queue, task_t *tsk)
{
	task_t *prev;

	if (*queue == NULL || (*queue)->next_evt > tsk->next_evt) {
		tsk->next = *queue;
		*queue = tsk;
	} else {
		prev = *queue;
		while(prev->next && prev->next->next_evt < tsk->next_evt) {
			prev = prev->next;
		}
		if (prev->next) {
			tsk->next = prev->next;
		} else {
			tsk->next = NULL;
		}
		prev->next = tsk;
	}
}


/*
 * dequeue
 *
 * Description: dequeue the task at the head of queue.  Empty queues
 *		not handled.
 * Inputs: 	
 *		queue	- queue from which to tsk
 * Outputs:	
 *		none
 * Returns:
 *		task formerly at the head of queue
 * Side Effects:
 *		queue is updated to point to then next element in queue.
 */
task_t *dequeue(task_t **queue)
{
	task_t *head;

	head = *queue;
	*queue = (*queue)->next;

	return head;
}

