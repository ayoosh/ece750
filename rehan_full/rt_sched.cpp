/*
 * rt_sched.cpp
 *
 *  Created on: Jun 18, 2013
 *      Author: rehan
 */
#include "scheduler.h"

extern int running_tasks[NUM_PROCESSORS];
extern long long current_time;
extern vector<task>*int_pointer;
extern stringstream logfile;
extern float scheduler_runtime;
extern struct timespec schedulestart;
extern struct timespec starttime;
extern struct timespec endtime;
extern timespec schedule_start;


void insert_deadline(vector<instance>*run_queue, instance *inst )
{
	for(unsigned int i=0;i<run_queue->size();i++)
	{
		if((*inst).deadline<(*run_queue)[i].deadline)
		{
			run_queue->insert(run_queue->begin()+i,(*inst));
			break;
		}
	}
}



void rt_scheduler_init()
{
	signal(SIGINT, int_handler);
	freq_set(CORE,"performance");
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(CORE,&mask);
	cout<<"setting affinity to processor "<<CORE<<endl;
	if(sched_setaffinity(0,CPU_COUNT(&mask),&mask)==-1)
	{
		perror("sched_setscheduler failed");
		exit(-1);
	}
	//lock memory
	if(mlockall(MCL_CURRENT | MCL_FUTURE)==-1)
	{
		perror("mlockall failed");
		exit(-2);
	}
	stack_prefault();

}

void read_temp_msr(int * temperature, int * fd, off_t * offset) {
	for (int i = 0; i < NUM_PROCESSORS; i++) {
		unsigned long long int msr = 0;
		read(fd[i], &msr, sizeof(msr));
		temperature[i] = THRESHOLD - ((msr / 65536) & (0x7F));
	}
}

void stack_prefault(void) {
	unsigned char dummy[MAX_SAFE_STACK];
	memset(dummy, 0, MAX_SAFE_STACK);
	return;
}

void freq_set(int core, string value) {
	stringstream command;
	command << "echo " << value << " |sudo tee /sys/devices/system/cpu/cpu"
			<< core << "/cpufreq/scaling_governor" << endl;
	int i = system(command.str().c_str());
	if (i == -1) {
		cout << "cpu 0 scaling failed" << endl;
		exit(1);
	}
}

void p_stop(int index, vector<task>*tasks) {
	if (index < 0) {
		return;
	}
	if (kill((*tasks)[index].pid, SIGSTOP) != 0) {
		cout << "error killing process with pid " << (*tasks)[index].pid << endl;
		exit(1);
	}
	(*tasks)[index].state = IDLE;
}

void p_start(int index,vector<task>*tasks) {
	if (kill((*tasks)[index].pid, SIGCONT) != 0) {
		cout << "error continuing process with pid " << (*tasks)[index].pid
				<< endl;
		exit(1);
	}
	(*tasks)[index].state = EXECUTING;
}

void p_change_affinity(int index, int core,vector<task>*tasks) {
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(core, &mask);
//	cout<<"setting affinities"<<endl;
	if (sched_setaffinity((*tasks)[index].pid, CPU_COUNT(&mask), &mask) == -1) {
		perror("affinity mask failed");
		exit(-1);
	}
}

void idle_processor(int core, vector<task>*tasks) {
	for (unsigned int i = 0; i < tasks->size(); i++) {
		if ((*tasks)[i].core == core) {
			p_stop(i,tasks);
//			tasks[i].core=-1;
		}
	}
}
void initialize_task(int index, int core, vector<task>*tasks) {
	cout << "initializing task" << endl;
	(*tasks)[index].core = core;
	(*tasks)[index].state = READY;
	(*tasks)[index].pid = fork();
	if ((*tasks)[index].pid > 0) {
		cout << "setting affinity for pid " << (*tasks)[index].pid << endl;
		p_change_affinity(index, core,tasks);
		stringstream temp;
		temp << "/proc/" << (*tasks)[index].pid << "/stat";
		//(*tasks)[index].stat_stream->open(temp.str().c_str());
		(*tasks)[index].stat_stream=new ifstream(temp.str().c_str());
	}
	if ((*tasks)[index].pid == 0) {
		//cout<<"executing command"<<endl;
		cout << "executing command for task index " << index << endl;
		execl("/home/rehan/rt_workspace/sort/Debug/sort", "sort", "1000", NULL);  // running sort task for each task
	}
}

char read_status(ifstream * stat_stream) {
	char result;
	string line;
	getline(*stat_stream, line);
	sscanf(line.c_str(), "%*i %*s %c", &result);
	stat_stream->clear();
	stat_stream->seekg(0, ios::beg);
	return result;
}


void preempt_task(int new_task, int core, vector<task>*tasks) {
	(*tasks)[running_tasks[core]].state = IDLE;
	p_stop(running_tasks[core],tasks);
	run_task(new_task, core,tasks);
}

/*void update_task_info(int core,vector<task>*tasks) {
	unsigned long long required_instances = 0;
	for (unsigned int i = 0; i < tasks->size(); i++) {
		required_instances = required_instances
				+ current_time / (*tasks)[i].period;
		if ((*tasks)[i].state == FINISHED && (*tasks)[i].next_start <= current_time) {
			(*tasks)[i].state = READY;
		}

	}

	if (instances_executed < required_instances) {
		cout << "TIME: " << current_time << " DEADLINE MISS: completed "
				<< instances_executed << " required " << required_instances
				<< endl;
		exit(1);
	}
	if (current_time % (hyperperiod) == 0) {
		for (unsigned int i = 0; i < tasks->size(); i++) {
			computations[i] = 0;
		}
	}
	if (running_tasks[core] == -1) {

		return;
	}

#if DEBUG
	struct timespec ttime;
	clock_gettime(1,&ttime);
	cout<<"status read start time: "<<ttime.tv_sec<<" seconds "<<ttime.tv_nsec<<"nanoseconds"<<endl;
#endif

	char res = read_status((*tasks)[running_tasks[core]].stat_stream);

#if DEBUG
	clock_gettime(1,&ttime);
	cout<<"status read end time: "<<ttime.tv_sec<<" seconds "<<ttime.tv_nsec<<"nanoseconds"<<endl;
#endif

	if (res == 'S' && (*tasks)[running_tasks[core]].state == EXECUTING) {

		cout << "running task on record " << running_tasks[core] << " at time "
				<< current_time << endl;

		int hyp_corrected_time = current_time % hyperperiod;
		int instance_count = hyp_corrected_time
				/ (*tasks)[running_tasks[core]].period + 1;

#if (TIMING)

		clock_gettime(1, &endtime);
		cout << "task end time " << endtime.tv_sec - schedulestart.tv_sec
				<< " seconds " << endtime.tv_nsec << " nanoseconds" << endl;
		cout << "cumulative execution time for task " << running_tasks[core]
				<< " is "
				<< computations[running_tasks[core]]
						- (((hyp_corrected_time
								/ (*tasks)[running_tasks[core]].period)
								* (*tasks)[running_tasks[core]].computation_time))
				<< "ms" << endl;
#endif
		computations[running_tasks[core]] = instance_count
				* (*tasks)[running_tasks[core]].computation_time;

		logfile << "#Finished task " << running_tasks[core] << " on core "
				<< core << endl;
		(*tasks)[running_tasks[core]].next_start =
				(*tasks)[running_tasks[core]].next_start
						+ (*tasks)[running_tasks[core]].period;
//		tasks[running_tasks[core]].deadline =
//				tasks[running_tasks[core]].deadline
//						+ tasks[running_tasks[core]].period;
		(*tasks)[running_tasks[core]].state = FINISHED;
		instances_executed = instances_executed + 1;
		running_tasks[core] = -1;
	}
}
*/
void int_handler(int signum) {
	cout << "sending terminate signals" << endl;
	for (unsigned int i = 0; i < int_pointer->size(); i++) {
		kill((*int_pointer)[i].pid, SIGINT);
	}
	pid_t childpid;
	for (unsigned int i = 0; i < int_pointer->size(); i++) {
		childpid = waitpid(-1, NULL, 0);
		cout << "successfully ended process " << childpid << endl;
	}
	ofstream outputfile;
	outputfile.open("logfile");
	outputfile << logfile.str();
	outputfile << "#scheduler runtime = " << scheduler_runtime;
	outputfile.close();

	exit(1);
}

bool update_temp(int * new_temp, int * old_temp) {

	bool ret = false;
	for (int i = 0; i < NUM_PROCESSORS; i++) {
		if (new_temp[i] != old_temp[i]) {
			ret = true;
			old_temp[i] = new_temp[i];
		}
	}
	return ret;
}

void run_task(int index, int core,vector<task>*tasks) {
#if (TEMP_EXPERIMENT==1)

	cout<<"entered to start running task "<<index<<" on core "<<core<<endl;

#endif
/*	if (index != -1) {
		computations[index] = computations[index] + GRANULARITY;
		previous_idle = false;
	}*///************************************************************ ALTERED cause speculating that it will not be required

	if (index != -1 && (*tasks)[index].state == EXECUTING) {
		return;
	}
	if (running_tasks[core] != -1) {
		p_stop(running_tasks[core],tasks);
		running_tasks[core] = -1;
//			tasks[running_tasks[core]].state=IDLE;
//			logfile<<"#stopping task "<<running_tasks[core]<<" on core "<<core<<endl;

	}
	if (index == -1) {
		return;
	}

//		logfile<<"#starting/resuming task "<<index<<" on core "<<core<<endl;
	running_tasks[core] = index;
	(*tasks)[index].core = core;

	p_change_affinity(index, core,tasks);

	if ((*tasks)[index].state == READY) {
#if (TIMING && SCHED_SCHEME==1)
		clock_gettime(1, &starttime);
		cout << "signal send time" << starttime.tv_sec - schedulestart.tv_sec
				<< " seconds " << starttime.tv_nsec << " nanoseconds" << endl;
		cout << "running task:" << index << endl;
#endif
		logfile << "#starting/resuming task " << index << " on core " << core
				<< endl;
		kill((*tasks)[index].pid, SIGUSR1);
		(*tasks)[index].state = EXECUTING;
//		cout << "signal sent from parent at time " << current_time << endl;
	} else {
		p_start(index,tasks);
	}

}



void run_task2(int index, int core,vector<task>*tasks) {
#if (TEMP_EXPERIMENT==1)

	cout<<"entered to start running task "<<index<<" on core "<<core<<endl;

#endif
/*	if (index != -1) {
		computations[index] = computations[index] + GRANULARITY;
		previous_idle = false;
	}*///************************************************************ ALTERED cause speculating that it will not be required

	if (running_tasks[core] != -1) {
		p_stop(running_tasks[core],tasks);
		running_tasks[core] = -1;
//			tasks[running_tasks[core]].state=IDLE;
//			logfile<<"#stopping task "<<running_tasks[core]<<" on core "<<core<<endl;

	}
	if (index == -1) {
		return;
	}

//		logfile<<"#starting/resuming task "<<index<<" on core "<<core<<endl;
	running_tasks[core] = index;
	(*tasks)[index].core = core;

	p_change_affinity(index, core,tasks);

	if ((*tasks)[index].state == READY) {
//		clock_gettime(1, &starttime);
//		cout << "signal send time" << starttime.tv_sec - schedulestart.tv_sec
//				<< " seconds " << starttime.tv_nsec << " nanoseconds" << endl;
//		cout << "running task:" << index << endl;
//		logfile << "#starting/resuming task " << index << " on core " << core
//				<< endl;
		kill((*tasks)[index].pid, SIGUSR1);
//		(*tasks)[index].state = EXECUTING;
		//cout << "signal sent from parent at time " << current_time << endl;
	} else {
		p_start(index,tasks);
	}
	timespec task_runtime;
	//clock_gettime(1,&task_runtime);
	//cout<<"running task "<<index<<" at time "<<time_diff(&schedule_start,&task_runtime)<<endl;

}

void status_loop(int index, vector<task>*tasks,char state)
{
	timespec interval;
	clock_gettime(1,&interval);

	char c=state;

	while(c==state)
	{
		interval.tv_nsec=interval.tv_nsec+10000;//reading status after every 10 us
		while(interval.tv_nsec>=1000000000)
		{
			interval.tv_nsec=interval.tv_nsec-1000000000;
			interval.tv_sec++;
		}
		c=read_status((*tasks)[index].stat_stream);
		//cout<<"status "<<c<<"looping on "<<state<<endl;
		clock_nanosleep(1,1,&interval,NULL);
//		cout<<"status at "<<time_diff(&schedule_start,&interval)<< " is "<<c<<endl;
	}
}



