/*
 * scheduler.h
 *
 *  Created on: Jun 15, 2013
 *      Author: rehan
 */

#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <sched.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
//**********************************
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <assert.h>
#include <algorithm>
#include <climits>
#include <math.h>
#include <iterator>
//#include <engine.h>
#include<string.h>
#include<time.h>
#include <random>
#include "config.h"
//#include "strtk.hpp"

using namespace std;



//******************global variables**********************
//extern float global_tasks[][4];
//extern float global_instances[][6];
//extern float interval_req[][13];
extern double global_tasks[][4];
extern double global_instances[][6];
extern double interval_req[][13];
extern int g_task_size;
extern int g_instance_size;
extern int interval_size;
extern float beta_multi[CORE][CORE];
extern float speed_levels[16];

//******************************************************

//Engine *ep;

//float comp_util;

struct taskset {
	float TTI;
	float average_power;
	float c_util;
	float t_util;
	int hyperperiod;
};

struct interval_tasks {
	int task_id;
	double computations;
	double power;
};

struct task {
	int computation_time;
	int period;
	int taskset;
	float power;
	float TTI;
	int computations;
	int next_start;
	int index;
	int tid;

	pid_t pid; // linux pid
	int core; // core on which task is executing
	int state; // state of task (FINISHED, EXECUTING or IDLE)
	int priority; // linux priority of task
	vector<string> command;
	ifstream *stat_stream;
	float end_temp[NUM_PROCESSORS];
};

struct float_task {
	float computation_time;
	float computations;
	float next_start;
	float period;
	float power;
	int index;

    int taskset;
};

struct long_task {
	long computation_time;
	long computations;
	long next_start;
	long period;
	double power;
	int index;
};

struct mprofile {
	double time;
	double val[CORE];
};

struct slack {
	int start;
	int end;
};

struct profile {
	double time;
	double temperature;

};

struct schedule {
	int task_id;
	int start;
	int end;
	float speed;
	int arrival;
	float power;
};

struct float_schedule {
	int task_id;
	double start;
	double end;
	double speed;
	double arrival;
	double power;
    bool is_last;
};

struct long_schedule {
	int task_id;
	long start;
	long end;
	double float_end;
	double speed;
	long arrival;
	double power;
	int core;
};

struct instance {
	int task_id;
	float arrival;
	float deadline;
//	int computation_time;
    float computation_time;
	float computations;
	float comps_left;
	float comps_done;
	float speed;
	float power;
	int mapping;
	int next_start;
    bool is_last;
	//int instance_id;
};

struct cd_computations {
	int deadline;
	int computations;
};

struct comps_req {
	int id;
	int start;
	int end;
	int computations; //unscaled computations
	float comps;
	int arrival;
	float speed;
	int mapping;
};

struct interval_s {
	long start;
	long end;
	long computations[MAX_TASKS][CORE];
	long net_computations[MAX_TASKS][CORE];
//	double computations[num_tasks];
//	double net_computations[num_tasks];

};

struct execution{
	int task;
	double speed;
	int speed_index;
	int core;
	long exec;
	float unit_exec;
};

struct trace {
	float val[CORE];

};

class interval
{
public:
	long start;
	long end;
	vector<execution> exec;
	interval();
	interval(double st,double en);
	~interval();
	void add_exec(execution newexec);
	long get_length();
	void set_interval(long st,long en);
	int get_size();
	execution get_element(int index);
	void clear();
private:


};


//util.cpp******************************************************
float corrected_temperature(float temperature);
void factorise(vector<int>* factors, int big_number);
int gcd(int a, int b);
int lcm(int a, int b);
int compute_lcm(vector<task>* tasks, int set);
int compute_lcm(vector<float_task>* tasks);
int compute_lcm(vector<long_task>* tasks);
void store_taskset(vector<task>*tasks);
void generate_instances(vector<task>*tasks, char *fname);
void generate_tasksets(vector<task>* tasks, int num_tasksets, int hyperperiod,
		int min_util, int max_util);
void generate_taskset(vector<float_task> *tasks, long hyperperiod,
		int num_tasks, float comp_util, float thermal_util);
void ab_generate_taskset(vector<float_task> *tasks, long hyperperiod,
        int num_tasks, float comp_util, float thermal_util);
void generate_periodic_taskset(vector<float_task> *tasks, long hyperperiod,
        int num_tasks, float comp_util, float thermal_util);
void generate_taskset(vector<float_task> *tasks, long hyperperiod,
		int num_tasks, float comp_util);
void generate_taskset(vector<task> *tasks, long hyperperiod,
		int num_tasks, float comp_util);
void read_tasksets(vector<task>*tasks, string fname);
void read_tasksets(vector<float_task>*tasks);
bool ascending(int a, int b);
bool float_ascending(float a, float b);
void imp_times(vector<task> * tasks, vector<int>*times);
void imp_times(vector<float_task> * tasks, vector<float>*times);
void imp_times(vector<long_task> * tasks, vector<long>*times);
void imp_times_partial(vector<task> * tasks, int * computations,
		vector<int>*times);
int min_deadline(vector<task>*tasks, int start);
int min_deadline(vector<float_task>*tasks, float start);
int min_deadline(vector<task>*tasks, int start, vector<int>*times,
		int *time_end);
double global_power(vector<task>*tasks, int start, int end, float util);
double global_power_partial(vector<task>*tasks, vector<int>*decisions,
		float util);
bool sort_power(task a, task b);
void generate_instances(vector<task>*tasks, vector<instance> *inst,
		float* speeds);
void partial_instances(vector<task>*tasks, vector<instance>*inst,
		vector<instance>*r_inst, float* speeds, int start, int end);
int myfloor(float f);
int myceil(float f);
void write_to_file(string fname, string *str);
void split(string *text, vector<string> *delimiters, vector<string>*elem);
//inline float time_diff(timespec* start,timespec *end);
//********************************************************************

//optimize.cpp functions***************************************
void optimize(vector<vector<double> >*solutions, vector<vector<int> >*order,
		int vector_index, vector<task>*tasks, double min_speed,
		double max_speed);
double optimize_static(vector<double> *output_speeds, vector<task>*tasks,
		double min_speed, double max_speed);
void scale(vector<task>*scaled_tasks, vector<task>*tasks, vector<double>*speed);
double optimize_maxfirst(vector<double> *output_speeds, vector<task>*tasks,
		double min_speed, double max_speed);
double optimize_minfirst(vector<double> *output_speeds, vector<task>*tasks,
		double min_speed, double max_speed);
double optimize_maxmin(vector<double> *output_speeds, vector<task>*tasks,
		double min_speed, double max_speed);
void optimize_serial(vector<double> *output_speeds, vector<task>*tasks,
		double min_speed, double max_speed);
void speed_scale(vector<task>*scaled_tasks, vector<double>*speeds,
		vector<task>*tasks, float adjust);
void speed_scale_discrete(vector<task>*scaled_tasks, vector<float>*speeds,
		vector<task>*tasks);

//***********************************************

// base_sched.cpp functions**************************************
//double heat(double init_temp, double power, int time);
//double cool(double init_temp, int time);
double heat(double init_temp, double power, double time);
void heat(double init_temp[CORE], double power[CORE], double time, double *out);
double cool(double init_temp, double time);
void ab_edf_schedule(vector<float_schedule> *, vector<instance> *, vector<float_task> * tasks);
void edf_schedule(vector<task> * tasks, vector<schedule>*edf);
void edf_schedule(vector<float_task> * tasks, vector<float_schedule>*edf);
void populate_slacks(vector<slack>*slacks, vector<schedule>*sch);
void compute_profile(vector<schedule>* sch, vector<task>*tasks,
		double thermal_util);
bool ab_compute_profile(vector<float_schedule>* sch, bool);
void compute_profile_multi(vector<schedule>* sch, vector<task>*tasks,
		double thermal_util);
void consolidate_schedule(vector<schedule>*sch, vector<task>*tasks);
void edl_schedule(vector<schedule>*edl, vector<schedule>*edf,
		vector<task>* tasks, vector<slack>*slacks);
void edl_schedule2(vector<schedule>*edl, vector<schedule>*edf);
void verify(vector<schedule>*sch, vector<task>*tasks);
void verify(vector<float_schedule>*sch, vector<float_task>*tasks);
void verify_s(vector<schedule>*sch, vector<schedule>*edf, vector<task>*tasks);
void verify(vector<schedule>*sch, vector<task>*tasks, vector<double>*speed);
void compute_profile_dynamic(vector<schedule>* sch, vector<task>*tasks,
		double thermal_util, string append);
void compute_profile_multi(vector<long_schedule>* sch, vector<long_task>*tasks);
void compute_profile(vector<float_schedule>* sch, vector<float_task>*tasks,
		int mode);
void resource_matrix(int ** matrix, int size, float likelyhood);
bool sort_sch(long_schedule a, long_schedule b);
bool compare_deadline(instance a, instance b);
//**************************************************

//slack.cpp********************************************************
bool sort_inst_deadline(instance a, instance b);
bool sort_inst_arrival(instance a, instance b);
int compute_slack2(int start, int *computations, vector<schedule>*sch,
		vector<task>*tasks, vector<int>*possible_tasks);
int compute_slack(int start, int * tid, int *computations, vector<schedule>*sch,
		vector<task>*tasks, vector<int>*possible_tasks);
int compute_slack_exact(int start_time, int* tid, int * computations,
		vector<task>*tasks, vector<int>*possible_tasks);
//****************************************************************

//power_dist.cpp**************************************************
void possibles(vector<int>*possible_tasks, int start, vector<task>*tasks);
float min_deviation(int *id, vector<int>*possible_tasks, vector<task>*tasks,
		float initial_temp, float target_temp);
float mlt(int *id, vector<int>*possible_tasks, vector<task>*tasks,
		float initial_temp, float max_temp);
void opt_schedule_exact(vector<schedule>*sch, vector<task> * tasks);
void opt_schedule(vector<schedule>*sch, vector<task> * tasks,
		vector<schedule>*ref);
void store_opt(vector<task>*tasks);
void call_gams();

//****************************************************************
// inst_sched.cpp*****************************************************
double global_power_instances(vector<task>*tasks, int start, int*computations);
void optimize_instances(vector<double> *output_speeds, vector<task>*tasks,
		vector<schedule>*sch, vector<schedule>*ref, vector<schedule>*ref2,
		double min_speed, double max_speed, vector<double>*optimal_speeds,
		vector<int>*sl);

//****************************************************************************
// rt_sched.cpp***************************************************************
void insert_deadline(vector<instance>*run_queue, instance *inst);
void rt_scheduler_init();
void read_temp_msr(int * temperature, int * fd, off_t * offset);
void stack_prefault(void);
void freq_set(int core, string value);
void p_stop(int index, vector<task>*tasks);
void p_start(int index, vector<task>*tasks);
void p_change_affinity(int index, int core, vector<task>*tasks);
void idle_processor(int core, vector<task>*tasks);
void initialize_task(int index, int core, vector<task>*tasks);
char read_status(ifstream * stat_stream);
void preempt_task(int new_task, int core, vector<task>*tasks);
void int_handler(int signum);
bool update_temp(int * new_temp, int * old_temp);
void run_task(int index, int core, vector<task>*tasks);
void run_task2(int index, int core, vector<task>*tasks);
void status_loop(int index, vector<task>*tasks, char c);
//******************************************************************************

//rt_impl.cpp*****************************************************************
void init_all(vector<task>*tasks);
void run_dynamic(vector<task>*tasks, float * speeds);
void run_schedule(vector<schedule>*sch, vector<task>*tasks);
void scheduler(vector<schedule>*o_sch, vector<task>*tasks,
		vector<instance>*dyn_inst, float* speeds, int period);
void scheduler2(vector<schedule>*o_sch, vector<task>*tasks,
		vector<instance>*dyn_inst, float* speeds, int period);
void dynamic_instances(vector<task>*tasks, float* speeds,
		vector<instance>*inst);

//intrval_sched.cpp***********************************************************
double average_power(vector<float_task>*tasks);
void interval_sched(vector<float_schedule>*sch, vector<float_task>*tasks);

//w2fq.cpp*********************************************************************
void w2fq_task_convert(vector<long_task>*tasks, vector<float_task>*f_tasks);
void w2fq_schedule(vector<long_schedule>*sch, vector<long_task>*tasks,
		vector<interval_s>*intervals, int core);
void w2fq_schedule_convert(vector<float_schedule>*fsch,
		vector<long_schedule>*lsch);
void generate_intervals(vector<interval_s>* intervals, vector<long_task>*tasks);
void generate_intervals_gps(vector<interval_s>* intervals,
		vector<long_task>*tasks);
bool interval_ascend(interval_s a, interval_s b);

//dynamic_interval.cpp*********************************************************
void dynamic_instance_schedule(vector<long_schedule>*sch,
		vector<long_task>*tasks, vector<interval_s>*intervals,
		int speed_scaling_enable);
void instance_override(vector<float_task>*tasks);

//dynamic_interval_speed.cpp***************************************************
void dynamic_instance_schedule_speed(vector<long_schedule>*sch, vector<long_task>*tasks,
		                             vector<interval>*intervals, vector<instance>*inst,int speed_scaling_enable);
void instance_override_speed(vector<float_task>*tasks, vector<instance>*inst);


//multi.cpp**********************************************************************
void generate_taskset_multi(vector<float_task> *tasks, long hyperperiod,
		int num_tasks, float comp_util);
int gams_include(vector<float_task>*tasks, int ** matrix);
void generate_intervals_multi(vector<interval_s>*intervals, string fname,
		vector<float_task>*tasks);
void generate_intervals_multi(vector<interval>*intervals, string fname);
void generate_intervals_multi(vector<interval>*intervals, string fname,
		vector<float_task>*tasks);


void multi_schedule(vector<long_schedule>*sch, vector<interval_s>*intervals,
		vector<long_task>*tasks);
void generate_power_trace(vector<long_schedule>*sch, string fname,
		long hyperperiod, float * average_power);
void total_comps(vector<long_schedule>*sch, vector<long_task>*tasks,
		vector<interval_s>*intervals);
void read_taskset_multi(vector<float_task> *tasks, string file);
void edf_schedule_multi(vector<long_task> * tasks, vector<long_schedule>*edf);
void populate_beta();
void generate_power_profile(vector<mprofile> *profile,
		vector<long_schedule>*sch, long hyperperiod);

//**********************hotspot_util.cpp*********************************************
void multi_simulate(string power_profile, string flp, int mode, double power);
void call_hotspot(string ptrace, string flp);
void call_hotspot(string ptrace, string ttrace, string flp, string init);
double read_ttrace(string fname, vector<trace>*therm);

//*****************inline*****************************************************
inline float time_diff(timespec* start, timespec *end) {
	return ((end->tv_sec - start->tv_sec) * 1000.0000000
			+ ((float) (end->tv_nsec - start->tv_nsec)) / 1000000.000000);
}

#endif /* SCHEDULER_H_ */
