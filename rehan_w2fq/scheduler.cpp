
#define S_COMP_CAP  5
#define S_POWER     50
#define S_PERIOD    50

#include "scheduler.h"
#include <tgmath.h>
#include <algorithm>
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

#define max(a,b) ({ __typeof__ (a) _a = (a); \
                __typeof__ (b) _b = (b); \
              _a > _b ? _a : _b; })

bool compare_instances(instance a, instance b) {
    if(a.deadline < b.deadline) {
        return true;
    } else if(a.deadline == b.deadline) {
        if (a.arrival < b.arrival) {
            return true;
        }
    }
    return false;
}

void tasks2instances(vector<task> *periodic_tasks, vector<instance> *aperiodics, vector<instance> *instances) {
    int i, start = 0;
    instance temp;

	for (i = 0; i < periodic_tasks->size(); i++) {
		while (start < tasksets[0].hyperperiod) {
			temp.computation_time = (*periodic_tasks)[i].computation_time;
			temp.arrival = start;
			temp.deadline = start + (*periodic_tasks)[i].period;
			start = temp.deadline;
			temp.task_id = i;
            temp.power = (*periodic_tasks)[i].power;
			instances->push_back(temp);
		}
	}
    
    instances->insert(instances->end(), aperiodics->begin(), aperiodics->end());
    sort(instances->begin(), instances->end(), compare_instances);
}

#define S_THERM_CAP ((S_COMP_CAP * S_POWER) / beta)
int s_last_deadline = 0;
void generate_aperiodics(vector<instance> *aperiodics, int arrival, int computation_time, double power) {
    double util_ratio, TTI = power * computation_time / beta;
    instance temp, tempi;
    double comp_time_remaining = computation_time;
    unsigned int i, ceiling = ceil(TTI / S_THERM_CAP);

    for (i = ceiling; i > 0; i--) {
        temp.arrival    = arrival;
        temp.power      = power;
        tempi.power     = 0;

        if (i == 1) {
            util_ratio = max(1, power / S_POWER);
            temp.deadline = max(arrival, s_last_deadline) + (comp_time_remaining * ( (double) S_PERIOD / S_COMP_CAP) * util_ratio);
            temp.computation_time = comp_time_remaining;
            tempi.computation_time = comp_time_remaining * (util_ratio - 1);
        } else {
            temp.deadline =  max(arrival, s_last_deadline) + S_PERIOD;
            temp.computation_time = S_COMP_CAP / util_ratio;
            tempi.computation_time = S_COMP_CAP - temp.computation_time;
            comp_time_remaining -= temp.computation_time;
        }
        
        tempi.arrival = arrival + temp.computation_time;
        s_last_deadline = tempi.deadline = temp.deadline;
        aperiodics->push_back(temp);
        aperiodics->push_back(tempi);
    }

    return;
}


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
	//ab_edf_schedule(&tasks, &edf);
	consolidate_schedule(&edf, &tasks);

	ab_compute_profile(&edf, &tasks, t_util);
}


