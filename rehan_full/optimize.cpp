/*
 * optimize.cpp
 *
 *  Created on: Jun 16, 2013
 *      Author: rehan
 */
#include "scheduler.h"
extern double corrected_threshold;

void optimize(vector<vector<double> >*solutions, vector<vector<int> >*order,
		int vector_index, vector<task>*tasks, double min_speed,
		double max_speed) {

	vector<double> violations;
	vector<int> tid;

	vector<task> temp_tasks;

	vector<double> temps;
	vector<int> otemps;
	for (unsigned int i = 0; i < (*solutions)[vector_index].size(); i++) {
		temps.push_back((*solutions)[vector_index][i]);
	}

	for (unsigned int i = 0; i < (*order)[vector_index].size(); i++) {
		otemps.push_back((*order)[vector_index][i]);
	}

	double available_util = 1.0;
	for (unsigned int i = 0; i < tasks->size(); i++) {
//		temp_speeds[i]=(*solutions)[vector_index][i];
		if ((*solutions)[vector_index][i] < 0) //-1 indicates that decision has not been taken
				{
			temp_tasks.push_back((*tasks)[i]);
		} else {
			available_util = available_util
					- ((((float) (*tasks)[i].computation_time)
							/ ((float) (*tasks)[i].period))
							/ (*solutions)[vector_index][i]);
		}
	}

	double g_power = global_power(&temp_tasks, 0, temp_tasks.size(),
			available_util);

	for (unsigned int i = 0; i < temp_tasks.size(); i++) {
		double temp_speed = (pow(((g_power / (double) temp_tasks[i].power)),
				1 / 3.0));
		if (available_util == 1.0) {
			cout << "optimal speed for task " << i << ": " << temp_speed
					<< endl;
		}

		if (temp_speed > max_speed || temp_speed < min_speed) {
			violations.push_back(temp_speed);
			tid.push_back(temp_tasks[i].tid);
		}
	}

	//solution termination************************

	if (violations.size() == 0) {
		cout << "no violations. Taskset remaining" << temp_tasks.size() << endl;
		for (unsigned int i = 0; i < temp_tasks.size(); i++) {
			(*solutions)[vector_index][temp_tasks[i].tid] = (pow(
					((g_power / (double) temp_tasks[i].power)), 1 / 3.0));
		}
		violations.clear();
		tid.clear();
		temp_tasks.clear();
		return;
	}

	//****************************************

	for (unsigned int i = 0; i < violations.size() - 1; i++) {
		if (temp_tasks.size() == tasks->size()) {
			solutions->push_back((*solutions)[vector_index]);
			order->push_back((*order)[vector_index]);
		}
	}
	for (unsigned int i = 0; i < violations.size(); i++) {
		//	if(i==0)
		//	{
		if (vector_index == 0) {
			(*solutions)[vector_index + i][tid[i]] =
					violations[i] > max_speed ? max_speed : min_speed;
			(*order)[vector_index + i].push_back(tid[i]);
			optimize(solutions, order, vector_index + i, tasks, min_speed,
					max_speed);
		}
	}
	violations.clear();
	tid.clear();
	temp_tasks.clear();
	temps.clear();
	otemps.clear();

	return;
}

double optimize_static(vector<double> *output_speeds, vector<task>*tasks,
		double min_speed, double max_speed) {
	double speed = 0.00;
	for (unsigned int i = 0; i < tasks->size(); i++) {
		speed = speed
				+ ((double) (*tasks)[i].computation_time)
						/ ((double) (*tasks)[i].period);
	}
	if (speed < min_speed) {
		speed = min_speed;
	}
	for (unsigned int i = 0; i < tasks->size(); i++) {
		output_speeds->push_back(speed);
	}

	double t_util = 0.00;
	for (unsigned int i = 0; i < tasks->size(); i++) {
		t_util = t_util
				+ ((double) (*tasks)[i].computation_time) * (*tasks)[i].power
						* pow(speed, S_FACTOR - 1.0)
						/ (((double) (*tasks)[i].period) * corrected_threshold);
	}

	return t_util;
}

void scale(vector<task>*scaled_tasks, vector<task>*tasks,
		vector<double>*speed) {
	for (unsigned int i = 0; i < tasks->size(); i++) {
		scaled_tasks->push_back((*tasks)[i]);
		int previous_comps=(*scaled_tasks)[i].computation_time;
		(*scaled_tasks)[i].computation_time = floor(
				(((double) ((*tasks)[i].computation_time)) / (*speed)[i]));
		(*speed)[i]=((float)previous_comps)/((float)((*scaled_tasks)[i].computation_time));
		(*scaled_tasks)[i].power = (*tasks)[i].power
				* pow((*speed)[i], S_FACTOR );

	}

}

double optimize_maxfirst(vector<double> *output_speeds, vector<task>*tasks,
		double min_speed, double max_speed) {
	vector<int> tid;

	double g_power; //=global_power(tasks,0,temp_tasks->size(),available_util);

	float temp_speeds[tasks->size()];

	for (unsigned int i = 0; i < tasks->size(); i++) {
		temp_speeds[i] = -1.00;
	}

	g_power = global_power_partial(tasks, &tid, 1.0);

#if(ENABLE_PRINTS)
	for(unsigned int i=0;i<tasks->size();i++)
	{
		cout<<"optimal speed for task: "<<i<<" with power consumption "<<(*tasks)[i].power <<":"<<(pow((g_power/((double)(*tasks)[i].power)),1/3.0))<<endl;
	}
	cout<<"global power "<<g_power<<endl;
#endif

//	int iteration=0;
	bool resolved = false;
	float used_util = 0.00;
	while (!resolved) {
		resolved = true;
		g_power = global_power_partial(tasks, &tid, 1.0 - used_util);
#if(ENABLE_PRINTS)
		cout<<"global power:"<<g_power<<"utilization "<<used_util<<endl;
#endif

		for (unsigned int i = 0; i < tasks->size(); i++) {
			float speed = (pow((g_power / ((double) (*tasks)[i].power)),
					1 / 3.0));
			if (speed > max_speed && temp_speeds[i] < 0) {
				tid.push_back(i);
				temp_speeds[i] = max_speed;
				used_util = used_util
						+ ((double) (*tasks)[i].computation_time)
								/ ((double) (*tasks)[i].period)
								/ temp_speeds[i];
				resolved = false;
				break;
			}
		}
	}

	resolved = false;
	//float used_util=0.00;
	while (!resolved) {
		resolved = true;
		g_power = global_power_partial(tasks, &tid, 1.0 - used_util);
#if(ENABLE_PRINTS)
		cout<<"global power:"<<g_power<<"utilization "<<used_util<<endl;
#endif

		for (unsigned int i = 0; i < tasks->size(); i++) {
			float speed = (pow((g_power / ((double) (*tasks)[i].power)),
					1 / 3.0));
			if (speed < min_speed && temp_speeds[i] < 0) {
				tid.push_back(i);
				temp_speeds[i] = min_speed;
				used_util = used_util
						+ ((double) (*tasks)[i].computation_time)
								/ ((double) (*tasks)[i].period)
								/ temp_speeds[i];
				resolved = false;
				break;
			}
		}
	}

	g_power = global_power_partial(tasks, &tid, 1.0 - used_util);

	for (unsigned int i = 0; i < tasks->size(); i++) {
		if (temp_speeds[i] < 0) {
			float speed = (pow((g_power / ((double) (*tasks)[i].power)),
					1 / 3.0));
			if (speed < min_speed || speed > max_speed) {
				cout << "possible error in speed evaluation" << endl;
				exit(1);
			}
			temp_speeds[i] = speed;
		}
	}

	double thermal_util = 0.00;
	for (unsigned int i = 0; i < tasks->size(); i++) {
		output_speeds->push_back(temp_speeds[i]);
//		cout<<"task "<<i<<" speed "<<temp_speeds[i]<<endl;
		thermal_util = thermal_util
				+ (*tasks)[i].computation_time * (*tasks)[i].power
						* pow(temp_speeds[i], S_FACTOR - 1.0)
						/ ((*tasks)[i].period * corrected_threshold);

	}

	//solution termination************************
	//violations.clear();
	tid.clear();

	return thermal_util;
}

double optimize_minfirst(vector<double> *output_speeds, vector<task>*tasks,
		double min_speed, double max_speed) {
	vector<int> tid;
	double g_power; //=global_power(tasks,0,temp_tasks->size(),available_util);

	float temp_speeds[tasks->size()];

	for (unsigned int i = 0; i < tasks->size(); i++) {
		temp_speeds[i] = -1.00;
	}

	g_power = global_power_partial(tasks, &tid, 1.0);

#if(ENABLE_PRINTS)
	for(unsigned int i=0;i<tasks->size();i++)
	{
		cout<<"optimal speed for task: "<<i<<" with power consumption "<<(*tasks)[i].power <<":"<<(pow((g_power/((double)(*tasks)[i].power)),1/3.0))<<endl;
	}
	cout<<"global power "<<g_power<<endl;
#endif

//	int iteration=0;
	bool resolved = false;
	float used_util = 0.00;
	while (!resolved) {
		resolved = true;
		g_power = global_power_partial(tasks, &tid, 1.0 - used_util);
#if(ENABLE_PRINTS)
		cout<<"global power:"<<g_power<<"utilization "<<used_util<<endl;
#endif

		for (unsigned int i = 0; i < tasks->size(); i++) {
			float speed = (pow((g_power / ((double) (*tasks)[i].power)),
					1 / 3.0));
			if (speed < min_speed && temp_speeds[i] < 0) {
				tid.push_back(i);
				temp_speeds[i] = min_speed;
				used_util = used_util
						+ ((double) (*tasks)[i].computation_time)
								/ ((double) (*tasks)[i].period)
								/ temp_speeds[i];
				resolved = false;
				break;
			}
		}
	}

	resolved = false;
	//float used_util=0.00;
	while (!resolved) {
		resolved = true;
		g_power = global_power_partial(tasks, &tid, 1.0 - used_util);
#if(ENABLE_PRINTS)
		cout<<"global power:"<<g_power<<"utilization "<<used_util<<endl;
#endif

		for (unsigned int i = 0; i < tasks->size(); i++) {
			float speed = (pow((g_power / ((double) (*tasks)[i].power)),
					1 / 3.0));
			if (speed > max_speed && temp_speeds[i] < 0) {
				tid.push_back(i);
				temp_speeds[i] = max_speed;
				used_util = used_util
						+ ((double) (*tasks)[i].computation_time)
								/ ((double) (*tasks)[i].period)
								/ temp_speeds[i];
				resolved = false;
				break;
			}
		}
	}

	if (used_util > 0.00) {
		return 100.00; //returning 100 so that maxfirst output is chosen by default
	}

	g_power = global_power_partial(tasks, &tid, 1.0 - used_util);

	for (unsigned int i = 0; i < tasks->size(); i++) {
		if (temp_speeds[i] < 0) {
			float speed = (pow((g_power / ((double) (*tasks)[i].power)),
					1 / 3.0));
			if (speed < min_speed || speed > max_speed) {
				cout << "possible error in speed evaluation" << endl;
				exit(1);
			}
			temp_speeds[i] = speed;
		}
	}

	double thermal_util = 0.00;
	for (unsigned int i = 0; i < tasks->size(); i++) {
		output_speeds->push_back(temp_speeds[i]);
		thermal_util = thermal_util
				+ (*tasks)[i].computation_time * (*tasks)[i].power
						* pow(temp_speeds[i], S_FACTOR - 1.0)
						/ ((*tasks)[i].period * corrected_threshold);

//		cout<<"task "<<i<<" speed "<<temp_speeds[i]<<endl;
	}

	//solution termination************************
	//violations.clear();
	tid.clear();

	return thermal_util;
}

double optimize_maxmin(vector<double> *output_speeds, vector<task>*tasks,
		double min_speed, double max_speed) {
	vector<double> temp_speeds1;
	vector<double> temp_speeds2;
	double maxutil = optimize_maxfirst(&temp_speeds1, tasks, min_speed,
			max_speed);
	double minutil = optimize_maxfirst(&temp_speeds2, tasks, min_speed,
			max_speed);

	if (maxutil < minutil) {
		for (unsigned int i = 0; i < temp_speeds1.size(); i++) {
			output_speeds->push_back(temp_speeds1[i]);
		}
		temp_speeds1.clear();
		temp_speeds2.clear();

		return maxutil;
	} else {
		for (unsigned int i = 0; i < temp_speeds2.size(); i++) {
			output_speeds->push_back(temp_speeds2[i]);
		}
		temp_speeds1.clear();
		temp_speeds2.clear();

		return minutil;
	}
}

void optimize_serial(vector<double> *output_speeds, vector<task>*tasks,
		double min_speed, double max_speed) {
	vector<int> tid;
	double g_power; //=global_power(tasks,0,temp_tasks->size(),available_util);

	float temp_speeds[tasks->size()];

	for (unsigned int i = 0; i < tasks->size(); i++) {
		temp_speeds[i] = -1.00;
	}

	g_power = global_power_partial(tasks, &tid, 1.0);

#if(ENABLE_PRINTS)
	for(unsigned int i=0;i<tasks->size();i++)
	{
		cout<<"optimal speed for task: "<<i<<" with power consumption "<<(*tasks)[i].power <<":"<<(pow((g_power/((double)(*tasks)[i].power)),1/3.0))<<endl;
	}
	cout<<"global power "<<g_power<<endl;
#endif

//	int iteration=0;
	bool resolved = false;
	float used_util = 0.00;
	while (!resolved) {
		resolved = true;
		g_power = global_power_partial(tasks, &tid, 1.0 - used_util);
#if(ENABLE_PRINTS)
		cout<<"global power:"<<g_power<<"utilization "<<used_util<<endl;
#endif

		for (unsigned int i = 0; i < tasks->size(); i++) {
			float speed = (pow((g_power / ((double) (*tasks)[i].power)),
					1 / 3.0));
			if ((speed > max_speed || speed < min_speed)
					&& temp_speeds[i] < 0) {
				tid.push_back(i);
				temp_speeds[i] = speed > max_speed ? max_speed : min_speed;
				used_util = used_util
						+ ((double) (*tasks)[i].computation_time)
								/ ((double) (*tasks)[i].period)
								/ temp_speeds[i];
				resolved = false;
				break;
			}
		}
	}
	g_power = global_power_partial(tasks, &tid, 1.0 - used_util);

	for (unsigned int i = 0; i < tasks->size(); i++) {
		if (temp_speeds[i] < 0) {
			float speed = (pow((g_power / ((double) (*tasks)[i].power)),
					1 / 3.0));
			if (speed < min_speed || speed > max_speed) {
				cout << "possible error in speed evaluation" << endl;
				exit(1);
			}
			temp_speeds[i] = speed;
		}
	}

	for (unsigned int i = 0; i < tasks->size(); i++) {
		output_speeds->push_back(temp_speeds[i]);
		cout << "task " << i << " speed " << temp_speeds[i] << endl;
	}

	//solution termination************************
	//violations.clear();
	tid.clear();

	return;
}

void speed_scale(vector<task>*scaled_tasks, vector<double>*speeds,
		vector<task>*tasks, float adjust) {
	/* double max_correction=0.0;
	 for(unsigned int i=0;i<tasks->size();i++)
	 {
	 max_correction=max_correction+1.0/((double)(*tasks)[i].period)*adjust;
	 }
	 double adjustment=1.0-max_correction;

	 cout<<"adjustment:"<<adjustment<<endl;
	 */
	double g_power = global_power(tasks, 0, tasks->size(), 1.0) / (adjust);
	for (unsigned int i = 0; i < tasks->size(); i++) {
		speeds->push_back(
				pow(((g_power / (double) (*tasks)[i].power)), 1 / 3.0));
	}
	for (unsigned int i = 0; i < tasks->size(); i++) {
		scaled_tasks->push_back((*tasks)[i]);
		(*scaled_tasks)[i].power = g_power;
		(*scaled_tasks)[i].computation_time = ceil(
				((double) (*scaled_tasks)[i].computation_time) / (*speeds)[i]);
	}
	float excess_computation = 1.00;
	for (unsigned int i = 0; i < scaled_tasks->size(); i++) {
		excess_computation = excess_computation
				- ((float) (*scaled_tasks)[i].computation_time)
						/ ((float) (*scaled_tasks)[i].period);
	}
	excess_computation = excess_computation * (-1);
#if(ENABLE_PRINTS)

	cout<<"excess computation as a result of speed scaling "<<excess_computation<<endl;
#endif

	float util1 = 0;
	float util2 = 0;

	for (unsigned int i = 0; i < tasks->size(); i++) {
		util1 = util1
				+ (float) (*tasks)[i].computation_time
						/ (float) (*tasks)[i].period;
		util2 = util2
				+ (float) (*scaled_tasks)[i].computation_time
						/ (float) (*scaled_tasks)[i].period;
	}
#if(ENABLE_PRINTS)

	cout<<"util "<<util1<<"|"<<util2<<endl;
#endif
	if (util2 > 1) {
#if(ENABLE_PRINTS)

		cout<<"running scaling again with greater precision adjustment"<<endl;
#endif
		scaled_tasks->clear();
		speeds->clear();
		speed_scale(scaled_tasks, speeds, tasks, adjust - (util2 - 1));
	}
}



