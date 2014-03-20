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

#if(MATLAB_ENABLE)
	ep =engOpen(NULL);
	if(ep==0)
	{
		cout<<"connection with matlab engine failed"<<endl;

	}
#endif

#if (RT_ENABLE)
	struct sched_param param;

	signal(SIGINT, int_handler);
	freq_set(CORE,"performance");
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(1,&mask);
	cout<<"setting affinity to processor "<<CORE<<endl;
	if(sched_setaffinity(0,CPU_COUNT(&mask),&mask)==-1)
	{
		perror("sched_setscheduler failed");
		exit(-1);
	}
	param.sched_priority = MY_PRIORITY; //priority for the scheduler
	if (sched_setscheduler(0, SCHED_FIFO, &param) == -1) { //scheduler has cooporative scheduling
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
#endif

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
		// generate_tasksets(&tasks,1,hyperperiod,50,100);
		if (argc == 6) {
//			generate_tasksets(&tasks, 1, hyperperiod, comp_util, comp_util);
		} else {
			read_tasksets(&tasks, task_file);
		}

		int_pointer=&tasks;


#if(OPT_ENABLE)
		call_gams();
		store_opt(&tasks);
#endif


		double t_util;
//		edf_schedule(&tasks, &edf);
//		consolidate_schedule(&edf, &tasks);

		thermal_optimal = 1;
//		compute_profile(&edf, &tasks, t_util);
//		edl_schedule2(&edl2, &edf);
//		consolidate_schedule(&edl2, &tasks);

//		populate_slacks(&slacks, &edf);
//		edl_schedule(&edl, &edf, &tasks, &slacks);
//		consolidate_schedule(&edl, &tasks);

#if(INST_ENABLE)

		cout<<"entering optimize instances"<<endl;
		vector<int>slack;
		optimize_instances(&i_speed,&tasks,&i_schedule,&edl, &edl2,MIN_SPEED,MAX_SPEED,&i_speed_temp,&slack);
		cout<<"exiting optimize instances"<<endl;

		cout<<"printing optimal schedule"<<endl;
		for(unsigned int i=0;i<i_schedule.size();i++)
		{
			cout<<"task "<<i_schedule[i].task_id<<" start "<<i_schedule[i].start<<" end "<<i_schedule[i].end
			<<" speed "<<i_speed[i]<<" power "<<tasks[i_schedule[i].task_id].power*pow(i_speed[i],3.0)<<
			" slack "<<slack[i]<<" deadline "<<(i_schedule[i].start/tasks[i_schedule[i].task_id].period+1)*tasks[i_schedule[i].task_id].period<<endl;
		}

		verify(&i_schedule,&tasks,&i_speed);
#endif

#if(SLACK_ENABLE)

		verify(&edl, &tasks);

		opt_schedule_exact(&opt_exact, &tasks);

		opt_schedule(&opt, &tasks, &edl);
#endif

#if(MAX_ENABLE)

		vector<float_task>ft;

		vector<long_task>ltasks;
		vector<long_schedule>lschedule;
		vector<long_schedule>lschedule2;
		vector<long_schedule>lschedule3;

		vector<float_schedule>wsch;
		vector<float_schedule>wsch2;
		vector<float_schedule>wsch3;
		vector<float_schedule>fedf;
		vector<schedule>o_sch;
		vector<interval_s>intervals;

	//	cout<<"generating task set "<<endl;

		float thermal_util=0.98;
		float computation_util=1.0;
		int num_tasks=rand() % (11) + 10;


//		generate_taskset(&ft,  1000, num_tasks,computation_util, thermal_util);
//		w2fq_task_convert(&ltasks, &ft);
//		generate_intervals_gps(&intervals, &ltasks);/
//		w2fq_schedule(&lschedule, &ltasks,&intervals,0);
//		edf_schedule(&ft,&fedf);

//		w2fq_schedule_convert(&wsch, &lschedule);
		//cout<<" printing float schedule "<<endl;
//		compute_profile(&wsch, &ft, 1);
//		compute_profile(&fedf, &ft, 4);


//		instance_override(&ft);

//		dynamic_instance_schedule(&lschedule2, &ltasks,&intervals,1);
//		w2fq_schedule_convert(&wsch2, &lschedule2);
//		compute_profile(&wsch2, &ft, 2);

//		dynamic_instance_schedule(&lschedule3, &ltasks,&intervals,2);
//		w2fq_schedule_convert(&wsch3, &lschedule3);
//		compute_profile(&wsch3, &ft, 3);


		ft.clear();
		ltasks.clear();
		lschedule.clear();
		fedf.clear();
		wsch.clear();

		float power;
		power=rand()/(float)RAND_MAX*20.00;
		power=power+18.00;

		cout<<power<<endl;

		float comp_util=1.5;//0.6;
		ofstream taskfile("taskset_file");
		int ** matrix;
		matrix = new int *[20];

		for(unsigned int i=0;i<20;i++)
		{
			matrix[i] =new int[20];
		}

		resource_matrix(matrix,20,0.5);


		stringstream command;

		generate_taskset_multi(&ft,  100, 15,2.0);
		gams_include(&ft, matrix);
		ofstream tasksetfile("taskset");
		for(unsigned int i=0;i<ft.size();i++)
		{
			tasksetfile<<"task"<<i<<"\t"<<ft[i].computation_time<<"\t"<<ft[i].period<<"\t"<<ft[i].power<<endl;
		}

		tasksetfile.close();

		vector<instance>inst;

		vector<interval>speed_intervals;

		vector<long_schedule>lsch;

		generate_intervals_multi(&speed_intervals, "instanceassign_speed.put");




		for(unsigned int i=0;i<speed_intervals.size();i++)
		{
			cout<<"start "<<speed_intervals[i].start<<" end "<<speed_intervals[i].end<<endl;
			for(unsigned int j=0;j<speed_intervals[i].get_size();j++)
			{
				cout<<"task "<<speed_intervals[i].exec[j].task<<"|"<<speed_intervals[i].exec[j].exec<<"|"<<speed_intervals[i].exec[j].core<<endl;
			}

		}


		//exit(1);

		instance_override_speed(&ft, &inst);
		w2fq_task_convert(&ltasks, &ft);

		dynamic_instance_schedule_speed(&lsch, &ltasks, &speed_intervals, &inst,0);

	//	vector<long_schedule>lsch2;
	//	dynamic_instance_schedule_speed(&lsch2, &ltasks, &speed_intervals, &inst,0);


		float scaled_power[CORE];

		double avg_power;

		generate_power_trace(&lsch, "power_profile_scaled", hyperperiod, scaled_power);
		multi_simulate("power_profile_scaled", "multi.flp",0,avg_power);

//		dynamic_instance_schedule


		exit(1);

		float comps[CORE];
		for(unsigned int i=0;i<speed_intervals.size();i++)
		{
			for(unsigned int j=0;j<CORE;j++)
			{
				comps[j]=0;
			}
			for(unsigned int j=0;j<speed_intervals[i].get_size();j++)
			{
				comps[speed_intervals[i].exec[j].core]=comps[speed_intervals[i].exec[j].core]+
						                                      speed_intervals[i].exec[j].exec;
			}

			cout<<speed_intervals[i].start<<"|"<<speed_intervals[i].end<<"|"<<comps[0]<<"|"<<comps[1]<<"|"<<comps[2]<<endl;

		}

		exit(1);

		float bins[BIN_LENGTH]={0.5,0.6,0.7,0.8,0.9,1.0,1.1};
		int bin_count[BIN_LENGTH]={0,0,0,0,0,0,0};

		int task_num=0;
		bool tutil_incomplete=true;
		while(comp_util<3.0)//1.0)
		{


			cout<<"generating for computil"<<comp_util<<endl;
			tutil_incomplete=true;

			for(unsigned  int b=0;b<BIN_LENGTH;b++)
			{
				bin_count[b]=0;
			}

			while(tutil_incomplete)
			{
				//cout<<" task num "<<task_num<<endl;
				//generate_taskset(&ft, 100, 10, comp_util);
				generate_taskset_multi(&ft,  100, 15,comp_util);
				float avg_power=0.00;
				float real_cutil=0.00;

				for(unsigned int i=0;i<ft.size();i++)
				{
					avg_power=avg_power+ft[i].computation_time/ft[i].period*ft[i].power;
					real_cutil=real_cutil+ft[i].computation_time/ft[i].period;
				}
				float tutil=avg_power/(beta*corrected_threshold);//UTIL_POWER;
				bool accept=false;
				for(unsigned int b=0;b<BIN_LENGTH;b++)
				{
					if(tutil> bins[b] && tutil<(bins[b]+0.1) && bin_count[b]<10)
					{
						accept=true;
						bin_count[b]=bin_count[b]+1;
					}
				}

				if(accept)
				{
					/*if(tutil<0.6 && comp_util>0.75)
					{
						ofstream tasksetfile("taskset");
						for(unsigned int i=0;i<ft.size();i++)
						{
							tasksetfile<<"task"<<i<<"\t"<<ft[i].computation_time<<"\t"<<ft[i].period<<"\t"<<ft[i].power<<endl;
						}

						taskfile<<real_cutil<<"\t"<<tutil<<endl;

						//w2fq_task_convert(&ltasks, &ft);
						//generate_intervals_gps(&intervals, &ltasks);
						//w2fq_schedule(&lschedule, &ltasks,&intervals,0);
						//edf_schedule(&ft,&fedf);
						//w2fq_schedule_convert(&wsch, &lschedule);
						//cout<<" printing float schedule "<<endl;
						//compute_profile(&wsch, &ft, 1);
						//compute_profile(&fedf, &ft, 4);

						//command<<"mkdir results_uni/"<<task_num<<";";
						//command<<"cp profile_float results_uni/"<<task_num<<"/.;";
						//command<<"cp profile_default results_uni/"<<task_num<<"/.;";
						//command<<"cp taskset results_uni/"<<task_num<<"/.";
						//system(command.str().c_str());
						//command.str("");
						tasksetfile.close();
						//exit(1);
					}*/
					//cout<<"checkpoint 1 "<<endl;
					gams_include(&ft, matrix);

					//cout<<"checkpoint 2 "<<endl;


					system("cd gams_files; ~rehan/gams/gams lower_bound.gms");
					system("cd gams_files; ~rehan/gams/gams task_assign.gms");

					command<<"mkdir results/"<<task_num<<";";
					command<<"cp gams_files/results.put results/"<<task_num<<"/.;";
					command<<"cp gams_files/taskassign.put results/"<<task_num<<"/.;";
					command<<"cp gams_files/taskassign_assign.put results/"<<task_num<<"/.;";
					command<<"cp gams_files/task_assign.lst results/"<<task_num<<"/.;";
					command<<"cp gams_files/lower_bound.lst results/"<<task_num<<"/.;";
					command<<"cp gams_files/taskset.put results/"<<task_num<<"/.";
					system(command.str().c_str());
					command.str("");

					//cin.get();
					//exit(1);
					task_num=task_num+1;
					tutil_incomplete=false;
					for(unsigned  int b=0;b<BIN_LENGTH;b++)
					{
						if(bin_count[b]<10)
						{
							tutil_incomplete=true;
						}
					}
					//exit(1);

				}
				intervals.clear();
				lschedule.clear();
				ltasks.clear();
				fedf.clear();
				wsch.clear();
				ft.clear();//only ft needs to be cleared for multicore simulation
			}

			//comp_util=comp_util+0.0025;//0.01;
			comp_util=comp_util+0.01;//0.01;
		}
		exit(1);


/*
		int task_num=0;
		while(comp_util<3.0)
		{

			for(unsigned int m=0;m<10;m++)
			{
				generate_taskset_multi(&ft,  100, 15,comp_util);
				float avg_power=0.00;
				float real_cutil=0.00;

				for(unsigned int i=0;i<ft.size();i++)
				{
					avg_power=avg_power+ft[i].computation_time/ft[i].period*ft[i].power;
					real_cutil=real_cutil+ft[i].computation_time/ft[i].period;
				}
				float tutil=avg_power/UTIL_POWER;

				if(tutil>0.5 && tutil<1.2)
				{
					taskfile<<real_cutil<<"\t"<<tutil<<endl;

					cout<<"checkpoint 1 "<<endl;
					gams_include(&ft, matrix);

					cout<<"checkpoint 2 "<<endl;
					system("cd gams_files; ~rehan/gams/gams lower_bound.gms");
					system("cd gams_files; ~rehan/gams/gams task_assign.gms");

					command<<"mkdir results/"<<task_num<<";";
					command<<"cp gams_files/results.put results/"<<atoi(argv[5])<<"/.;";
					command<<"cp gams_files/taskassign.put results/"<<atoi(argv[5])<<"/.;";
					command<<"cp gams_files/taskassign_assign.put results/"<<atoi(argv[5])<<"/.;";
					command<<"cp gams_files/task_assign.lst results/"<<atoi(argv[5])<<"/.;";
					command<<"cp gams_files/lower_bound.lst results/"<<atoi(argv[5])<<"/.;";
					command<<"cp gams_files/taskset.put results/"<<atoi(argv[5])<<"/.";
					system(command.str().c_str());
					command.str("");
					task_num=task_num+1;
				}
				ft.clear();
			}



			comp_util=comp_util+0.001;
		}
/*
		exit(1);

	//	generate_taskset_multi(&ft,  100, 8,2.5, power);
		//read_taskset_multi(&ft, "gams_folder/taskset.put");





		float cutil=0;
		float tutil=0;

		for(unsigned int i=0;i<ft.size();i++)
		{
			cout<<"task "<<i<<" computation time "<<ft[i].computation_time<<" period "
					<<ft[i].period<<" power "<<ft[i].power<<" comp util "<<ft[i].computation_time/ft[i].period
					<<" thermal util "<<ft[i].computation_time*ft[i].power/(ft[i].period*beta*corrected_threshold)<<endl;
			cutil=cutil+ft[i].computation_time/ft[i].period;
			tutil=tutil+ft[i].computation_time*ft[i].power/(ft[i].period*beta*corrected_threshold);
		}

		cout<<" computation utilization "<<cutil<<" thermal utilization "<<tutil<<endl;



		int int_size=gams_include(&ft, matrix);


		populate_beta();

		for(unsigned int i=0;i<CORE;i++)
		{
			for(unsigned int j=0;j<CORE;j++)
			{
				cout<<beta_multi[i][j]<<"\t";
			}
			cout<<endl;
		}







		system("cd gams_files; ~rehan/gams/gams task_assign.gms");//

		//system("cd gams_files; ~rehan/gams/gams instance_assign.gms");//




		//vector<trace>ttrace;


//		read_ttrace("hotspot_files/thermal_profile", &ttrace);


		intervals.clear();



		vector<mprofile>prof;

		generate_intervals_multi(&intervals,"gams_files/taskassign.put", &ft);
		vector<long_schedule>multi_sch;
		ltasks.clear();
		w2fq_task_convert(&ltasks, &ft);
		multi_schedule(&multi_sch,&intervals, &ltasks);
		long hyperperiod=compute_lcm(&ltasks);
		float avg_power[CORE];
		generate_power_trace(&multi_sch, "power_profile", hyperperiod, avg_power);

		generate_power_profile(&prof,&multi_sch, hyperperiod);

		cout<<"power profile length "<<prof.size()<<endl;

		double average_power;
		for(unsigned int i=0;i<ltasks.size();i++)
		{
			average_power=average_power+(((double)ltasks[i].computation_time)/((double)ltasks[i].period))*ltasks[i].power;
		}
		//compute_profile_multi(&multi_sch, &ltasks);

		//exit(1);

	//	multi_simulate("power_profile", "multi.flp",0,average_power);

		exit(1);

		multi_sch.clear();
		intervals.clear();
		generate_intervals_multi(&intervals,"gams_files/instanceassign.put", &ft);
		multi_schedule(&multi_sch,&intervals, &ltasks);
		generate_power_trace(&multi_sch, "power_profile", hyperperiod, avg_power);
		//multi_simulate("power_profile", "multi.flp",1);
	//	exit(1);




		system(command.str().c_str());


		for(unsigned int i=0;i<intervals.size();i++)
		{
			cout<<" start "<<intervals[i].start<<" end "<<intervals[i].end<<endl;
			for(unsigned int k=0;k<CORE;k++)
			{
				cout<<" core"<<k<<": ";
				int total=0;
				float average_power=0;
				for(unsigned int j=0;j<ft.size();j++)
				{
					cout<<intervals[i].computations[j][k]<<" ";
					total=total+intervals[i].computations[j][k];
					average_power=average_power+((float)intervals[i].computations[j][k])*ft[j].power/((float)(intervals[i].end-intervals[i].start));
				}
				assert(total<=(intervals[i].end-intervals[i].start));
				cout<<" total "<<total<<" average power "<<average_power<< endl;
			}
		}


		cout<<endl<<endl;

		for(unsigned int i=0;i<multi_sch.size();i++)
		{
		//	cout<<"task "<<multi_sch[i].task_id<<" start "<<multi_sch[i].start<<" end "<<multi_sch[i].end<<" core "<<multi_sch[i].core<<endl;
		}



		//avg_power=(float*)malloc(sizeof(float)*CORE);






		cout<<" printing average power"<<endl;
		for(unsigned int i=0;i<CORE;i++)
		{
			cout<<"core"<<i<<" average power "<<avg_power[i]<<endl;
		}

		total_comps(&multi_sch,&ltasks,&intervals);

	//	exit(1);



//		void w2fq_schedule(vector<long_schedule>*sch, vector<long_task>*tasks, vector<interval>*intervals, int core)

/*		while(computation_util<=1.0)
		{
			thermal_util=0.99;

			while(thermal_util<=1.0)
			{
				generate_taskset(&ft,  1000, computation_util, thermal_util);
				w2fq_task_convert(&ltasks, &ft);
				w2fq_schedule(&lschedule, &ltasks);
				edf_schedule(&ft,&fedf);

				w2fq_schedule_convert(&wsch, &lschedule);
				//cout<<" printing float schedule "<<endl;
				compute_profile(&wsch, &ft, 1);
				compute_profile(&fedf, &ft, 2);

				ft.clear();
				ltasks.clear();
				lschedule.clear();
				fedf.clear();
				wsch.clear();
				thermal_util=thermal_util+0.001;

			}
			computation_util=computation_util+0.01;

		}

*/


/*

//       t_util=optimize_maxfirst(&h_speed,&tasks,0.75,1.2);
		timespec start_time,end_time;
		clock_gettime(1,&start_time);
		t_util=optimize_maxmin(&h_speed,&tasks,MIN_SPEED,MAX_SPEED);
		clock_gettime(1,&end_time);
		scale(&scaled_max_first,&tasks,&h_speed);
		edf_schedule(&scaled_max_first,&edf_max_first);
		thermal_optimal=4;
		compute_profile(&edf_max_first, &scaled_max_first,t_util);

		float max_first_time=time_diff(&start_time,&end_time);


		slacks.clear();
		opt.clear();
		opt_exact.clear();
		edl.clear();


		populate_slacks(&slacks, &edf_max_first);
		edl_schedule(&edl, &edf_max_first, &scaled_max_first, &slacks);
		consolidate_schedule(&edl, &scaled_max_first);

		clock_gettime(1,&start_time);
		//opt_schedule(&opt, &scaled_max_first, &edl);
		clock_gettime(1,&end_time);

//		opt_schedule_exact(&opt_exact, &scaled_max_first);
	//	cout<<" schedule size "<<opt_exact.size()<<endl;
	//	run_schedule(&opt,&scaled_max_first);
	//	cout<<"TIME to find the optimal schedule "<<time_diff(&start_time,&end_time)<<"ms"<< " Maxfirst time" << max_first_time<<" Hyperperiod "<<tasksets[0].hyperperiod<<" tasks "<< tasks.size()<<endl;

		float max_speeds[tasks.size()];
		for(unsigned int i=0;i<tasks.size();i++)
		{
			max_speeds[i]=((float)tasks[i].computation_time)/((float)scaled_max_first[i].computation_time);
		}


		vector<schedule>o_sch2;
		//run_dynamic(&scaled_max_first,max_speeds);
		vector<instance>dyn_inst;
		dynamic_instances(&scaled_max_first,max_speeds ,&dyn_inst);

		vector<instance>dyn_inst2;
		for(unsigned int i=0;i<dyn_inst.size();i++)
		{
			dyn_inst2.push_back(dyn_inst[i]);
		}
		double tutil=0;

		scheduler(&o_sch,&scaled_max_first,&dyn_inst,max_speeds,sched_interval);
		compute_profile_dynamic(&o_sch, &tasks,tutil,"");




		scheduler2(&o_sch2,&scaled_max_first,&dyn_inst2, max_speeds, sched_interval);
		compute_profile_dynamic(&o_sch2, &tasks,tutil,"window");

/*		for(unsigned int i=0;i<o_sch2.size();i++)
		{
			cout<<"task "<<o_sch2[i].task_id<<" start "<<o_sch2[i].start<<" end "<<o_sch2[i].end<<" speed "<<o_sch2[i].speed<<endl;
		}
*/

#endif
#if(STATIC_ENABLE)
		t_util=optimize_static(&s_speed,&tasks,MIN_SPEED,MAX_SPEED);
		scale(&scaled_static,&tasks,&s_speed);
		edf_schedule(&scaled_static,&edf_static);
		thermal_optimal=5;
		compute_profile(&edf_static, &scaled_static,t_util);
#endif
#if(MATLAB_ENABLE)
		t_util=optimize_matlab(&m_speed,&tasks,MIN_SPEED,MAX_SPEED);
		scale(&scaled_matlab,&tasks,&m_speed);
		edf_schedule(&scaled_matlab,&edf_matlab);
		thermal_optimal=6;
		compute_profile(&edf_matlab, &scaled_matlab,t_util);
#endif
#if(NOCONS_ENABLE)
		speed_scale(&scaled_tasks,&speeds,&tasks,1.0);
		edf_schedule(&scaled_tasks,&edf2);
		thermal_optimal=7;
		compute_profile(&edf2,&scaled_tasks,t_util);
#endif

#if(SPEED_DEBUG)
		for(int i=0;i<tasks.size();i++)
		{
			cout<<"matlab: "<<m_speed[i]<<" max first:"<<h_speed[i]<<" static speed:<<"<<s_speed[i]<<" nocons speed:"<< speeds[i]<<endl;

		}

#endif

#if(ENABLE_PRINTS)
		cout<<"max first taskset"<<endl;
		for(int i=0;i<scaled_max_first.size();i++)
		{
			cout<<"task "<<i<<" computation time:"<<scaled_max_first[i].computation_time<<" period:"<<scaled_max_first[i].period<<" power:"<<scaled_max_first[i].power<<endl;
		}
		cout<<"static speed taskset"<<endl;
		for(int i=0;i<scaled_max_first.size();i++)
		{
			cout<<"task "<<i<<" computation time:"<<scaled_static[i].computation_time<<" period:"<<scaled_static[i].period<<" power:"<<scaled_static[i].power<<endl;
		}
#endif

#if(ENABLE_PRINTS)

		for(unsigned int i=0;i<speeds.size();i++)
		{
			cout<<"speed for task: "<<i<<"|"<<speeds[i]<<endl;
		}
#endif

		for (unsigned int i = 0; i < tasks.size(); i++) {

//			tasks[i].stat_stream->clear();
//			tasks[i].stat_stream->close();



			//        util1=util1+(float)tasks[i].computation_time/(float)tasks[i].period;
			//        util2=util2+(float)scaled_tasks[i].computation_time/(float)scaled_tasks[i].period;
			//        util3=util3+(float)discrete_scaled[i].computation_time/(float)discrete_scaled[i].period;
		}

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
		o_sch.clear();
	}
#if(MATLAB_ENABLE)
	engClose(ep);
#endif
}

