/*
 * scheduler.cpp
 *
 *  Created on: May 21, 2013
 *      Author: rehan
 */
/*
 * scheduler3.cpp
 *
 *  Created on: May 19, 2013
 *      Author: rehan
 */
//rt includes
#include "scheduler.h"

//#include "config.h"
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
 //float beta_multi[CORE][CORE];
//****************************************************************

int main(int argc, char* argv[]) {


	cout<<"beta ="<<beta<<endl;
	if (argc < 6) {
		cout
				<< "invalid format: <tasksets><hyperperiod length><computation utilization><interval> <seed>"
				<< endl;
		exit(1);
	}

	corrected_threshold = corrected_temperature(THRESHOLD);

	cout<<"beta "<<beta<<" corrected threshold "<<corrected_threshold<<endl;
//	exit(1);

	int iter = atoi(argv[1]);
	int hyperperiod = atoi(argv[2]);

	int comp_util = atoi(argv[3]);
	int sched_interval=atoi(argv[4]);
	seed = atoi(argv[5]);

	string task_file;
	if (argc == 7) {
		task_file = argv[6];
	}
	srand(seed);

	vector<task> tasks;
	vector<schedule> edf;
	vector<task> scaled_tasks;
	vector<double> speeds;
	vector<schedule> edf2;
	vector<task> discrete_scaled;
	vector<schedule> edf3;
	vector<schedule> edl2;
	vector<schedule> i_schedule;

	vector<schedule> opt;
	vector<schedule> opt_exact;

	vector<float> possible_speeds;

	vector<double> h_speed;
	vector<double> s_speed;

	vector<double> m_speed;

	vector<double> i_speed;
	vector<double> i_speed_temp;

	vector<slack> slacks;
	vector<schedule> edl;

	vector<task> scaled_max_first;
	vector<task> scaled_static;
	vector<task> scaled_matlab;
	vector<schedule> edf_max_first;
	vector<schedule> edf_static;
	vector<schedule> edf_matlab;

	for (int z = 0; z < iter; z++) {
		thermal_optimal = 0;
//		 generate_tasksets(&tasks,1,hyperperiod,50,100);
		if (argc == 6) {
			generate_tasksets(&tasks,1,hyperperiod,50,100);
		//	generate_tasksets(&tasks, 1, hyperperiod, comp_util, comp_util);
		} else {
			read_tasksets(&tasks, task_file);
		}

		int_pointer=&tasks;


#if(OPT_ENABLE)
		call_gams();
		store_opt(&tasks);
#endif


		double t_util;
		edf_schedule(&tasks, &edf);
		consolidate_schedule(&edf, &tasks);

		thermal_optimal = 1;
		compute_profile(&edf, &tasks, t_util);
//		edl_schedule2(&edl2, &edf);
//		consolidate_schedule(&edl2, &tasks);

//		populate_slacks(&slacks, &edf);
//		edl_schedule(&edl, &edf, &tasks, &slacks);
//		consolidate_schedule(&edl, &tasks);



//		for (unsigned int i = 0; i < tasks.size(); i++) {

//			tasks[i].stat_stream->clear();
//			tasks[i].stat_stream->close();



			//        util1=util1+(float)tasks[i].computation_time/(float)tasks[i].period;
			//        util2=util2+(float)scaled_tasks[i].computation_time/(float)scaled_tasks[i].period;
			//        util3=util3+(float)discrete_scaled[i].computation_time/(float)discrete_scaled[i].period;
//		}

#if(ENABLE_PRINTS)

		//  cout<<"util "<<util1<<"|"<<util2<<"|"<<util3<<endl;
		// cout<<"globally optimal power"<<g_power<<endl;
		//cout<<"computed thermally optimal schedule"<<endl;
#endif

		tasks.clear();
		edf.clear();
		scaled_tasks.clear();
		speeds.clear();
		edf2.clear();
		discrete_scaled.clear();
		edf3.clear();
		possible_speeds.clear();
		tasksets.clear();

		opt.clear();
		possible_speeds.clear();
		slacks.clear();

		h_speed.clear();
		s_speed.clear();
		m_speed.clear();
		i_speed.clear();
		i_speed_temp.clear();

		scaled_max_first.clear();
		scaled_static.clear();
		scaled_matlab.clear();
		edf_max_first.clear();
		edf_static.clear();
		edf_matlab.clear();
		edl2.clear();
		edl.clear();
		i_schedule.clear();
		//o_sch.clear();
	}
#if(MATLAB_ENABLE)
	engClose(ep);
#endif
}

