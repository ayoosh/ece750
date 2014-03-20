/*
 * inst_sched.cpp
 *
 *  Created on: Jun 16, 2013
 *      Author: rehan
 */
#include "scheduler.h"
extern float beta;
extern float corrected_threshold;
extern int seed;
extern vector<taskset>tasksets;


double global_power_instances(vector<task>*tasks, int start, int*computations) {
	int total_computations = 0;
	double global_power = 0.00;
	for (unsigned int i = 0; i < tasks->size(); i++) {
		total_computations = (tasksets[0].hyperperiod / (*tasks)[i].period)
				* (*tasks)[i].computation_time - computations[i];
		global_power = global_power
				+ total_computations * pow((*tasks)[i].power, 1 / 3.0)
						/ ((double) (tasksets[0].hyperperiod - start));
	}
	global_power = pow(global_power, 3.0);
	return global_power;
}

void optimize_instances(vector<double> *output_speeds, vector<task>*tasks,
		vector<schedule>*sch, vector<schedule>*ref, vector<schedule>*ref2,
		double min_speed, double max_speed, vector<double>*optimal_speeds,
		vector<int>*sl) {
	schedule temp;
	for (unsigned int i = 0; i < tasks->size(); i++) {
		(*tasks)[i].computations = 0;
		(*tasks)[i].next_start = 0;
	}
	vector<int> times;
	imp_times(tasks, &times);

	int computations_performed[tasks->size()];

	for (unsigned int i = 0; i < tasks->size(); i++) {
		computations_performed[i] = 0;
	}

#if(ENABLE_PRINTS)

	for(unsigned int i=0;i<times.size();i++)
	{
		cout<<"times: "<<i<<":"<<times[i]<<endl;
	}
#endif

	int start = 0;
	int time_end;

	vector<profile> temperature;
	profile temp_t;
	double target_power = global_power_instances(tasks, start,
			computations_performed);
	temp_t.time = 0;
	cout << "target power " << target_power << " initial temperature "
			<< target_power / beta << endl;
	temp_t.temperature = target_power / beta;

	float max_temp;
	max_temp = target_power / beta;
	bool terminate = false;

	int iteration = 0;
	while (!terminate) {
		max_temp = temp_t.temperature;
		sch->clear();
		output_speeds->clear();
		optimal_speeds->clear();
		temperature.clear();
		temperature.push_back(temp_t);
		for (unsigned int i = 0; i < tasks->size(); i++) {
			(*tasks)[i].computations = 0;
			(*tasks)[i].next_start = 0;
			computations_performed[i] = 0;
		}

		while (start < tasksets[0].hyperperiod) {
			int taskid = min_deadline(tasks, start, &times, &time_end);
			temp_t.time = start;
			if (taskid == -1) {
				temp_t.temperature = cool(
						temperature[temperature.size() - 1].temperature,time_end - start);
				temp_t.time = time_end;
				temperature.push_back(temp_t);
				start = time_end;
				if (start >= tasksets[0].hyperperiod) {
					break;
				}
				taskid = min_deadline(tasks, start, &times, &time_end);
				temp_t.time = start;
			}
			target_power = global_power_instances(tasks, start,
					computations_performed);
			if (target_power < 0) {
				cout << "error detected in computation of target power" << endl;
				for (unsigned int i = 0; i < tasks->size(); i++) {
					cout << "task " << i << " computations done "
							<< computations_performed[i]
							<< " total computations "
							<< (tasksets[0].hyperperiod / (*tasks)[i].period)
									* (*tasks)[i].computation_time << endl;
				}
			}
			float scale = pow(((double) (*tasks)[taskid].power) / target_power,
					1 / 3.0);

			if (scale > 1 / min_speed) {
				scale = 1 / min_speed;
			}
			optimal_speeds->push_back(1 / scale);
			int unit_computations = (*tasks)[taskid].computation_time
					- (*tasks)[taskid].computations;
			int scaled_computations = unit_computations * scale; //implicit floor value due to cast to int

			scale = ((float) (scaled_computations))
					/ ((float) (unit_computations));

			if (scale < 1 / max_speed) {
				scale = 1 / max_speed;
				scaled_computations = ceil(((float) unit_computations) * scale); //implicit floor value due to cast to int
				scale = ((float) (scaled_computations))
						/ ((float) (unit_computations));
			}

			int t_index1, slack;

			temp.start = start;
			temp.task_id = taskid;

			vector<int> possible_tasks;

			slack = compute_slack_exact(start, &t_index1,
					computations_performed, tasks, &possible_tasks);
			cout<<"slack from instances "<<slack<<" min deadline task "<<t_index1<<endl;
			sl->push_back(slack);

			assert(slack>=0);

			if (start + scaled_computations <= time_end) {
				if ((scaled_computations - unit_computations) <= slack) {

					computations_performed[taskid] =
							computations_performed[taskid] + unit_computations;
					(*tasks)[taskid].next_start = ((start
							/ (*tasks)[taskid].period) + 1)
							* (*tasks)[taskid].period;
					(*tasks)[taskid].computations = 0;
					temp.end = start + scaled_computations;
				} else {

					scale = ((double) (slack + unit_computations))
							/ ((double) (unit_computations));
					computations_performed[taskid] =
							computations_performed[taskid] + unit_computations;
					(*tasks)[taskid].next_start = ((start
							/ (*tasks)[taskid].period) + 1)
							* (*tasks)[taskid].period;
					(*tasks)[taskid].computations = 0;
					temp.end = start + slack + unit_computations;
				}
			} else {

				if ((scaled_computations - unit_computations) > slack) {
					scale = ((double) (unit_computations + slack))
							/ ((double) (unit_computations));
					computations_performed[taskid] =
							computations_performed[taskid] + unit_computations;
					(*tasks)[taskid].next_start = ((start
							/ (*tasks)[taskid].period) + 1)
							* (*tasks)[taskid].period;
					(*tasks)[taskid].computations = 0;
					temp.end = start + slack;
				} else {
					int done_unit_computations = ceil(
							(double) (time_end - start) / scale);
					scale = ((double) (time_end - start))
							/ ((double) done_unit_computations);
					computations_performed[taskid] =
							computations_performed[taskid]
									+ done_unit_computations;
					(*tasks)[taskid].computations =
							(*tasks)[taskid].computations
									+ done_unit_computations;
					if ((*tasks)[taskid].computations
							== (*tasks)[taskid].computation_time) { //corner case handled
						(*tasks)[taskid].next_start = ((start
								/ (*tasks)[taskid].period) + 1)
								* (*tasks)[taskid].period;
						(*tasks)[taskid].computations = 0;
					}
					temp.end = time_end;
				}
			}

			sch->push_back(temp);

			temp_t.time = temp.end;
			temp_t.temperature = heat(
					temperature[temperature.size() - 1].temperature,
					(*tasks)[temp.task_id].power / pow(scale, 3),
					temp.end - temp.start);

			if (temp_t.time > temperature[temperature.size() - 1].time) {
				temperature.push_back(temp_t);
			}
			max_temp =
					max_temp > temp_t.temperature ?
							max_temp : temp_t.temperature;

			output_speeds->push_back(1 / scale);
			start = temp.end;

		}
		iteration = iteration + 1;

		if (temp_t.time < tasksets[0].hyperperiod) {
			temp_t.temperature = cool(temp_t.temperature,
					tasksets[0].hyperperiod - temp_t.time);
			temp_t.time = tasksets[0].hyperperiod;
			temperature.push_back(temp_t);
		}

		if (iteration > 20
				|| fabs(
						temperature[0].temperature
								- temperature[temperature.size()].temperature)
						< 0.5) {
			terminate = true;
		}
		temp_t.temperature =
				temperature[0].temperature
						> temperature[temperature.size() - 1].temperature ?
						temperature[0].temperature
								- ((temperature[0].temperature
										- temperature[temperature.size() - 1].temperature)
										/ 2) :
						temperature[0].temperature
								+ ((temperature[temperature.size() - 1].temperature
										- temperature[0].temperature) / 2);

		temp_t.time = 0;
		start = 0;

	}

	for (unsigned int i = 0; i < output_speeds->size(); i++) {
		cout << i << "|" << (*output_speeds)[i] << endl;
	}

	cout << "iterations " << iteration << endl;

#if(ENABLE_PRINTS)

	for(unsigned int i=0;i<sch->size();i++)
	{
		cout<<i<<": Task:"<<(*sch)[i].task_id<<" start:"<<(*sch)[i].start<<" end: "<<(*sch)[i].end<<endl;
	}
#endif

	stringstream fname;
	ofstream global_results;
	fname << "results_instances" << seed;
	global_results.open(fname.str().c_str(), fstream::app);

	float c_util = 0.00;
	float t_util = 0.00;

	bool thermal_violation = false;
	if (max_temp > corrected_threshold) {
		thermal_violation = true;
	}

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
			<< tasks->size() << endl;
	global_results.close();

	ofstream thermal_profile;

	thermal_profile.open("profile_instances");

	thermal_profile << "#Time\tTemparature\n";

	for (unsigned int i = 0; i < temperature.size(); i++) {
		thermal_profile << temperature[i].time << "\t"
				<< temperature[i].temperature << endl;
	}
	temperature.clear();
}

