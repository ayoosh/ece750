#include "scheduler.h"

float beta_multi[CORE][CORE];
int running_tasks[NUM_PROCESSORS] = { -1, -1, -1, -1 };
long long current_timedd = -GRANULARITY;
vector<task>*int_pointer;
stringstream logfile;
float scheduler_runtime = 0;
struct timespec schedulestart, starttime, endtime;

//****************global variables********************************
 double beta = 1 / (R * C) - (DEL / C);
 double corrected_threshold;
 int thermal_optimal = 0;
 double global_sort_power = 0;
 unsigned int seed;
 float speed_levels[S_L]={0.5, 0.533, 0.567, 0.6, 0.633, 0.667, 0.7, 0.733, 0.767, 0.8, 0.833, 0.867, 0.9, 0.933, 0.967, 1.0};


 vector<float> speeds;
 vector<taskset> tasksets;
//****************************************************************

int main(int argc, char* argv[]) {

	if (argc < 2) {
		cout<< "invalid format: <taskfile>"	<< endl;
		exit(1);
	}

	corrected_threshold = corrected_temperature(THRESHOLD);
	cout<<"beta "<<beta<<" corrected threshold "<<corrected_threshold<<endl;

	int iter = 1;
	int hyperperiod = 0;

	string task_file;
	task_file = argv[1];

	vector<task> tasks;
	vector<schedule> edf;
	read_tasksets(&tasks, task_file);
		
    int_pointer=&tasks;
		
    double t_util;
	edf_schedule(&tasks, &edf);
	consolidate_schedule(&edf, &tasks);

	compute_profile(&edf, &tasks, t_util);
}

