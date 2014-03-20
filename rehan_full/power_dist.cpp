/*
 * power_dist.cpp
 *
 *  Created on: Jun 16, 2013
 *      Author: rehan
 */

#include "scheduler.h"

extern double beta;
extern double corrected_threshold;
extern int seed;
extern int thermal_optimal;
extern vector<taskset>tasksets;

void possibles(vector<int>*possible_tasks, int start, vector<task>*tasks) {
	possible_tasks->clear();
	possible_tasks->push_back(-1);
	for (unsigned int i = 0; i < tasks->size(); i++) {
		if (start >= (*tasks)[i].next_start
				&& (*tasks)[i].computations < (*tasks)[i].computation_time) {
			possible_tasks->push_back(i);
		}
	}
}

float min_deviation(int *id, vector<int>*possible_tasks, vector<task>*tasks,
	float initial_temp, float target_temp) {

	assert(possible_tasks->size()>0);
	(*id) = -1;
	double temperature;
	double deviation = 10000000000.00;
	for (unsigned int i = 0; i < possible_tasks->size(); i++) {
		double task_temp;
		if((*possible_tasks)[i]>=0)
		{
			task_temp=heat(initial_temp,(*tasks)[(*possible_tasks)[i]].power, 1);
		}
		else
		{
			assert((*possible_tasks)[i]==-1);
			task_temp=cool(initial_temp, 1);
		}
		if (fabs(target_temp-task_temp)<= deviation) {
			(*id) = (*possible_tasks)[i];
			temperature = task_temp;
			deviation = fabs(target_temp - temperature);
		}
	}

	return temperature;
}


float mlt(int *id, vector<int>*possible_tasks, vector<task>*tasks,
		float initial_temp, float max_temp) {

	assert(possible_tasks->size()>0);
	(*id) = -2;
	double temperature = corrected_threshold*10;
	double deviation = fabs(max_temp - temperature);
	for (unsigned int i = 0; i < possible_tasks->size(); i++) {
		double task_temp;
		if((*possible_tasks)[i]>=0)
		{
			task_temp=heat(initial_temp,(*tasks)[(*possible_tasks)[i]].power, 1);
		}
		else
		{
			assert((*possible_tasks)[i]==-1);
			task_temp=cool(initial_temp, 1);
		}

		if (fabs(max_temp-task_temp)<= deviation && (max_temp-task_temp)>0) {
			(*id) = (*possible_tasks)[i];
			temperature = task_temp;
			deviation = fabs(max_temp - temperature);
		}
	}

	if((*id)==-2)
	{
		temperature= min_deviation(id, possible_tasks, tasks, initial_temp, max_temp);
	}

	return temperature;
}

void opt_schedule_exact(vector<schedule>*sch, vector<task> * tasks) {
	schedule tmp;
	clock_t start_t, end_t;
	start_t = clock();
	float TTI=0;
	tmp.start=0;
	tmp.end=0;
	tmp.task_id=-1;
	for(unsigned int i=0;i<tasks->size();i++)
	{
		TTI = TTI + (*tasks)[i].power * (*tasks)[i].computation_time
								* (tasksets[0].hyperperiod / (*tasks)[i].period) / beta;
			}

	float initial_temp = TTI/ tasksets[0].hyperperiod;
	float future_impact = TTI;

	vector<int> possible_tasks;

	vector<profile> temperature;

	int slack = 0;

	int computations[tasks->size()];
	for (unsigned int i = 0; i < tasks->size(); i++) {
		computations[i] = 0;
		(*tasks)[i].next_start = 0;
		(*tasks)[i].computations = 0;
	}

	int hyperperiod = tasksets[0].hyperperiod;
	int t_index = -1;
	float target_temp = future_impact / ((float) (hyperperiod));

	float sub_impact;
	float max_temp = initial_temp;
	profile current;
	float pre_initial;
	for (int i = 0; i < hyperperiod; i++) {
		current.time = i;
		current.temperature = initial_temp;
		temperature.push_back(current);
		possibles(&possible_tasks, i, tasks);

		if (slack < 10) {
			slack = compute_slack_exact(i, &t_index, computations, tasks,
					&possible_tasks);
		}
		pre_initial = initial_temp;
		initial_temp = min_deviation(&t_index, &possible_tasks, tasks,
					initial_temp, target_temp);

		if(t_index!=tmp.task_id)
		{
			tmp.end=i;
			if(tmp.task_id!=-1)
			{
				sch->push_back(tmp);
			}
			tmp.start=i;
			tmp.task_id=t_index;

		}

		if (t_index >= 0) {
			sub_impact = (pre_initial - initial_temp) / beta * GRANULARITY
					+ (*tasks)[t_index].power / beta;
			if (sub_impact < 0)
				cout << " sub impact heat " << sub_impact << " pre initial "
						<< pre_initial << " current " << initial_temp
						<< " first term "
						<< (pre_initial - initial_temp) / beta * GRANULARITY<<" power"
						<< (*tasks)[t_index].power << endl;
		} else {
			sub_impact = (pre_initial - initial_temp) / beta * GRANULARITY;
			if (sub_impact < 0)
				cout << " sub impact cool " << sub_impact << " pre initial "
						<< pre_initial << " current " << initial_temp
						<< " first term "
						<< (pre_initial - initial_temp) / beta * GRANULARITY<<" power"
						<< (*tasks)[t_index].power << endl;

		}

		if (t_index >= 0) {
			(*tasks)[t_index].computations = (*tasks)[t_index].computations + 1;
			computations[t_index] = computations[t_index] + 1;
			if ((*tasks)[t_index].computations
					== (*tasks)[t_index].computation_time) {
				(*tasks)[t_index].computations = 0;
				(*tasks)[t_index].next_start = (*tasks)[t_index].next_start
						+ (*tasks)[t_index].period;
			}
		}

		future_impact = future_impact - sub_impact;
		target_temp = (future_impact) / (tasksets[0].hyperperiod - i);
		max_temp = max_temp > initial_temp ? max_temp : initial_temp;
		slack = slack - 1;
	}

	if(tmp.task_id!=-1 && tmp.start!=(*sch)[sch->size()-1].start)
	{
		tmp.end=tasksets[0].hyperperiod;
		sch->push_back(tmp);
	}

	ofstream thermal_profile;
	switch (thermal_optimal) {
	case 4:
		thermal_profile.open("profile_maxfirst_slack_exact");
		break;
	case 5:
		thermal_profile.open("profile_staticspeed_slack_exact");
		break;
	case 7:
		thermal_profile.open("profile_nocons_slack_exact");
		break;
	default:
		thermal_profile.open("profile_slack_exact");
		break;
	}

	thermal_profile << "#Time\tTemparature\n";

	bool thermal_violation = false;
	max_temp = 0;

	double i_initial = TTI/ tasksets[0].hyperperiod;
	double initial = (temperature[temperature.size() - 1].temperature
			- cool(i_initial, temperature.size()))
			/ (1 - exp(-1 * beta * tasksets[0].hyperperiod / GRANULARITY));

	for (unsigned int i = 0; i < temperature.size(); i++) {
		temperature[i].temperature = temperature[i].temperature
				- cool(i_initial, i + 1) + cool(initial, i + 1);
	}

	for (unsigned int i = 0; i < temperature.size(); i++) {

		if(i%PROFILE_GRANULARITY==0)
		{
			thermal_profile << temperature[i].time << "\t"<< temperature[i].temperature << endl;
		}
		if (temperature[i].temperature > corrected_threshold) {
			thermal_violation = true;
		}
		if (temperature[i].temperature > max_temp) {
			max_temp = temperature[i].temperature;
		}
	}
	thermal_profile.close();
	ofstream global_results;
	stringstream fname;
	fname << "results_slack_exact" << seed;
	global_results.open(fname.str().c_str(), fstream::app);

	float c_util = 0.00;
	float t_util = 0.00;

	for (unsigned int i = 0; i < tasks->size(); i++) {
		c_util = c_util
				+ ((float) (*tasks)[i].computation_time)
						/ ((float) (*tasks)[i].period);
		t_util = t_util
				+ tasksets[0].hyperperiod / ((float) (*tasks)[i].period)
						* (*tasks)[i].power * (*tasks)[i].computation_time
						/ beta;
	}

	t_util = t_util / ((float) (tasksets[0].hyperperiod * corrected_threshold));

	end_t = clock();

	global_results << c_util << "\t" << t_util << "\t" << thermal_violation
			<< "\t" << max_temp << "\t" << temperature.size() << "\t"
			<< tasks->size() << "\t" << ((float) (end_t - start_t)) / 1000000.00
			<< endl;
	global_results.close();

	possible_tasks.clear();
	temperature.clear();

}

void opt_schedule(vector<schedule>*sch, vector<task> * tasks,
		vector<schedule>*ref) {
	clock_t start_t, end_t;
	start_t = clock();

	schedule tmp;
	float TTI=0;
	tmp.start=0;
	tmp.end=0;
	tmp.task_id=-1;
	for(unsigned int i=0;i<tasks->size();i++)
	{
		TTI = TTI + (*tasks)[i].power * (*tasks)[i].computation_time
								* (tasksets[0].hyperperiod / (*tasks)[i].period) / beta;
			}

	float initial_temp = TTI/ tasksets[0].hyperperiod;
	float future_impact = TTI;

	vector<int> possible_tasks;
	vector<profile> temperature;

	int slack = 0;

	int computations[tasks->size()];
	for (unsigned int i = 0; i < tasks->size(); i++) {
		computations[i] = 0;
		(*tasks)[i].next_start = 0;
		(*tasks)[i].computations = 0;
	}

	int hyperperiod = tasksets[0].hyperperiod;
	int t_index = -1;
	float target_temp = future_impact / ((float) (hyperperiod));

	cout << "target_temp " << future_impact / hyperperiod << " TTI:"
			<< future_impact << endl;

	float sub_impact;
	float max_temp = initial_temp;
	profile current;
	float pre_initial;
	for (int i = 0; i < hyperperiod; i++) {
		current.time = i;
		current.temperature = initial_temp;
		temperature.push_back(current);
		possibles(&possible_tasks, i, tasks);

		if (slack < 10) {
			slack = compute_slack2(i, computations, ref, tasks,
					&possible_tasks);

		}
		pre_initial = initial_temp;
			initial_temp = min_deviation(&t_index, &possible_tasks, tasks,
					initial_temp, target_temp);

		if(t_index!=tmp.task_id)
		{
			tmp.end=i;
			if(tmp.task_id!=-1)
			{
				sch->push_back(tmp);
			}
			tmp.start=i;
			tmp.task_id=t_index;

		}
		if (t_index >= 0) {
			sub_impact = (pre_initial - initial_temp) / beta * GRANULARITY
					+ (*tasks)[t_index].power / beta;
			if (sub_impact < 0)
				cout << " sub impact heat " << sub_impact << " pre initial "
						<< pre_initial << " current " << initial_temp
						<< " first term "
						<< (pre_initial - initial_temp) / beta * GRANULARITY<<" power"
						<< (*tasks)[t_index].power << endl;
		} else {
			sub_impact = (pre_initial - initial_temp) / beta * GRANULARITY;
			if (sub_impact < 0)
				cout << " sub impact cool " << sub_impact << " pre initial "
						<< pre_initial << " current " << initial_temp
						<< " first term "
						<< (pre_initial - initial_temp) / beta * GRANULARITY<<" power"
						<< (*tasks)[t_index].power << endl;

		}

		if (t_index >= 0) {
			//cout<<"executed task "<<t_index<<" with deadline "<< (i/(*tasks)[t_index].period+1)*(*tasks)[t_index].period<<endl;
			(*tasks)[t_index].computations = (*tasks)[t_index].computations + 1;
			computations[t_index] = computations[t_index] + 1;
			if ((*tasks)[t_index].computations
					== (*tasks)[t_index].computation_time) {
				(*tasks)[t_index].computations = 0;
				(*tasks)[t_index].next_start = (*tasks)[t_index].next_start
						+ (*tasks)[t_index].period;
			}
		}
		else if(slack==0){
			cout<<"incorrect execution of task  "<<t_index<<endl;
			for(int i=0;i<possible_tasks.size();i++)
			{
				cout<<"possible task "<<possible_tasks[i]<<endl;
			}
		}

		future_impact = future_impact - sub_impact;
		target_temp = (future_impact) / (tasksets[0].hyperperiod - i);
		max_temp = max_temp > initial_temp ? max_temp : initial_temp;
		slack = slack - 1;
	}

	if(tmp.task_id!=-1 && tmp.start!=(*sch)[sch->size()-1].start)
	{
		tmp.end=tasksets[0].hyperperiod;
		sch->push_back(tmp);
	}

	ofstream thermal_profile;
	switch (thermal_optimal) {
	case 4:
		thermal_profile.open("profile_maxfirst_slack");
		break;
	case 5:
		thermal_profile.open("profile_staticspeed_slack");
		break;
	case 7:
		thermal_profile.open("profile_nocons_slack");
		break;
	default:
		thermal_profile.open("profile_slack");
		break;
	}

	thermal_profile << "#Time\tTemparature\n";

	bool thermal_violation = false;
	max_temp = 0;

	double i_initial = TTI / tasksets[0].hyperperiod;
	double initial = (temperature[temperature.size() - 1].temperature
			- cool(i_initial, temperature.size()))
			/ (1 - exp(-1 * beta * tasksets[0].hyperperiod / GRANULARITY));

	for (unsigned int i = 0; i < temperature.size(); i++) {
		temperature[i].temperature = temperature[i].temperature
				- cool(i_initial, i + 1) + cool(initial, i + 1);
	}

	for (unsigned int i = 0; i < temperature.size(); i++) {

		if(i%PROFILE_GRANULARITY==0)
		{
			thermal_profile << temperature[i].time << "\t"<< temperature[i].temperature << endl;
		}
		if (temperature[i].temperature > corrected_threshold) {
			thermal_violation = true;
		}
		if (temperature[i].temperature > max_temp) {
			max_temp = temperature[i].temperature;
		}
	}
	thermal_profile.close();
	ofstream global_results;
	stringstream fname;
	fname << "results_slack" << seed;
	global_results.open(fname.str().c_str(), fstream::app);

	float c_util = 0.00;
	float t_util = 0.00;

	for (unsigned int i = 0; i < tasks->size(); i++) {
		c_util = c_util
				+ ((float) (*tasks)[i].computation_time)
						/ ((float) (*tasks)[i].period);
		t_util = t_util
				+ tasksets[0].hyperperiod / ((float) (*tasks)[i].period)
						* (*tasks)[i].power * (*tasks)[i].computation_time
						/ beta;
	}

	t_util = t_util / ((float) (tasksets[0].hyperperiod * corrected_threshold));

	end_t = clock();
	global_results << c_util << "\t" << t_util << "\t" << thermal_violation
			<< "\t" << max_temp << "\t" << temperature.size() << "\t"
			<< tasks->size() << "\t" << ((float) (end_t - start_t)) / 1000000.00
			<< endl;
	global_results.close();

	possible_tasks.clear();
	temperature.clear();
}


void store_opt(vector<task>*tasks) {
	ifstream gamsout;
	gamsout.open("output.txt", ios::in);
	float max_temp, time;
	string trash;
	gamsout >> max_temp >> trash >> time;
	cout << max_temp << "|" << time << endl;
	gamsout.close();

	int thermal_violation;
	if (max_temp > corrected_threshold) {
		thermal_violation = 1;
	}

	ofstream global_results;
	global_results.open("results_gams", fstream::app);

	float c_util = 0.00;
	float t_util = 0.00;

	for (unsigned int i = 0; i < tasks->size(); i++) {
		c_util = c_util
				+ ((float) (*tasks)[i].computation_time)
						/ ((float) (*tasks)[i].period);
		t_util = t_util
				+ tasksets[0].hyperperiod / ((float) (*tasks)[i].period)
						* (*tasks)[i].power * (*tasks)[i].computation_time
						/ beta;
	}

	t_util = t_util / ((float) (tasksets[0].hyperperiod * corrected_threshold));

	global_results << c_util << "\t" << t_util << "\t" << thermal_violation
			<< "\t" << max_temp << "\t" << tasksets[0].hyperperiod << "\t"
			<< tasks->size() << "\t" << time << endl;
	global_results.close();
}

void call_gams() {
	system("alias gams=/usr/gams/gams24.0_linux_x86_32_sfx/gams");
	system("alias gamslib=/usr/gams/gams24.0_linux_x86_32_sfx/gamslib");
	system("/usr/gams/gams24.0_linux_x86_32_sfx/gams 635_project_1.gms");
}
