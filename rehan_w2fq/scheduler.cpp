
#define WFQ_GRAN    ((float) W_INT)
#define AS_COMPU    ((float) 0.5)
#define AS_THERMU   ((float) 0.5)
#define AS_POWER    (AS_THERMU * beta * corrected_threshold / AS_COMPU)

#define AP 2 // Ratio by which aper task power is multiplied
#define NUM_APERIODICS 10
#define INTERARRIVAL_MAX 5
#define INTERARRIVAL_INC ((float) 0.1)
#define INTERARRIVAL_MIN 0
#define REPS 10
#define PERIODIC_HYPERPERIOD 10

#define HYPERPERIOD_MAX 1000
#define HYPERPERIOD_MIN 900
#define HYPERPERIOD_SCALE 100




#define INTERLEAVE  1
#define INTERLEAVE_GRANULARITY .01
#define CHOOSE_REAL_FIRST 1

#include "scheduler.h"
#include <tgmath.h>
#include <algorithm>
#include <iomanip>
#include <random>
#include <assert.h>
float beta_multi[CORE][CORE];
int running_tasks[NUM_PROCESSORS] = { -1, -1, -1, -1 };
long long current_timedd = -GRANULARITY;
vector<task>*int_pointer;
stringstream logfile;
float scheduler_runtime = 0;
struct timespec schedulestart, starttime, endtime;
int n_aperiodics = 5;

int S_COMP_CAP = 0;
double S_POWER = 0;
int S_PERIOD = 0;

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
void ab_wfq (vector <float_schedule> *wfq, vector <float_task> *periodic, vector <instance> *aperiodic, float start, float end);
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

/*
bool compare_deadline(instance a, instance b) {
    if (a.deadline < b.deadline) {
        return true;
    }
    return false;
}
*/
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
    
    if (aperiodics) {
        instances->insert(instances->end(), aperiodics->begin(), aperiodics->end());
    }
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

double s_last_deadline = 0;
int aper_task_id = 500;
void generate_aperiodics(vector<instance> *aperiodics, int arrival, int computation_time, double power) {
    double TTI = power * computation_time / beta;
    instance temp, tempi;
    double comp_time_remaining = computation_time;
    double S_THERM_CAP = ((S_COMP_CAP * S_POWER) / beta);
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
    }

    aper_task_id++;
    return;
}

double s_last_tbs = 0;

void generate_aperiodics_tbs(vector<instance> *aperiodics, int arrival, int computation_time, double power) {
    
    instance temp;

    temp.arrival    = arrival;
    temp.power      = power;
    temp.computation_time = computation_time;
    temp.deadline = max(arrival, s_last_tbs) + (computation_time * (((float) S_PERIOD) / S_COMP_CAP));
    temp.task_id = aper_task_id++;
    
    s_last_tbs = temp.deadline;
    
    if (temp.computation_time != 0) {
        aperiodics->push_back(temp);
    }

    return;
}

void generate_poisson(vector<instance> *aperiodics, vector<instance> *aperiodics_tbs, float mean_arrival, int num)
{
    aperiodics->clear();

    s_last_tbs = 0;
    s_last_deadline = 0;
    aper_task_id = 500;
    
    int n_tasks = num;
    float number;
    float power;
    float comp_time;
    instance temp;

    std::default_random_engine generator;
    std::poisson_distribution<int> distribution(mean_arrival);
    generator.seed(rand());

    float p[1000000]={};
    float last_arrival = 0;
    p[0] = 0;

    for (int i = 1; i < n_tasks; i++) {
        number = distribution(generator);
        //number = mean_arrival;
        p[i] = ((i == 0) ? 0 : p[i - 1]) + number;
        last_arrival = p[i];
    }
    
    /*
    for (int i=0; i<n_tasks; ++i) {
        cout << i << ":" << p[i]<<endl;
    }
    */
    for(int i=0; i<n_tasks; i++) {
        temp.task_id = aper_task_id++;
        temp.arrival = (float) p[i];
        temp.computation_time = 1;
        temp.power = AS_POWER * AP;
        aperiodics->push_back(temp);
    }
}

#if 0
int main(int argc, char* argv[]) {

    srand(time(NULL));
	corrected_threshold = corrected_temperature(THRESHOLD);

	int iter = 1, hyperperiod = 0, num_periodics = 10, i, j, k;
	string task_file;
    unsigned int v_ours = 0, v_tbs = 0;
    float t_util = 0.7;
    float c_util = 0.8;

	vector<float_task> periodic_tasks;
	vector<float_schedule> wfq, edf, edf_tbs, edf_periodic;
    vector<instance> aperiodics, aperiodics_tbs;
    vector<instance> instances, instances_tbs, instances_periodic;
    
    generate_periodic_taskset(&periodic_tasks, /*hyperperiod*/ 100, /*num_tasksets*/ 5, (1 - AS_COMPU), (1 - AS_THERMU));
    generate_poisson(&aperiodics, &aperiodics_tbs, 4, 1);

    ab_wfq (&wfq, &periodic_tasks, &aperiodics, /*start*/ 0, /*end*/ 10);
    ab_compute_profile(&wfq);
    
    exit(0);
    for (i = 0; i < 100; i++) {

        periodic_tasks.clear();
        edf.clear();
        edf_tbs.clear();
        edf_periodic.clear();
        aperiodics.clear();
        aperiodics_tbs.clear();
        instances.clear();
        instances_tbs.clear();
        instances_periodic.clear();
	    
        vector<task> periodic_tasks;
	    vector<float_schedule> edf, edf_tbs, edf_periodic;
        vector<instance> aperiodics, aperiodics_tbs;
        vector<instance> instances, instances_tbs, instances_periodic;
        
        hyperperiod = ((int) (rand() % (HYPERPERIOD_MAX + 1 - HYPERPERIOD_MIN) + HYPERPERIOD_MIN) / HYPERPERIOD_SCALE) * HYPERPERIOD_SCALE; 
        ab_generate_taskset(&periodic_tasks, hyperperiod, num_periodics, c_util, t_util);        
        tasks2instances(&periodic_tasks, NULL, &instances_periodic);
	    ab_edf_schedule(&edf_periodic, &instances_periodic);
        
	    corrected_threshold = corrected_temperature(THRESHOLD) - 5;
        if (ab_compute_profile(&edf_periodic)) {
            i--;
            continue;
        }

        S_COMP_CAP = (periodic_tasks)[0].computation_time;// / 2);
        S_POWER = (periodic_tasks)[0].power;// / 2);
        S_PERIOD = (periodic_tasks)[0].period;// / 2);

        if(!S_COMP_CAP || !S_POWER) {
            i--;
            continue;
        }
        //cout<<"Srvr Params: "<<endl<<S_COMP_CAP<<endl<<S_POWER<<endl<<S_PERIOD<<endl;
        periodic_tasks.erase(periodic_tasks.begin());    

        generate_poisson(&aperiodics, &aperiodics_tbs);


        tasks2instances(&periodic_tasks, &aperiodics, &instances);
        tasks2instances(&periodic_tasks, &aperiodics_tbs, &instances_tbs);
    
        for(unsigned int i=0;i<instances.size();i++) {
//            cout<<setw(5)<<i<<": Task:"<<setw(4)<<instances[i].task_id<<" arrival:"<<setw(7)<<instances[i].arrival;
//            cout<<" deadline: "<<setw(7)<<instances[i].deadline<<" computations: "<<setw(7)<<instances[i].computation_time<<"\t"<<" Power: "<<instances[i].power<<endl;
        }

	    ab_edf_schedule(&edf, &instances);
        ab_edf_schedule(&edf_tbs, &instances_tbs);
    
	    corrected_threshold = corrected_temperature(THRESHOLD) + 5;
        if (ab_compute_profile(&edf_tbs)) { 
            v_tbs++;
        } else {
            i--;
            continue;
        }
	    
        if (ab_compute_profile(&edf)) {
            v_ours++;
        }
	    //cout<<"Violations"<<endl<<"Ours: "<<v_ours<<" TBS: "<<v_tbs<<endl;
    }
	cout<<"Violations"<<endl<<"Ours: "<<v_ours<<" TBS: "<<v_tbs<<endl;
}
#endif



void ab_wfq (vector <float_schedule> *wfq, vector <float_task> *periodic, vector <instance> *aperiodic, float start, float end /*hyperperiod*/)
{
    wfq->clear();
    tasksets[0].hyperperiod = end - start;
    unsigned int i, j, k;
    long comp_cnt, idle_cnt;
    bool is_high_power;
    float arrival, deadline, sanity_arrival, power_ratio, idle_factor, cur_time;
    vector <instance> instances_blown;
    instance temp;
    float_schedule sched; 


    for (i = 0; i < periodic->size(); i++) {
  //      cout << (*periodic)[i].index << " C = " << (*periodic)[i].computation_time << " T = " << (*periodic)[i].period << " P = " << (*periodic)[i].power << endl;
    }
    
    // First blow up periodic tasks to granular instances
    for (i = 0; i < periodic->size(); i++) {
        arrival = start; // assumption: all periodics start at start
        for (j = 0; j < (int)((end - start) / (*periodic)[i].period); j++) {
            deadline = arrival + (*periodic)[i].period; 
            for (comp_cnt = (long) ((*periodic)[i].computation_time / WFQ_GRAN); comp_cnt > 0; comp_cnt--) {
                temp.task_id = (*periodic)[i].index;
                temp.computation_time = WFQ_GRAN;
                temp.power = (*periodic)[i].power;

                temp.arrival = arrival;
                temp.deadline = arrival + (((*periodic)[i].period / (*periodic)[i].computation_time) * WFQ_GRAN);
                arrival = temp.deadline;
                temp.is_last = (comp_cnt == 1);

                instances_blown.push_back(temp);

                //cout << "Task_id: " << temp.task_id << " Arrival: " << temp.arrival << " Deadline: " << temp.deadline << endl;
            }
            // arrival var is the deadline of the last granular instance
            assert(arrival <= deadline + 2 * WFQ_GRAN);        
            arrival = deadline;
        }
        assert(deadline <= end + 2 * WFQ_GRAN);
    }

    // Now blow up aperiodics while praying to THE ALGO
    for (i = 0; i < aperiodic->size(); i++) {
//        cout << (*aperiodic)[i].task_id << " C = " << (*aperiodic)[i].computation_time << " A = " << (*aperiodic)[i].arrival << " P = " << (*aperiodic)[i].power << endl;
    }

    for (i = 0; i < aperiodic->size(); i++) {
        is_high_power = (*aperiodic)[i].power > AS_POWER;
        power_ratio = is_high_power ? ((*aperiodic)[i].power / AS_POWER) : 1;
        idle_factor = is_high_power ? (power_ratio - 1) : 0;

        arrival = (i == 0) ? (*aperiodic)[i].arrival : max((*aperiodic)[i].arrival, (*aperiodic)[i - 1].deadline);
        deadline = (*aperiodic)[i].deadline = arrival + (((*aperiodic)[i].computation_time * power_ratio) / AS_COMPU);

        comp_cnt = (long) ((*aperiodic)[i].computation_time / WFQ_GRAN);

        while (comp_cnt > 0) {
            temp.power = (*aperiodic)[i].power;
            comp_cnt--;

            temp.task_id = (*aperiodic)[i].task_id;
            temp.computation_time = WFQ_GRAN;

            temp.arrival = arrival;
            temp.deadline = arrival + (WFQ_GRAN * power_ratio / AS_COMPU);
            arrival = temp.deadline;

            temp.is_last = (comp_cnt == 0);
            
            instances_blown.push_back(temp);

            //cout << "Task_id: " << temp.task_id << " Arrival: " << temp.arrival << " Deadline: " << temp.deadline << " Power : " << temp.power << endl;
        }

        assert(arrival <= (*aperiodic)[i].deadline + 2 * WFQ_GRAN);
    }


    // Now we have all periodic and aperiodics neatly blown up into granular instances.
    // All that is left is to create a schedule.
    // The code assumes that the instances are ordered as:
    //      Level 1: Deadlines
    //      Level 2: Arrival
    // So the first element is the one with earliest deadline and then earliest arrival.
    sort(instances_blown.begin(), instances_blown.end(), compare_deadline);
    
    cur_time = start;
    while (instances_blown.size() > 0) {
        
        
        for (i = 0; i < instances_blown.size(); i++) {
            // Run down the deadline sorted list to find the first eligible instance
            if (cur_time >= instances_blown[i].arrival) {
                break;
            }
        }
        

        if (i >= instances_blown.size()) { // Uh Oh, no eligible instance
            cur_time += WFQ_GRAN;
            continue; // This gets us back to while loop check, to make sure we break out when instances_blown list is empty
        }

        sched.task_id = instances_blown[i].task_id;
        sched.arrival = instances_blown[i].arrival;
        sched.power = instances_blown[i].power;
        sched.is_last = instances_blown[i].is_last;

        sched.start = cur_time;
        cur_time += WFQ_GRAN;
        sched.end = sched.start + WFQ_GRAN;

        wfq->push_back(sched);
        instances_blown.erase(instances_blown.begin() + i);

        //cout << "Task_id: " << sched.task_id << " Start: " << sched.start << " End: " << sched.end << endl;
    }

    // Insert some verifiers
    //
}

int main(int argc, char* argv[]) {

    srand(time(NULL));
	corrected_threshold = corrected_temperature(THRESHOLD);

	int i, j, k;
    int violations;
    double interarrival_time;

	vector<float_task> periodic_tasks;
	vector<float_schedule> wfq;
    vector<instance> aperiodics, aperiodics_tbs;

    float_task as_periodic_task;
    as_periodic_task.computation_time = 1;
    as_periodic_task.period = 1 / AS_COMPU;
    as_periodic_task.power = AS_POWER;
    as_periodic_task.index = 499;
    
    double response_time[(unsigned int) (INTERARRIVAL_MAX / INTERARRIVAL_INC)];
    for (i = 0; i < sizeof(response_time)/sizeof(double); i++) {
        response_time[i] = 0;
    }
    
    violations = 0;
    for (j = 0; j < REPS; j++) {
//        cout << "Interarrival mean = " << interarrival_time << endl;
        for (interarrival_time = INTERARRIVAL_MAX, i = 0; interarrival_time > INTERARRIVAL_MIN; interarrival_time -= INTERARRIVAL_INC, i++) {

            while (1) {
                generate_periodic_taskset(&periodic_tasks, /*hyperperiod*/ PERIODIC_HYPERPERIOD, /*num_tasksets*/ 2, (1 - AS_COMPU), (1 - AS_THERMU));
                periodic_tasks.push_back(as_periodic_task);
                aperiodics.clear();
                ab_wfq (&wfq, &periodic_tasks, &aperiodics, /*start*/ 0, /*end*/ PERIODIC_HYPERPERIOD);
                if (!ab_compute_profile(&wfq, true)) {
                    break;
                }
            }

            periodic_tasks.pop_back();

            generate_poisson(&aperiodics, &aperiodics_tbs, interarrival_time, NUM_APERIODICS);
            ab_wfq (&wfq, &periodic_tasks, &aperiodics, /*start*/ 0, /*end*/ (NUM_APERIODICS * 2) * INTERARRIVAL_MAX);
            
            // Evaluate Response Time
            for (k = 0; k < wfq.size(); k++) {
                if (wfq[k].is_last == true) {
                    if(wfq[k].task_id >= 500) {
                        response_time[i] += (double) (wfq[k].end - aperiodics[wfq[k].task_id - 500].arrival);
//                        cout << "Task " << (wfq[k].task_id - 500) << " Response time = " << response_time[i] << endl;
                    }
                }
            } 
            
            // Check for thermal violations
            if (ab_compute_profile(&wfq, false)) {
                cout << "Thermal violation!!!" << endl;
                violations++;
//                exit (1);
            }
        }
    }
    
    for (interarrival_time = INTERARRIVAL_MAX, i = 0; i  < sizeof(response_time) / sizeof(double) ; interarrival_time -= INTERARRIVAL_INC, i++) {
        response_time[i] = response_time[i] / (NUM_APERIODICS * REPS);
        cout << "Interarrival mean = " << interarrival_time << "\t" << " Response time = " << response_time[i] << endl; // Bhuvana, this is main output.
    }

    cout << "Violations : " << violations << endl;
}
