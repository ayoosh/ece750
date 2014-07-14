/*
 * util.cpp
 *
 *  Created on: Jun 16, 2013
 *      Author: rehan
 */

#include "scheduler.h"
//#include "intervals.h"
extern double beta;
extern double corrected_threshold;
extern vector<taskset>tasksets;
extern double global_sort_power;

float corrected_temperature(float temperature) {
	return (temperature * C - (C * (R * RAU + AMBIENT)) / (1 - R * DEL));
}

void factorise(vector<int>* factors, int big_number) {
	for (int i = 2; i <= big_number; i++) {
		if (big_number % i == 0) {
			factors->push_back(i);
		}
	}

	if (factors->size() == 1) {
		cout << "encountered a prime number" << endl;
	}
}

int gcd(int a, int b) {
	for (;;) {
		if (a == 0)
			return b;
		b %= a;
		if (b == 0)
			return a;
		a %= b;
	}
	return 1;
}

int lcm(int a, int b) {
	int temp = gcd(a, b);
	return temp ? (a / temp * b) : 0;
}

int compute_lcm(vector<task>* tasks, int set) {
	int ret_lcm = 1;
	for (unsigned int i = 0; i < tasks->size(); i++) {
		if ((*tasks)[i].taskset == set) {
			ret_lcm = lcm(ret_lcm, (*tasks)[i].period);
		}
	}
	return ret_lcm;
}

int compute_lcm(vector<float_task>* tasks) {
	int ret_lcm = 1;
	for (unsigned int i = 0; i < tasks->size(); i++) {
			ret_lcm = lcm(ret_lcm, (*tasks)[i].period);
	}
	return ret_lcm;
}

int compute_lcm(vector<long_task>* tasks) {
	int ret_lcm = 1;
	for (unsigned int i = 0; i < tasks->size(); i++) {
			ret_lcm = lcm(ret_lcm, (*tasks)[i].period);
	}
	return ret_lcm;
}



void store_taskset(vector<task>*tasks) {
	ofstream computation_file;
	ofstream period_file;
	ofstream power_file;
	computation_file.open("computation_time", std::ios::app);
	period_file.open("period", std::ios::app);
	power_file.open("power", std::ios::app);

	for (unsigned int i = 0; i < tasks->size(); i++) {
		computation_file << (*tasks)[i].computation_time << "\t";
		period_file << (*tasks)[i].period << "\t";
		power_file << (*tasks)[i].power << "\t";
	}
	computation_file << endl;
	period_file << endl;
	power_file << endl;

	computation_file.close();
	period_file.close();
	power_file.close();
}

void read_tasksets(vector<task>*tasks, string fname) {
	ifstream taskfile;
	taskfile.open(fname.c_str(), ios::in);
	string comp, period, power, init;
	taskfile >> init;

	task temp;
	temp.taskset = 0;
	temp.computations = 0;
	temp.index = 0;
	while (!taskfile.eof()) {
		taskfile >> temp.computation_time >> temp.period >> temp.power;
	//	 cout << temp.computation_time << "|" << temp.period << "|"
          //                              << temp.power << endl;

		if (temp.computation_time > 0 && temp.period > 0) {
			cout << temp.computation_time << "|" << temp.period << "|"
					<< temp.power << endl;
			tasks->push_back(temp);
		}
		temp.computation_time = 0;
	}
	cout << "exited loop" << endl;
	int hyp_length = compute_lcm(tasks, 0);
	float total_impact = 0;
	cout << "size" << tasks->size() << endl;
	for (unsigned int j = 0; j < tasks->size(); j++) {

		cout<<"task:"<<j<<"taskset:"<<(*tasks)[j].taskset<<" computation time:"<<(*tasks)[j].computation_time<<" period:"<<(*tasks)[j].period<< " Utilization: "<<(float)(*tasks)[j].computation_time/(float)(*tasks)[j].period<<endl;
		cout << (*tasks)[j].power << "|" << (*tasks)[j].computation_time << "|"
				<< (*tasks)[j].period << endl;
		total_impact = total_impact
				+ (*tasks)[j].power * (*tasks)[j].computation_time
						* (hyp_length / (*tasks)[j].period) / beta;
	}
	cout << "tti calculated" << endl;
	taskset temp_set;
	temp_set.TTI = total_impact / GRANULARITY;
	temp_set.t_util = total_impact / (corrected_threshold * (hyp_length));
	temp_set.average_power = total_impact / (hyp_length * beta);
	temp_set.hyperperiod = hyp_length;
	tasksets.push_back(temp_set);
	cout << "read successfully" << endl;

}

void read_tasksets(vector<float_task>*tasks) {
	float_task temp;
	for (unsigned int i=0;i<g_task_size;i++)
	{
		temp.index=global_tasks[i][0];
		temp.computation_time=global_tasks[i][1];
		temp.period=global_tasks[i][2];
		temp.power=global_tasks[i][3];
		tasks->push_back(temp);
	}


}


void generate_instances(vector<task>*tasks, char *fname) {
	vector<instance> inst;
	instance temp;
	for (unsigned int i = 0; i < tasks->size(); i++) {
		int start = 0;
		while (start < tasksets[0].hyperperiod) {
			temp.computation_time = (*tasks)[i].computation_time;
			temp.arrival = start;
			temp.deadline = start + (*tasks)[i].period;
			start = temp.deadline;
			temp.task_id = i;
			inst.push_back(temp);
		}
	}
	ofstream outfile;
	outfile.open(fname);
	outfile << "set T/1*" << tasksets[0].hyperperiod << "/;" << endl;
	outfile << "set I/1*" << inst.size() << "/;" << endl;
	outfile << "parameter computation(I)/";
	for (unsigned int i = 1; i <= inst.size(); i++) {
		if (i < inst.size())
			outfile << i << " " << inst[i - 1].computation_time << ",";
		else
			outfile << i << " " << inst[i - 1].computation_time << "/;" << endl;
	}

	outfile << "parameter arrival(I)/";
	for (unsigned int i = 1; i <= inst.size(); i++) {
		if (i < inst.size())
			outfile << i << " " << inst[i - 1].arrival << ",";
		else
			outfile << i << " " << inst[i - 1].arrival << "/;" << endl;

	}

	outfile << "parameter deadline(I)/";
	for (unsigned int i = 1; i <= inst.size(); i++) {
		if (i < inst.size())
			outfile << i << " " << inst[i - 1].deadline << ",";
		else
			outfile << i << " " << inst[i - 1].deadline << "/;" << endl;

	}

	outfile << "parameter Activity(I)/";
	for (unsigned int i = 1; i <= inst.size(); i++) {
		if (i < inst.size())
			outfile << i << " " << (*tasks)[inst[i - 1].task_id].power << ",";
		else
			outfile << i << " " << (*tasks)[inst[i - 1].task_id].power << "/;"
					<< endl;
	}
	inst.clear();
	outfile.close();

}

void generate_instances(vector<task>*tasks, vector<instance> *inst, float* speeds) {
	instance temp;
	for (unsigned int i = 0; i < tasks->size(); i++) {
		int start = 0;
		while (start < tasksets[0].hyperperiod) {
			temp.computation_time = (*tasks)[i].computation_time;
			temp.computations=temp.computation_time;
			temp.arrival = start;
			temp.deadline = start + (*tasks)[i].period;
			temp.task_id = i;
			temp.speed=speeds[temp.task_id];
			temp.power=(*tasks)[temp.task_id].power/pow(speeds[temp.task_id],3);
			start = temp.deadline;
			inst->push_back(temp);
		}
	}
}

void partial_instances(vector<task>*tasks,vector<instance>*inst, vector<instance>*r_inst,float* speeds,int start,int end)
{
	instance temp;
	int local_start;
	int index=0;

	if(r_inst->size()==0)
	{
		return;
	}

	while((*r_inst)[index].arrival<start)
	{

	//	cout<<"adding previous instances"<<endl;
		temp=(*r_inst)[index];
		temp.computation_time = round(((float)((*tasks)[(*r_inst)[index].task_id].computation_time))*(speeds[temp.task_id]));

		temp.computations=(temp.computation_time-(*r_inst)[index].comps_done)/temp.speed;

		//		temp.computations=temp.computations/speeds[(*r_inst)[index].task_id];
//		if((*r_inst)[index].task_id!=)
/*
		cout<<"debug code "<<endl;
		cout<<"id "<<(*r_inst)[index].task_id<<" computations time "<<(*tasks)[(*r_inst)[index].task_id].computation_time<<" speed "
				<<speeds[temp.task_id]<<" new speed "<<temp.speed<<" computat "<<(*r_inst)[index].computation_time<<"|"<<(*r_inst)[index].computations<<endl;
*/
		inst->push_back(temp);
		index=index+1;
		if(index>=r_inst->size())
		{
			break;
		}
	}
	for (unsigned int i = 0; i < tasks->size(); i++)
	{
		//cout<<" loop iteration for task "<<i<<endl;
		local_start=start%(*tasks)[i].period==0?start:(start/(*tasks)[i].period+1)*(*tasks)[i].period;
		while (local_start < end)
		{
		//	cout<<" local start "<<local_start<<endl;
			temp.task_id = i;
			temp.computation_time = round((*tasks)[i].computation_time*speeds[i]);
			temp.computations=(*tasks)[i].computation_time;
			temp.arrival = local_start;
			temp.deadline = local_start+ (*tasks)[i].period;
			temp.speed=speeds[temp.task_id];
			temp.power=(*tasks)[temp.task_id].power;
			local_start = temp.deadline;
			temp.task_id = i;
			inst->push_back(temp);
 		}
	}
}

void generate_tasksets(vector<task>* tasks, int num_tasksets, int hyperperiod,int min_util, int max_util)
{
//	cout << "entered generate tasksets" << endl;
	vector<int> factors;
	factorise(&factors, hyperperiod);

	task temp;
	for (int i = 0; i < num_tasksets; i++) {
		temp.taskset = i;
		int num_tasks = rand() % (11) + 10;
		
		//num_tasks=5;

		int utilization = 0;
		if ((max_util - min_util) > 0) {
			//utilization = 2*rand() % (max_util - min_util) + min_util;
            utilization = max_util;
		} else {
			utilization = min_util;
		}
        cout << "utilizatio = "<<utilization<< endl;
		for (int j = 0; j < num_tasks; j++) {
			temp.index = i;
			int index = rand() % factors.size();
			int task_utilization;
			temp.period = factors[index]; //* MULT_FACTOR;
			if (j < num_tasks - 1) {
				task_utilization = rand()
						% (utilization / ((num_tasks - j) / 2)
								- utilization / (2 * (num_tasks - j)))
						+ utilization / (2 * (num_tasks - j));

             //   task_utilization = utilization * ((( rand() % (num_tasks - j)) + 1) / (num_tasks - j));
                utilization = utilization - task_utilization;
			} else {
				task_utilization = utilization;
			}
            cout << "j:" << j << "util:" << task_utilization <<endl;
			temp.computation_time = temp.period * task_utilization / 100;
			//temp.stat_stream=new ifstream();
			temp.priority=20;
			temp.state=FINISHED;
			temp.core=CORE;
			if (temp.computation_time > 0) {
				tasks->push_back(temp);
			}
		}
	}

    cout << "tasks->size()="<<tasks->size()<<endl;

	for (unsigned int i = 0; i < tasks->size(); i++) {
		(*tasks)[i].tid = i;
	}

	taskset temp_set;
	for (int i = 0; i < num_tasksets; i++) {

		int hyp_length = compute_lcm(tasks, 0);
		float thermal_capacity = ((float) hyp_length) * corrected_threshold;
		int thermal_utilization = rand() % 51 + 50;
		float taskset_target_TTI = thermal_capacity
				* ((float) thermal_utilization) / 100;
		float average_power = taskset_target_TTI / hyp_length * 2; //*beta;//beta not added to reduce power

		int start = -1;
		int end;
		for (unsigned int j = 0; j < tasks->size(); j++) {
			if ((*tasks)[j].taskset == i) {
				end = j;
				if (start == -1) {
					start = j;
				}
			}
		}

		float total_util = 0;
		for (int j = start; j <= end; j++) {

			     //#if(ENABLE_PRINTS)
			cout << "task:" << j << "taskset:" << (*tasks)[j].taskset
					<< " computation time:" << (*tasks)[j].computation_time
					<< " period:" << (*tasks)[j].period << " Utilization: "
					<< (float) (*tasks)[j].computation_time
							/ (float) (*tasks)[j].period << endl;
			     //#endif
			total_util = total_util
					+ (float) (*tasks)[j].computation_time
							/ (float) (*tasks)[j].period;

		}
#if(ENABLE_PRINTS)

		cout<<"total_utilization="<<total_util<<endl;
#endif
		temp_set.c_util = total_util;
		average_power = average_power / total_util;
#if(ENABLE_PRINTS)

		cout<<"length of hyperperiod:"<<hyp_length<<endl;
		cout<<"thermal utilization: "<<thermal_utilization<<"average_power: "<<average_power<<endl;
#endif
		float deviation = average_power * 1.6;
		for (int j = start; j <= end; j++) {
			float power = average_power * 0.2
					+ ((double) (rand()) / RAND_MAX) * deviation;
			(*tasks)[j].power = power;
		}

		float total_impact = 0;
		for (int j = start; j < end; j++) {
			total_impact = total_impact
					+ (*tasks)[j].power * (*tasks)[j].computation_time
							* (hyp_length / (*tasks)[j].period) / beta;
		}
		temp_set.TTI = total_impact / GRANULARITY;
		temp_set.t_util = total_impact / (corrected_threshold * (hyp_length));
		temp_set.average_power = total_impact / hyp_length * beta;
		temp_set.hyperperiod = hyp_length;
		tasksets.push_back(temp_set);
#if(ENABLE_PRINTS)

		cout<<"required_impact "<<taskset_target_TTI<<" actual thermal impact: "<<total_impact<<endl;
#endif
	}
/*#if(OPT_ENABLE==1)
	generate_instances(tasks,"input");
#endif*/
//    exit(1);

//	cout << "taskset generated" << endl;
}

void resource_matrix(int ** matrix,int size, float likelyhood)
{
	for(int i=0;i<size;i++)
	{
		for(int j=0;j<size;j++)
		{
			if(i==j)
			{
				matrix[i][j]=0;
			}
			else
			{
				float decide=((float)rand())/RAND_MAX;
				if(decide<likelyhood)
				{
					matrix[i][j]=1;
				}
				else
				{
					matrix[i][j]=0;
				}
			}
		}
	}

}


void generate_taskset(vector<float_task> *tasks, long hyperperiod, int num_tasks, float comp_util, float thermal_util)
{
	vector<int> factors;
	factorise(&factors, hyperperiod);

	float_task temp;
//	int num_tasks =

//	cout<<" number of tasks "<<num_tasks<<endl;
	float sumU=comp_util;
	float t_sumU=thermal_util;

//	cout<<" initial sumU "<<sumU<<" t_sumU "<<t_sumU<<endl;
	float t_next_sumU;
	float next_sumU;

	bool violation=false;

	for (int i = 0; i < num_tasks; i++)
	{
		int index = rand() % factors.size();
		temp.period = factors[index];

		float cutil;
		if(i<num_tasks-1)
		{
			next_sumU=sumU*pow((float)(rand()/((float)(RAND_MAX))),1.0/((float)(num_tasks-(i+1))));
//			cout<<" sumu "<<sumU<<" next sumu "<<next_sumU<<endl;
			cutil=sumU-next_sumU;

		}
		else
		{
			cutil=sumU;
		}


		temp.computation_time=temp.period*(cutil);
		temp.computation_time=floor(temp.computation_time/(W_INT*temp.period))*W_INT*temp.period;
		sumU=sumU-(temp.computation_time/temp.period);


		if (temp.computation_time > 0)
		{
			tasks->push_back(temp);
		}
	}

	for(unsigned int i=0;i<tasks->size() && !violation;i++)
	{
		float tutil;
		float max_tutil=t_sumU;
		float min_tutil=t_sumU;
		if(i<num_tasks-1)
		{
			for(unsigned int j=i+1;j<tasks->size();j++)
			{
				max_tutil=max_tutil-MIN_POWER*(*tasks)[j].computation_time/(corrected_threshold*beta*(*tasks)[j].period);
				min_tutil=min_tutil-MAX_POWER*(*tasks)[j].computation_time/(corrected_threshold*beta*(*tasks)[j].period);
			}

			float local_max;
			float local_min;

			local_max=MAX_POWER*(*tasks)[i].computation_time/(corrected_threshold*beta*(*tasks)[i].period);
			local_min=MIN_POWER*(*tasks)[i].computation_time/(corrected_threshold*beta*(*tasks)[i].period);

			max_tutil=max_tutil>local_max?local_max:max_tutil;
			min_tutil=min_tutil<local_min?local_min:min_tutil;

			if(min_tutil>max_tutil || max_tutil<min_tutil)
			{
				cout<<" error detected min tutil "<<min_tutil<<" max util "<<max_tutil<<endl;
			}


	//		cout<<" index "<<i<<" num tasks "<<num_tasks<<" max "<<max_tutil<<" min "<<min_tutil<<endl;

			tutil=-1;

			int iteration=0;
			while((tutil<min_tutil || tutil>max_tutil) && !violation)
			{

				t_next_sumU=t_sumU*pow(rand()/((float)(RAND_MAX)),1.0/((float)(num_tasks-(i+1))));
				tutil=t_sumU-t_next_sumU;

				if(iteration>1000)
				{
					violation=true;
				}

				iteration=iteration+1;
			}
//			cout<<" exited while loop "<<endl;
		}
		else
		{
			tutil=t_sumU;
		}

		(*tasks)[i].power=corrected_threshold*beta * (*tasks)[i].period*tutil/(*tasks)[i].computation_time;
		(*tasks)[i].power=floor((*tasks)[i].power*10.0)/10.0;
		(*tasks)[i].power=(*tasks)[i].power>MAX_POWER?MAX_POWER:(*tasks)[i].power<MIN_POWER?MIN_POWER:(*tasks)[i].power;

		t_sumU=t_sumU-(*tasks)[i].computation_time*(*tasks)[i].power/(corrected_threshold*beta*(*tasks)[i].period);


		//cout<<"numerator "<<corrected_threshold*beta * temp.period*tutil<<endl;

	}


//	cout<<"checkpoint 1"<<endl;

//	for(unsigned int i=0;i<tasks->size();i++)
//	{
//		if((*tasks)[i].power<MIN_POWER || (*tasks)[i].power>MAX_POWER)
//		{
//			violation=true;
//			break;
//		}
//	}

	if(violation)
	{
		tasks->clear();
		generate_taskset(tasks, hyperperiod, num_tasks, comp_util, thermal_util);
	}

	for(unsigned int i =0;i<tasks->size();i++)
	{
		(*tasks)[i].index=i;
	}

}

void ab_generate_taskset(vector<float_task> *tasks, long hyperperiod, int num_tasks, float comp_util, float thermal_util)
{
    tasks->clear();
	vector<int> factors;
	factorise(&factors, hyperperiod);

	float_task temp;

	float sumU=comp_util;
	float t_sumU=thermal_util;

	float t_next_sumU;
    float next_sumU;
	bool violation=false;

	for (int i = 0; i < num_tasks; i++) {
		int index = rand() % factors.size();
		temp.period = factors[index];
        temp.taskset = 0;

		float cutil;
		if(i<num_tasks-1) {
			next_sumU=sumU*pow((float)(rand()/((float)(RAND_MAX))),1.0/((float)(num_tasks-(i+1))));
			cutil=sumU-next_sumU;

		} else {
			cutil=sumU;
		}

		temp.computation_time=temp.period*(cutil);
		temp.computation_time=floor(temp.computation_time/(W_INT*temp.period))*W_INT*temp.period;
		sumU=sumU-(temp.computation_time/temp.period);


		if (temp.computation_time > 0) {
			tasks->push_back(temp);
		}
	}

	for(unsigned int i=0;i<tasks->size() && !violation;i++) {
		float tutil;
		float max_tutil=t_sumU;
		float min_tutil=t_sumU;
		if(i<num_tasks-1) {
			for(unsigned int j=i+1;j<tasks->size();j++) {
				max_tutil=max_tutil-MIN_POWER*(*tasks)[j].computation_time/(corrected_threshold*beta*(*tasks)[j].period);
				min_tutil=min_tutil-MAX_POWER*(*tasks)[j].computation_time/(corrected_threshold*beta*(*tasks)[j].period);
			}

			float local_max;
			float local_min;

			local_max=MAX_POWER*(*tasks)[i].computation_time/(corrected_threshold*beta*(*tasks)[i].period);
			local_min=MIN_POWER*(*tasks)[i].computation_time/(corrected_threshold*beta*(*tasks)[i].period);

			max_tutil=max_tutil>local_max?local_max:max_tutil;
			min_tutil=min_tutil<local_min?local_min:min_tutil;

			if(min_tutil>max_tutil) {
				cout<<" error detected min tutil "<<min_tutil<<" max util "<<max_tutil<<endl;
			}

			tutil=-1;

			int iteration=0;
			while((tutil<min_tutil || tutil>max_tutil) && !violation)
			{

				t_next_sumU=t_sumU*pow(rand()/((float)(RAND_MAX)),1.0/((float)(num_tasks-(i+1))));
				tutil=t_sumU-t_next_sumU;

				if(iteration>1000)
				{
					violation=true;
				}

				iteration++;
			}
//			cout<<" exited while loop "<<endl;
		}
		else
		{
			tutil=t_sumU;
		}

		(*tasks)[i].power=corrected_threshold*beta * (*tasks)[i].period*tutil/(*tasks)[i].computation_time;
		(*tasks)[i].power=floor((*tasks)[i].power*10.0)/10.0;
		(*tasks)[i].power=(*tasks)[i].power>MAX_POWER?MAX_POWER:(*tasks)[i].power<MIN_POWER?MIN_POWER:(*tasks)[i].power;

		t_sumU=t_sumU-(*tasks)[i].computation_time*(*tasks)[i].power/(corrected_threshold*beta*(*tasks)[i].period);


		//cout<<"numerator "<<corrected_threshold*beta * temp.period*tutil<<endl;

	}


//	cout<<"checkpoint 1"<<endl;

//	for(unsigned int i=0;i<tasks->size();i++)
//	{
//		if((*tasks)[i].power<MIN_POWER || (*tasks)[i].power>MAX_POWER)
//		{
//			violation=true;
//			break;
//		}
//	}

//	if(violation)
//	{
//		tasks->clear();
//		ab_generate_taskset(tasks, hyperperiod, num_tasks, comp_util, thermal_util);
//	}

	for(unsigned int i =0;i<tasks->size();i++)
	{
		(*tasks)[i].index=i;
	}


	taskset temp_set;
    int num_tasksets = 1;
	for (int i = 0; i < num_tasksets; i++) {

		int hyp_length = hyperperiod;//compute_lcm(tasks, 0);
		float thermal_capacity = ((float) hyp_length) * corrected_threshold;
		//int thermal_utilization = rand() % 51 + 50;
        float thermal_utilization = thermal_util;
		float taskset_target_TTI = thermal_capacity
				* ((float) thermal_utilization) / 100;
		float average_power = taskset_target_TTI / hyp_length * 2; //*beta;//beta not added to reduce power

		int start = -1;
		int end;
		for (unsigned int j = 0; j < tasks->size(); j++) {
			if ((*tasks)[j].taskset == i) {
				end = j;
				if (start == -1) {
					start = j;
				}
			}
		}

		float total_util = 0;
		for (int j = start; j <= end; j++) {

			     #if(ENABLE_PRINTS)
			cout << "task:" << j << "taskset:" << (*tasks)[j].taskset
					<< " computation time:" << (*tasks)[j].computation_time
					<< " period:" << (*tasks)[j].period << " Utilization: "
					<< (float) (*tasks)[j].computation_time
							/ (float) (*tasks)[j].period << " power: "<< (*tasks)[j].power<<endl;
			     #endif
			total_util = total_util
					+ (float) (*tasks)[j].computation_time
							/ (float) (*tasks)[j].period;

		}
#if(ENABLE_PRINTS)

		cout<<"total_utilization="<<total_util<<endl;
#endif
		temp_set.c_util = total_util;
		average_power = average_power / total_util;
#if(ENABLE_PRINTS)

		cout<<"length of hyperperiod:"<<hyp_length<<endl;
		cout<<"thermal utilization: "<<thermal_utilization<<"average_power: "<<average_power<<endl;
#endif
/*		float deviation = average_power * 1.6;
		for (int j = start; j <= end; j++) {
			float power = average_power * 0.2
					+ ((double) (rand()) / RAND_MAX) * deviation;
			(*tasks)[j].power = power;
		}*/

		float total_impact = 0;
		for (int j = start; j < end; j++) {
			total_impact = total_impact
					+ (*tasks)[j].power * (*tasks)[j].computation_time
							* (hyp_length / (*tasks)[j].period) / beta;
		}
		temp_set.TTI = total_impact / GRANULARITY;
		temp_set.t_util = total_impact / (corrected_threshold * (hyp_length));
		temp_set.average_power = total_impact / hyp_length * beta;
		temp_set.hyperperiod = hyp_length;
		tasksets.push_back(temp_set);
    }

}

void generate_periodic_taskset(vector<float_task> *tasks, long hyperperiod, int num_tasks, float comp_util, float thermal_util)
{
    tasks->clear();
    
    vector<int> factors;
    factorise(&factors, hyperperiod);

    float_task temp;

    float sumU=comp_util;
    float t_sumU=thermal_util;

    float t_next_sumU = t_sumU;
    float next_sumU;
    bool violation=false;
    float total_util = 0, t_total_util = 0;

    for (int i = 0; i < num_tasks; i++) {
        int index = rand() % factors.size();
        temp.period = (float)factors[index];
        temp.taskset = 0;

        float cutil;
        float rnum;
retry:
        if(i<num_tasks-1) {
            rnum = rand() % 100;
            if(rnum < 50 || rnum >= 70) //Percentage should be not too low nor too high
                goto retry;
            next_sumU = sumU * (rnum / (float) (100));
            cutil = sumU - next_sumU; 
        }
        else {
            cutil = sumU;
        }

        temp.computation_time = temp.period * (cutil);
        temp.computation_time = floor(temp.computation_time /(W_INT*temp.period))*W_INT*temp.period;
        
        sumU = sumU - (temp.computation_time/(temp.period)); 

        if (temp.computation_time > 0) {
            //cout << "i = " << i << " C = " << temp.computation_time << " T = " << temp.period << " cutil = " << cutil   \
                << " sumU = " << sumU << endl;
            total_util += cutil;
            tasks->push_back(temp);
        }
        else {
           // cout << "Negative computation time " << endl;
        }
    }
    //Return the actual Periodic taskset utilization
//    cout << " Total Periodic Utilization = " << total_util << endl;
#if 1
	for(unsigned int i=0;i<tasks->size() && !violation;i++) {
		float tutil;
		float max_tutil=t_sumU;
		float min_tutil=t_sumU;
        float p_tutil;
		if(i<num_tasks-1) {
			for(unsigned int j=i+1;j<tasks->size();j++) {
				max_tutil=max_tutil-MIN_POWER*(*tasks)[j].computation_time/(corrected_threshold*beta*(*tasks)[j].period);
				min_tutil=min_tutil-MAX_POWER*(*tasks)[j].computation_time/(corrected_threshold*beta*(*tasks)[j].period);
			}
			float local_max;
			float local_min;

			local_max=MAX_POWER*(*tasks)[i].computation_time/(corrected_threshold*beta*(*tasks)[i].period);
			local_min=MIN_POWER*(*tasks)[i].computation_time/(corrected_threshold*beta*(*tasks)[i].period);

			max_tutil=max_tutil>local_max?local_max:max_tutil;
			min_tutil=min_tutil<local_min?local_min:min_tutil;

            //cout << "i = " << i << " min_tutil = " << min_tutil << " max_tutil = " << max_tutil;

			if(min_tutil>max_tutil) {
				cout<<" error detected min tutil "<<min_tutil<<" max util "<<max_tutil<<endl;
			}
			tutil=-1;

			int iteration=0;
            
            float min_ratio = (min_tutil / t_sumU) *  100;
            //cout << "Ratio should be atleast " << min_ratio << endl;
            float max_ratio = (max_tutil / t_sumU) * 100;
            //cout << "Ratio should not be more than " << max_ratio << endl;

            float randnum;
#if 0
t_retry:
            randnum = rand() % 100;
            //if((randnum < (min_ratio*1.5)) || (randnum > (max_ratio*0.5))) //|| (randnum < 5 && randnum > 50))
            if(randnum < min_ratio || randnum > max_ratio) {
                if(iteration < 100) {
                    iteration++;
                    goto t_retry;
                }
                else {
                    violation = true;
                    cout << "Failed to generate randnum " << endl;
                }

            }
            //cout  << "randnum = " << randnum << endl;
            if(!violation) {
            tutil = t_sumU * (randnum / 100);
            }
            else {
                tutil = t_sumU * max_ratio / 100;
            }
#endif
            //Assign tutil equally to all tasks.  Ignore the above calculation
            tutil = thermal_util/num_tasks;
        }
		else
		{
			//tutil=t_sumU;
            tutil = thermal_util/num_tasks;
		}

//        cout << "i = " << i << " tutil = " << tutil << " t_next_sumU = " << t_next_sumU << " t_sumU = " << t_sumU << endl;

        /*if((tutil<min_tutil || tutil>max_tutil) && (i < (num_tasks-1))) {
            cout << "Taskset Generation Violation !!! " << endl;
        }*/

		(*tasks)[i].power=corrected_threshold*beta * (*tasks)[i].period*tutil/(*tasks)[i].computation_time;
		(*tasks)[i].power=floor((*tasks)[i].power*10.0)/10.0;
		(*tasks)[i].power=(*tasks)[i].power>MAX_POWER?MAX_POWER:(*tasks)[i].power<MIN_POWER?MIN_POWER:(*tasks)[i].power;
        //MAX_POWER is too high. reduce it
        if((*tasks)[i].power == MAX_POWER)
            (*tasks)[i].power = MAX_POWER - 20;

//        cout << " power = " << (*tasks)[i].power;

		t_sumU=t_sumU-(*tasks)[i].computation_time*(*tasks)[i].power/(corrected_threshold*beta*(*tasks)[i].period);
        //t_sumU = t_sumU - tutil;
        t_next_sumU = t_sumU;
        t_total_util += tutil;

        //cout << " tutil = " << tutil /*<< " t_next_sumU = " << t_next_sumU << " t_sumU = " << t_sumU */<< endl;

		//cout<<"numerator "<<corrected_threshold*beta * temp.period*tutil<<endl;

	}

//    cout << "Total Thermal Utilization = " << t_total_util << endl;
    
/* Setup Parameters for taskset */    
    for(unsigned int i =0;i<tasks->size();i++)
	{
		(*tasks)[i].index=i;
	}

    taskset temp_set;
    int num_tasksets = 1;
	for (int i = 0; i < num_tasksets; i++) {

		//int hyp_length = compute_lcm(tasks, 0);
		float thermal_capacity = ((float) hyperperiod) * corrected_threshold;
		//int thermal_utilization = rand() % 51 + 50;
		float taskset_target_TTI = thermal_capacity
				* ((float)thermal_util ) / 100;
		float average_power = taskset_target_TTI / hyperperiod * 2; //*beta;//beta not added to reduce power

        int start = -1;
		int end;
		for (unsigned int j = 0; j < tasks->size(); j++) {
			if ((*tasks)[j].taskset == i) {
				end = j;
				if (start == -1) {
					start = j;
				}
			}
		}
        
        temp_set.c_util = total_util;
		average_power = average_power / total_util;
        
        float total_impact = 0;
        for (int j = start; j < end; j++) {
			total_impact = total_impact
					    + (*tasks)[j].power * (*tasks)[j].computation_time
					    * (hyperperiod / (*tasks)[j].period) / beta;
		    //cout << " i = " << j << " total_impact = " << total_impact << endl;
        }

        temp_set.TTI = total_impact / GRANULARITY;
		temp_set.t_util = total_impact / (corrected_threshold * hyperperiod);
		temp_set.average_power = total_impact / hyperperiod * beta;
		temp_set.hyperperiod = hyperperiod*10;
		tasksets.push_back(temp_set);

  //      cout << "Taskset " << i << " TTI = " << temp_set.TTI << " t_util = " << temp_set.t_util << " average_power = " << temp_set.average_power << " hyperperiod = " << temp_set.hyperperiod << " total_impact = " << total_impact << " c_util = " << temp_set.c_util << endl;
    }
#endif

}

void generate_taskset(vector<float_task> *tasks, long hyperperiod, int num_tasks, float comp_util)
{
	vector<int> factors;
	factorise(&factors, hyperperiod);

	float_task temp;
//	int num_tasks =

//	cout<<" number of tasks "<<num_tasks<<endl;
	float sumU=comp_util;

//	cout<<" initial sumU "<<sumU<<" t_sumU "<<t_sumU<<endl;
	float t_next_sumU;
	float next_sumU;

	bool violation=false;

	for (int i = 0; i < num_tasks; i++)
	{
		int index = rand() % factors.size();
		temp.period = factors[index];

		float cutil;
		if(i<num_tasks-1)
		{
			next_sumU=sumU*pow((float)(rand()/((float)(RAND_MAX))),1.0/((float)(num_tasks-(i+1))));
//			cout<<" sumu "<<sumU<<" next sumu "<<next_sumU<<endl;
			cutil=sumU-next_sumU;

		}
		else
		{
			cutil=sumU;
		}


		temp.computation_time=temp.period*(cutil);
		temp.computation_time=floor(temp.computation_time/(W_INT_HIGH*temp.period))*W_INT_HIGH*temp.period;
		sumU=sumU-(temp.computation_time/temp.period);


		if (temp.computation_time > 0)
		{
			tasks->push_back(temp);
		}
	}

	for(unsigned int i=0;i<tasks->size() && !violation;i++)
	{
		(*tasks)[i].power=((double)rand())/RAND_MAX*(MAX_POWER-MIN_POWER) +MIN_POWER;
	}


	for(unsigned int i =0;i<tasks->size();i++)
	{
		(*tasks)[i].index=i;
	}

}

void generate_taskset(vector<task> *tasks, long hyperperiod, int num_tasks, float comp_util)
{
	vector<int> factors;
	factorise(&factors, hyperperiod);

	task temp;

	float sumU=comp_util;

//	cout<<" initial sumU "<<sumU<<" t_sumU "<<t_sumU<<endl;
	float t_next_sumU;
	float next_sumU;

	bool violation=false;

	for (int i = 0; i < num_tasks; i++)
	{
		int index = rand() % factors.size();
		temp.period = factors[index];

		float cutil;
		if(i<num_tasks-1)
		{
			next_sumU=sumU*pow((float)(rand()/((float)(RAND_MAX))),1.0/((float)(num_tasks-(i+1))));
//			cout<<" sumu "<<sumU<<" next sumu "<<next_sumU<<endl;
			cutil=sumU-next_sumU;

		}
		else
		{
			cutil=sumU;
		}


		temp.computation_time=temp.period*(cutil);
		temp.computation_time=floor(temp.computation_time/(W_INT_HIGH*temp.period))*W_INT_HIGH*temp.period;
		sumU=sumU-(temp.computation_time/temp.period);


		if (temp.computation_time > 0)
		{
			tasks->push_back(temp);
		}
	}

	for(unsigned int i=0;i<tasks->size() && !violation;i++)
	{
		(*tasks)[i].power=((double)rand())/RAND_MAX*(MAX_POWER-MIN_POWER) +MIN_POWER;
	}


	for(unsigned int i =0;i<tasks->size();i++)
	{
		(*tasks)[i].index=i;
        cout << "task:" << i << " computation time:" << (*tasks)[i].computation_time
               << " period:" << (*tasks)[i].period << " Utilization: "
               << (float) (*tasks)[i].computation_time
               / (float) (*tasks)[i].period << "Power:" << (*tasks)[i].power << endl;

	}

}

bool ascending(int a, int b) {
	return (a < b);
}

bool float_ascending(float a, float b) {
	return (a < b);
}


void imp_times(vector<task> * tasks, vector<int>*times) {
	int hyperperiod = compute_lcm(tasks, (*tasks)[0].taskset);

	for (unsigned int i = 0; i < tasks->size(); i++) {
		int start = 0;
		while (start <= hyperperiod) {
			bool found = false;
			for (unsigned int j = 0; j < times->size(); j++) {
				if ((*times)[j] == start) {
					found = true;
					break;
				}
			}
			if (!found) {
				times->push_back(start);
			}
			start = start + (*tasks)[i].period;
		}
	}
	sort(times->begin(), times->end(), ascending);

}


void imp_times(vector<long_task> * tasks, vector<long>*times) {
	int hyperperiod = compute_lcm(tasks);

	for (unsigned int i = 0; i < tasks->size(); i++) {
		int start = 0;
		while (start <= hyperperiod) {
			bool found = false;
			for (unsigned int j = 0; j < times->size(); j++) {
				if ((*times)[j] == start) {
					found = true;
					break;
				}
			}
			if (!found) {
				times->push_back(start);
			}
			start = start + (*tasks)[i].period;
		}
	}
	sort(times->begin(), times->end(), ascending);
}



void imp_times(vector<float_task> * tasks, vector<float>*times) {
	float hyperperiod = compute_lcm(tasks);

	for (unsigned int i = 0; i < tasks->size(); i++) {
		float start = 0;
		while (start <= hyperperiod) {
			bool found = false;
			for (unsigned int j = 0; j < times->size(); j++) {
				if ((*times)[j] == start) {
					found = true;
					break;
				}
			}
			if (!found) {
				times->push_back(start);
			}
			start = start + (*tasks)[i].period;
		}
	}
	sort(times->begin(), times->end(), float_ascending);

}



void imp_times_partial(vector<task> * tasks, int * computations,
		vector<int>*times) {
	int hyperperiod = compute_lcm(tasks, (*tasks)[0].taskset);
//    times->push_back(start_time);
	for (unsigned int i = 0; i < tasks->size(); i++) {
		int start = 0;
		while (start <= hyperperiod) {
			bool found = false;
			for (unsigned int j = 0; j < times->size(); j++) {
				if ((*times)[j] == start) {
					found = true;
					break;
				}
			}
			if (!found) {
				if (computations[i]
						<= (start / ((*tasks)[i].period))
								* (*tasks)[i].computation_time) {
					times->push_back(start);
				} else if (computations[i]
						< (start / ((*tasks)[i].period) + 1)
								* (*tasks)[i].computation_time) {
					times->push_back(
							start + computations[i]
									- (start / ((*tasks)[i].period))
											* (*tasks)[i].computation_time);
				}
			}
			start = ((start / (*tasks)[i].period) + 1) * (*tasks)[i].period;
		}
	}
	sort(times->begin(), times->end(), ascending);

}


int min_deadline(vector<task>*tasks, int start) {
	int min_deadline = INT_MAX;
	int task_id = -1;
	int deadline = -1;

	for (unsigned int i = 0; i < tasks->size(); i++) {
		deadline = (start / (*tasks)[i].period) * (*tasks)[i].period
				+ (*tasks)[i].period;
		if (start >= (*tasks)[i].next_start && deadline < min_deadline) {
			task_id = i;
			min_deadline = deadline;
		}
	}

// cout<<"returning value "<< task_id<<endl;

	return task_id;
}

int min_deadline(vector<float_task>*tasks, float start) {
	float min_deadline = INT_MAX;
	int task_id = -1;
	float deadline = -1;

	for (unsigned int i = 0; i < tasks->size(); i++) {
		deadline = floor((start / (*tasks)[i].period)) * (*tasks)[i].period + (*tasks)[i].period;
		if (start >= (*tasks)[i].next_start && deadline < min_deadline) {
			task_id = i;
			min_deadline = deadline;
		}
	}

// cout<<"returning value "<< task_id<<endl;
	return task_id;
}


int min_deadline(vector<task>*tasks, int start, vector<int>*times,
		int *time_end) {
	int earliest_deadline = INT_MAX;
	int task_id = -1;
	int deadline = -1;

	for (unsigned int i = 0; i < tasks->size(); i++) {
		deadline = (start / (*tasks)[i].period) * (*tasks)[i].period
				+ (*tasks)[i].period;
		if (start >= (*tasks)[i].next_start && deadline < earliest_deadline) {
			task_id = i;
			earliest_deadline = deadline;
		}
	}

	*time_end = earliest_deadline;
	for (unsigned int i = 0; i < times->size(); i++) {
		if ((*times)[i] > start && (*times)[i] < earliest_deadline) {
			int newtask = min_deadline(tasks, (*times)[i]);

//    		cout<<"previous task "<<task_id<<" new task "<<newtask<<" start "<<start<<" limit "<<(*times)[i]<<endl;
			if (newtask != task_id) {
				*time_end = (*times)[i];
				break;
			}
		}
	}
// cout<<"returning value "<< task_id<<endl;
	return task_id;
}

double global_power(vector<task>*tasks, int start, int end, float util) {
	double g_power = 0.0;
	//float sum=0;

	for (int i = start; i < end; i++) {
		g_power = g_power
				+ (pow((*tasks)[i].power, 1 / 3.0)
						* (((double) (*tasks)[i].computation_time))
						/ ((double) (*tasks)[i].period));
	}

#if(PRINT_TASKSET)
	for(unsigned int i=0;i<tasks->size();i++)
	{
		cout<<"task\t"<<i<<" computation time\t"<<(*tasks)[i].computation_time<<"\t period\t"<<(*tasks)[i].period<<"\tpower\t"<<(*tasks)[i].power<<"\t|\t"<<pow((*tasks)[i].power,3)<<endl;
	}
	cout<<"optimal power"<<pow(g_power,3)<<endl;
	exit(1);
#endif
	return (pow(g_power / util, 3));

}

double global_power_partial(vector<task>*tasks, vector<int>*decisions,
		float util) {
	double g_power = 0.0;
	//float sum=0;

	bool found;
	for (unsigned int i = 0; i < tasks->size(); i++) {
		found = false;
		for (unsigned int j = 0; j < decisions->size(); j++) {

			if (i == (*decisions)[j]) {
				found = true;
			}
		}
		if (!found) {
			g_power = g_power
					+ (pow((*tasks)[i].power, 1 / 3.0)
							* (((double) (*tasks)[i].computation_time))
							/ ((double) (*tasks)[i].period));

		}
	}

#if(PRINT_TASKSET)
	for(unsigned int i=0;i<tasks->size();i++)
	{
		cout<<"task\t"<<i<<" computation time\t"<<(*tasks)[i].computation_time<<"\t period\t"<<(*tasks)[i].period<<"\tpower\t"<<(*tasks)[i].power<<"\t|\t"<<pow((*tasks)[i].power,3)<<endl;
	}
	cout<<"optimal power"<<pow(g_power,3)<<endl;
	exit(1);
#endif
	return (pow(g_power / util, 3));

}



bool sort_power(task a, task b) {
	return (fabs(a.power - global_sort_power)
			> fabs(b.power - global_sort_power));
}


int myfloor(float f)
{
	double frac,integer;
	frac = modf (f , &integer);
	int ret=frac>0.99?((int)ceil(f)):((int)floor(f));
	return ret;
//	return ((end->tv_sec-start->tv_sec)*1000.0000000 + ((float)(end->tv_nsec-start->tv_nsec))/1000000.000000);
}

int myceil(float f)
{
	double frac,integer;
	frac = modf (f , &integer);
	int ret=frac<0.01?((int)floor(f)):((int)ceil(f));
	return ret;
//	return ((end->tv_sec-start->tv_sec)*1000.0000000 + ((float)(end->tv_nsec-start->tv_nsec))/1000000.000000);
}

void write_to_file(string fname, string *str)
{
	ofstream outfile;
	outfile.open(fname.c_str());
	outfile<<(*str);
	outfile.close();
}

void split(string *text, vector<string> *delimiters, vector<string>*elem)
{
	size_t pos=string::npos;

	size_t pos_temp;
	int id=0;
	bool terminate=false;
	int start=0;
	while (!terminate)
	{
		pos=string::npos;
		for(unsigned int i=0;i<delimiters->size();i++)
		{
			pos_temp=(text->substr(start,text->length())).find((*delimiters)[i].c_str());
			pos=pos>pos_temp?pos_temp:pos;
			id=pos>pos_temp?i:id;

		//	cout<<" delimiter search start: "<<start<<" length: "<<pos<<" npos "<<string::npos<<endl;
		}
		if(pos==string::npos)
		{
		//	cout<<"terminating"<<endl;
			terminate=true;
		}
		else
		{
		//	cout<<" string added "<<text->substr(start,pos)<<endl;
			if (pos>0)
			{
				elem->push_back(text->substr(start,pos));
			}
			//cout<<(*elem)[elem->size()-1];
			//cout<<elem->size()<<"|"<<start<<"|"<<pos<<"|"<<text->length()<<endl;
			start=start+pos+(*delimiters)[id].size();
			//cin.get();
		}
	}
	if(start<text->length())
	elem->push_back(text->substr(start,text->length()-start));
}



