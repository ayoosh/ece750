#define INTERLEAVE  1
#define INTERLEAVE_GRANULARITY 1
#define CHOOSE_REAL_FIRST 1
#define S_COMP_CAP  5
#define S_POWER     50
#define S_PERIOD    50

#include "scheduler.h"
#include <tgmath.h>
#include <algorithm>
#include <iomanip>
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

#define min(a,b) ({ __typeof__ (a) _a = (a); \
                __typeof__ (b) _b = (b); \
              _a < _b ? _a : _b; })

bool compare_arrival(instance a, instance b) {
    if (a.arrival < b.arrival) {
        return true;
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
			temp.task_id = instances->size();
            temp.power = (*periodic_tasks)[i].power;
			instances->push_back(temp);
		}
        start = 0;
	}
    
    instances->insert(instances->end(), aperiodics->begin(), aperiodics->end());
    sort(instances->begin(), instances->end(), compare_arrival);
}

void insert_aperiodic_instances(instance real, instance idle, vector<instance> *aperiodics) {

// If interleaving is desired, interleave real and idle tasks in a round robin fashion, with a predefined INTERLEAVE_GRANULARITY
#if INTERLEAVE

    if (idle.computation_time == 0) {
        // This real task has a power less than server power. Nothing to do here.
        aperiodics->push_back(real);
        return;
    }

    double arrival = real.arrival;
    bool choose_real = CHOOSE_REAL_FIRST; // Controls if real task is chosen first or the idle one. After this the choice is round robin.
    double comp_ratio = real.computation_time / idle.computation_time;
    real.comps_left = real.computation_time;
    idle.comps_left = idle.computation_time;
    
    instance temp;
    temp.deadline = real.deadline;
    
    /*
     * In every iteration choose one of real or idle task,
     * Take care of one INTERLEAVE_GRANULARITY computation time
     */
    while ((real.comps_left > 0) && (idle.comps_left > 0)) {

        /*
         * Interleaving depends on staggering arrival times.
         * Makse sure to break ties based on arrival times in EDF
         */
        temp.arrival = arrival;
        if (choose_real) {
            if (comp_ratio > 1) {
                temp.computation_time = min(INTERLEAVE_GRANULARITY * comp_ratio, real.comps_left);
            } else {
                temp.computation_time = min(INTERLEAVE_GRANULARITY, real.comps_left);
            }
            temp.task_id = real.task_id;
            temp.power = real.power;
            real.comps_left -= temp.computation_time;
        } else {
            if (comp_ratio < 1) {
                temp.computation_time = min(INTERLEAVE_GRANULARITY / comp_ratio, idle.comps_left);
            } else {
                temp.computation_time = min(INTERLEAVE_GRANULARITY, idle.comps_left);
            }
            temp.task_id = idle.task_id;
            temp.power = idle.power;
            idle.comps_left -= temp.computation_time;
        }

        choose_real = !choose_real;
        arrival += temp.computation_time;
        aperiodics->push_back(temp);
    }

    /* 
     * One of the real or idle is handled completely till now
     * Dump remaining computation time of the other in one contiguous task
     */
    if (real.comps_left > 0) {
        real.arrival = arrival;
        real.computation_time = real.comps_left;
        aperiodics->push_back(real);
    }

    if (idle.comps_left > 0) {
        idle.arrival = arrival;
        idle.computation_time = idle.comps_left;
        aperiodics->push_back(idle);
    }

#else
    // If we do not want to Interleave real and idle parts just add them to instances as it is. No sweat.
    aperiodics->push_back(real);
    if(idle.computation_time) {
        aperiodics->push_back(idle);
    }
#endif
    return;
}

#define S_THERM_CAP ((S_COMP_CAP * S_POWER) / beta)
double s_last_deadline = 0;
int aper_task_id = 500;
void generate_aperiodics(vector<instance> *aperiodics, int arrival, int computation_time, double power) {
    double TTI = power * computation_time / beta;
    instance temp, tempi;
    double comp_time_remaining = computation_time;
    unsigned int i, ceiling = ceil(TTI / S_THERM_CAP);

    double util_ratio = max(1, power / S_POWER);
    
    for (i = ceiling; i > 0; i--) {
        temp.arrival    = max(arrival, s_last_deadline);
        temp.power      = power;
        tempi.power     = 0;

        if (i == 1) {
            temp.deadline = max(arrival, s_last_deadline) + (comp_time_remaining * ( (double) S_PERIOD / S_COMP_CAP) * util_ratio);
            temp.computation_time = comp_time_remaining;
            tempi.computation_time = comp_time_remaining * (util_ratio - 1);
        } else {
            temp.deadline =  max(arrival, s_last_deadline) + S_PERIOD;
            temp.computation_time = S_COMP_CAP / util_ratio;
            tempi.computation_time = S_COMP_CAP - temp.computation_time;
            comp_time_remaining -= temp.computation_time;
            arrival += S_COMP_CAP;
        }
        
        tempi.arrival = arrival + temp.computation_time;
        s_last_deadline = tempi.deadline = temp.deadline;
        temp.task_id = tempi.task_id = aper_task_id;
        insert_aperiodic_instances(temp, tempi, aperiodics);
        /*
        aperiodics->push_back(temp);
        if(tempi.computation_time) {
            aperiodics->push_back(tempi);
        }
        */
    }

    aper_task_id++;
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
    double t_util;

	string task_file;
	task_file = argv[1];

	vector<task> periodic_tasks;
	vector<float_schedule> edf;
    vector<instance> aperiodics;
    vector<instance> instances;

	read_tasksets(&periodic_tasks, task_file);
    generate_aperiodics(&aperiodics, 10, 5, 50);
    generate_aperiodics(&aperiodics, 30, 5, 100);
    generate_aperiodics(&aperiodics, 31, 5, 5);
    generate_aperiodics(&aperiodics, 50, 5, 200);
    generate_aperiodics(&aperiodics, 55, 5, 50);
    generate_aperiodics(&aperiodics, 70, 5, 0);

    tasks2instances(&periodic_tasks, &aperiodics, &instances);
    
    for(unsigned int i=0;i<instances.size();i++) {
    cout<<setw(5)<<i<<": Task:"<<setw(4)<<instances[i].task_id<<" arrival:"<<setw(7)<<instances[i].arrival;
    cout<<" deadline: "<<setw(7)<<instances[i].deadline<<" computations: "<<setw(7)<<instances[i].computation_time<<"\t"<<" Power: "<<instances[i].power<<endl;
    }
//    int_pointer=&tasks;

	ab_edf_schedule(&edf, &instances);
//	consolidate_schedule(&edf, &tasks);

	ab_compute_profile(&edf);
}


