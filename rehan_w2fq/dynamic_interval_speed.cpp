/*
 * dynamic_interval_speed.cpp
 *
 *  Created on: Mar 9, 2014
 *      Author: rehan
 */
/*
 * dynamic_interval.cpp
 *
 *  Created on: Dec 15, 2013
 *      Author: rehan
 */

/*
 * dynamic_interval.cpp
 *
 *  Created on: Dec 12, 2013
 *      Author: rehan
 */
#include "scheduler.h"

long end_scheduler_speed(vector<long_schedule> *sch, vector<interval>*intervals,
		vector<long_task>*tasks, int interval_index, double* instance_max_comps,
		double *instance_comps, double *net_comps_done, double *total_worst_comps,
		int core) {
	long end_slack; //(*intervals)[interval_index].end-(*sch)[sch->size()-1].end;

	long new_start;
	for (unsigned int i = 0; i < sch->size(); i++) {
		if ((*sch)[i].core == core) {
			end_slack = (*intervals)[interval_index].end - (*sch)[i].end;
			new_start = (*sch)[i].end;
		}
	}

	if (end_slack < 5) {
		return end_slack;
	}

	double possible_comps[tasks->size()];

	int task_id = -1;

	for (unsigned int i = 0; i < tasks->size(); i++) {
		if (net_comps_done[i] < (total_worst_comps[i])) {
			possible_comps[i] = instance_max_comps[i] - instance_comps[i];
			assert(possible_comps>0);
			if (task_id == -1) {
				task_id = i;
			} else if ((*tasks)[i].power > (*tasks)[task_id].power) {
				task_id = i;
			}

		} else {
			possible_comps[i] = 0;
		}
	}

	if (task_id == -1) {
		return 0;
	}

	//cout<<" end slack "<<end_slack<<" task "<<task_id<<" selected power consumption "<<(*tasks)[task_id].power<<endl;

	double average_power = 0;

	for (unsigned int i = 0; i < sch->size(); i++) {
		if ((*sch)[i].start >= (*intervals)[interval_index].start
				&& (*sch)[i].core == core) {
			average_power = average_power
					+ (*sch)[i].power
							* ((*sch)[i].float_end - (double) (*sch)[i].start);
		}
	}

	average_power = average_power
			/ ((double) ((*sch)[sch->size() - 1].end
					- (*intervals)[interval_index].start));
	//cout<<" average power "<<average_power<<endl;

	double speed = pow(average_power / (*tasks)[task_id].power, 1.0 / 3.0);
	//speed=speed>MAX_SPEED?MAX_SPEED:speed<MIN_SPEED?MIN_SPEED:speed;

	if (speed > speed_levels[S_L - 1]) {
		speed = speed_levels[S_L - 1];
	} else if (speed < speed_levels[0]) {
		speed = speed_levels[0];
	} else {
		for (unsigned int j = S_L - 1; j >= 0; j--) {
			if (speed_levels[j] < speed) {
				speed = speed_levels[j + 1];
				break;
			}
		}
	}

	//cout<<"speed" <<speed<<endl;

	double computations = floor(
			((double) ((*intervals)[interval_index].end - new_start)) * speed);

	cout << " task id " << task_id << " computations " << computations
			<< " possible comps " << possible_comps[task_id] << endl;
	computations =
			computations > possible_comps[task_id] ?
					possible_comps[task_id] : computations;

	long_schedule temp_sch;

	temp_sch.arrival = ((*intervals)[interval_index].start
			/ (*tasks)[task_id].period) * (*tasks)[task_id].period;
	temp_sch.start = new_start;
	temp_sch.end = temp_sch.start + ceil(computations / speed);
	assert(temp_sch.end<=(*intervals)[interval_index].end);
	temp_sch.float_end = (double) temp_sch.start + computations / speed;
	temp_sch.power = (*tasks)[task_id].power * pow(speed, 3);
	temp_sch.speed = speed;
	temp_sch.task_id = task_id;
	temp_sch.core = core;

	cout << "schedule added: start" << temp_sch.start << " end " << temp_sch.end
			<< " float end " << temp_sch.float_end << " power "
			<< temp_sch.power << " speed " << temp_sch.speed << endl;

	net_comps_done[task_id] = (instance_comps[task_id] + myceil(computations)) >= instance_max_comps[task_id] - 0.01 ?
			       total_worst_comps[task_id] :
					net_comps_done[task_id] + myceil(computations);
	instance_comps[task_id] = instance_comps[task_id] + myceil(computations);

	cout << " net computations done " << net_comps_done[task_id]
			<< " instance_comps " << instance_comps[task_id] << " max_comps "
			<< instance_max_comps[task_id] << endl;

	sch->push_back(temp_sch);
	return ((*intervals)[interval_index].end - (*sch)[sch->size() - 1].end);

}

void w2fq_interval_speed(vector<long_schedule>*sch, double* computations,
		double* speed, double *max_computations, vector<long_task>*tasks,
		long start, long end, int core) {
	long scaled_comps[tasks->size()];
	double rate[tasks->size()];
	long done_comps[tasks->size()];

	long_schedule temp_sch;

	for (unsigned int i = 0; i < tasks->size(); i++) {
		scaled_comps[i] = myceil(((double) computations[i]) / speed[i]);
		rate[i] = ((double) scaled_comps[i]) / ((double) (end - start));
		done_comps[i] = 0;

		cout << "task " << i << " rate " << rate[i] << " required computations"
				<< computations[i] << " max computations "
				<< max_computations[i] << " max scaled "
				<< myceil((max_computations[i]) / speed[i]) << endl;
	}

	long st = start;
	while (st < end) {
		//cout<<" start "<<start<<endl;
		int id = -1;
		double etime = end + 1;

		for (unsigned int i = 0; i < tasks->size(); i++) {
			if (rate[i] > 0.000000001
					&& done_comps[i] <= round((double((st - start)) * rate[i]))
					&& (double) (done_comps[i] + 1) / rate[i]
							<= (etime + W_INT / 10)
					&& (done_comps[i] + 1)
							<= myceil((max_computations[i]) / speed[i])) {
				etime = round((double((done_comps[i] + 1))) / rate[i]);
				id = i;
			}
		}

		if (id == -1) {
			//	cout<<"trying to idle "<<st<<endl;

			for (unsigned int i = 0; i < tasks->size(); i++) {
				//		cout<<"end time "<<etime<<" id "<<id<<endl;
				if (rate[i] > 0.000000001
						&& (double) (done_comps[i] + 1) / rate[i]
								<= (etime + W_INT / 10)
						&& (done_comps[i] + 1)
								<= myceil((max_computations[i]) / speed[i])) {
					etime = round((double((done_comps[i] + 1))) / rate[i]);
					id = i;
				}
			}
			//	cout<<"exit id"<<id<<endl;

		}

		if (id >= 0) {

			temp_sch.core = core;
			temp_sch.start = st;
			temp_sch.end = st + 1;
			temp_sch.task_id = id;
			temp_sch.power = (*tasks)[id].power * pow(speed[id], 3);
			done_comps[id] = done_comps[id] + 1;
			temp_sch.float_end =
					done_comps[id]
							> (((double) max_computations[id]) / speed[id]) ?
							((double) temp_sch.end)
									- (((double) done_comps[id])
											- ((max_computations[id])
													/ speed[id])) :
							temp_sch.end;

			temp_sch.speed = speed[id];
			temp_sch.arrival = (start / (*tasks)[id].period)
					* (*tasks)[id].period;
			//done_comps[id]=round(done_comps[id] * round(1/W_INT)) * W_INT;
			sch->push_back(temp_sch);
			st = temp_sch.end;
			//start=round(start * round(1/W_INT)) * W_INT;
		} else {
			//	cout<<"idling "<<st<<endl;
			st = st + 1;
		}
	}

	//cout<<"*************\n\n";

	for (unsigned int i = 0; i < tasks->size(); i++) {
		//	cout<<"task "<<i<<" done computations "<<done_comps[i]<<endl;
	}
	//cout<<"*************\n\n";

}

float global_power_interval_speed(double *computations, vector<long_task>*tasks,
		int earliness, double * speed) {
	double total_comps = 0.00;
	double scaled_activity = 0.00;
	for (unsigned int i = 0; i < tasks->size(); i++) {
		total_comps = total_comps + computations[i] / speed[i];
		scaled_activity = scaled_activity
				+ (computations[i]) * pow((*tasks)[i].power, 1 / 3.0);
	}

	return (pow((scaled_activity / (total_comps + earliness)), 3));
}

int maxfirst_interval_speed(double * computations, long earliness,
		double *speeds, vector<long_task>*tasks, int length) {
#if(INST_MF_PRINT)
	cout<<"\n****************starting new instance maxfirst******************"<<endl;
#endif

	double g_power; //=global_power(tasks,0,temp_tasks->size(),available_util);
	double local_comps[tasks->size()];

	for (unsigned int i = 0; i < tasks->size(); i++) {
		local_comps[i] = computations[i];
	}
	bool resolved = false;
	while (!resolved) {
		resolved = true;
		g_power = global_power_interval_speed(local_comps, tasks, earliness,
				speeds);

		//	cout<<"global power high "<<g_power<<endl;
#if(ENABLE_PRINTS)
		//cout<<"global power:"<<g_power<<"utilization "<<used_util<<endl;
#endif
		for (unsigned int i = 0; i < tasks->size(); i++) {
			float speed = pow(g_power / (*tasks)[i].power, 1 / 3.0);
			if (speed > speed_levels[S_L - 1] && local_comps[i] > 0) {
				earliness = earliness
						- (myceil(((local_comps[i])) / speeds[i])
								- local_comps[i] / speed_levels[S_L - 1]);
				speeds[i] = speed_levels[S_L - 1];
				local_comps[i] = 0;
				resolved = false;
				break;

			}
		}
	}
	resolved = false;

	while (!resolved) {
		//	cout<<"global power low "<<g_power<<endl;
		resolved = true;
		g_power = global_power_interval_speed(local_comps, tasks, earliness,
				speeds);

		for (unsigned int i = 0; i < tasks->size(); i++) {
			float speed = pow(g_power / (*tasks)[i].power, 1 / 3.0);
			if (speed < speed_levels[0] && local_comps[i] > 0) {
//					speeds[i]=((double)local_comps[i])/((double)myfloor(((double)(local_comps[i]))/MIN_SPEED));
				earliness = earliness
						- (myceil(((local_comps[i])) / speed_levels[0])
								- local_comps[i] / speeds[i]);
				speeds[i] = speed_levels[0];
				local_comps[i] = 0;
				resolved = false;
				break;
			}
		}
	}
	g_power = global_power_interval_speed(local_comps, tasks, earliness,
			speeds);
	//	cout<<"global power regular "<<g_power<<endl;

	for (unsigned int i = 0; i < tasks->size(); i++) {
		for (unsigned int i = 0; i < tasks->size(); i++) {
			float speed = pow(g_power / (*tasks)[i].power, 1 / 3.0);
			if (local_comps[i] > 0) {
				for (unsigned int j = S_L - 1; j >= 0; j--) {
					if (speed_levels[j] < speed) {
						speeds[i] = speed_levels[j + 1];
						break;
					}
				}
				//	speeds[i]=((double)local_comps[i])/((double)myfloor(((double)(local_comps[i]))/speed));
				local_comps[i] = 0;
				break;
			}
		}
	}

	earliness = length;

	for (unsigned int i = 0; i < tasks->size(); i++) {
		earliness = earliness - myceil(computations[i] / speeds[i]);
	}

	assert(earliness>=0);

	return (earliness);
}

void instance_override_speed(vector<float_task>*tasks, vector<instance>*inst) {

	int hyperperiod = compute_lcm(tasks);
	int index = 0;

	instance temp_inst;

	int total_instances = 0;
	for (unsigned int i = 0; i < tasks->size(); i++) {
		total_instances = total_instances + hyperperiod / (*tasks)[i].period;
	}

	cout << " total instances " << total_instances << endl;

	for (unsigned int i = 0; i < tasks->size(); i++) {
		int start = 0;
		while (start < hyperperiod) {
			temp_inst.computations = ((double) (*tasks)[i].computation_time)
					* (0.5 + (0.5 * rand() / RAND_MAX));
			temp_inst.arrival = start;
			temp_inst.deadline = start + round((*tasks)[i].period);
			temp_inst.task_id = i;
			start = temp_inst.deadline;
			inst->push_back(temp_inst);
		}
	}
}

void dynamic_instance_schedule_speed(vector<long_schedule>*sch,
		vector<long_task>*tasks, vector<interval>*intervals,
		vector<instance>*inst, int speed_scaling_enable) {
	if (tasks->size() > MAX_TASKS) {
		cout << "max task size error. increase parameter" << endl;
		exit(1);

	}
	for (unsigned int i = 0; i < tasks->size(); i++) {
		cout << " task " << i << " computation time "
				<< (*tasks)[i].computation_time << " period "
				<< (*tasks)[i].period << " power " << (*tasks)[i].power << endl;
	}

	cout << "intervals size " << intervals->size() << endl;

	int inst_index[tasks->size()];
	int temp_task = -1;
	for (unsigned int i = 0; i < inst->size(); i++) {
		if ((*inst)[i].task_id != temp_task) {
			temp_task = (*inst)[i].task_id;
			inst_index[temp_task] = i;
		}
	}

	double comps_req[tasks->size()];
	double net_comps_done[tasks->size()];

	int instance_num[tasks->size()];
	double instance_comps[tasks->size()];
	double instance_max_comps[tasks->size()]; ///real computations limit
	double instance_worst_comps[inst->size()]; //worst case computations limit

	double max_comps[tasks->size()];
	double interval_task_speed[intervals->size()][tasks->size()];
	double scaled_comps[intervals->size()][tasks->size()];
	double unscaled_comps[intervals->size()][tasks->size()];
	double total_worst_comps[tasks->size()];
	int task_core_mapping[intervals->size()][tasks->size()];

	double total_comps[intervals->size()][tasks->size()];
	for (unsigned int i = 0; i < intervals->size(); i++) {
		for (unsigned int j = 0; j < tasks->size(); j++) {
			if (i == 0) {
				total_comps[i][j] = 0;
			} else {
				total_comps[i][j] = total_comps[i - 1][j];
			}
			scaled_comps[i][j] = 0;
			unscaled_comps[i][j] = 0;
			task_core_mapping[i][j] = 0;

		}

		for (int j = 0; j < (*intervals)[i].get_size(); j++) {

			task_core_mapping[i][(*intervals)[i].exec[j].task] =
					(*intervals)[i].exec[j].core;

			scaled_comps[i][(*intervals)[i].exec[j].task] =
					scaled_comps[i][(*intervals)[i].exec[j].task]
							+ (*intervals)[i].exec[j].exec;
			unscaled_comps[i][(*intervals)[i].exec[j].task] =
					unscaled_comps[i][(*intervals)[i].exec[j].task]
							+ (*intervals)[i].exec[j].unit_exec;
			total_comps[i][(*intervals)[i].exec[j].task] =
					total_comps[i][(*intervals)[i].exec[j].task]
							+ (*intervals)[i].exec[j].unit_exec;
		}
	}

	for (unsigned int i = 0; i < intervals->size(); i++) {
		for (unsigned int j = 0; j < tasks->size(); j++) {
			if (scaled_comps[i][j] > 0 && unscaled_comps[i][j] > 0) {
				interval_task_speed[i][j] = unscaled_comps[i][j]
						/ scaled_comps[i][j];
			} else {
				interval_task_speed[i][j] = 1;
			}
		}

	}

	for (unsigned int i = 0; i < tasks->size(); i++) {
		net_comps_done[i] = 0;
		instance_num[i] = 0;
		instance_comps[i] = 0;
		instance_max_comps[i] = 0;
	}

	for (unsigned int i = 0; i < inst->size(); i++) {
		instance_worst_comps[i] = 0;
	}

	for (unsigned int i = 0; i < inst->size(); i++) {
		for (unsigned int j = 0; j < intervals->size(); j++) {
//			cout<<"size "<<(*intervals)[j].get_size()<<endl;
//			cout<<"inst index "<<i<<" interval_index "<<j;
//			cout<<" start "<<(*intervals)[j].start<<" arrival "<<(*inst)[i].arrival/W_INT<<
//					" end "<<(*intervals)[j].end<< "deadline "<<(*inst)[i].deadline/W_INT<<" condition1 "<<
//					((*intervals)[j].start>=(long)(*inst)[i].arrival/W_INT)<<" condition 2"<<((*intervals)[j].end<=(long)((*inst)[i].deadline/W_INT+1))<<endl;

			if ((*intervals)[j].start
					>= round(((double) ((*inst)[i].arrival)) / W_INT)
					&& (*intervals)[j].end
							<= round(
									((double) ((*inst)[i].deadline)) / W_INT)) {
//				cout<<"size "<<(*intervals)[j].get_size()<<endl;
//				cin.get();
				for (unsigned int k = 0; k < (*intervals)[j].get_size(); k++) {
//					cout<<"inst index "<<i<<" interval_index "<<j<<" exec index "<<k<<endl;

					if ((*intervals)[j].exec[k].task == (*inst)[i].task_id) {
						instance_worst_comps[i] = instance_worst_comps[i]
								+ (*intervals)[j].exec[k].unit_exec;
					}
				}
			}
		}
	}

	//****debug prints***********************

	for (unsigned int i = 0; i < intervals->size(); i++) {
		cout << "****************interval " << i << " start "
				<< (*intervals)[i].start << " end " << (*intervals)[i].end
				<< endl;

		for (unsigned int j = 0; j < tasks->size(); j++) {
			cout << "task " << j << " scaled comps " << scaled_comps[i][j]
					<< " unscaled_comps " << unscaled_comps[i][j] << " speed "
					<< interval_task_speed[i][j] << " core "
					<< task_core_mapping[i][j] << endl;
		}
	}

	cout << "\n\n******************************************************\n\n";
	for (unsigned int i = 0; i < inst->size(); i++) {
		cout << "task " << (*inst)[i].task_id << " arrival "
				<< (*inst)[i].arrival << " comps " << instance_worst_comps[i]
				<< endl;
	}

	cout << "\n\n******************************************************\n\n";

	for (unsigned int i = 0; i < intervals->size(); i++) {
		//	cout<<"****************interval "<<i<<" start "<<(*intervals)[i].start<<" end "<<(*intervals)[i].end<<endl;

		for (unsigned int j = 0; j < tasks->size(); j++) {
			//	cout<<"task "<<j<<" total_comps "<<total_comps[i][j]<<endl;
		}
	}

	cout << "\n\n******************************************************\n\n";

//	exit(1);

	//****************************************

	int schedule_index = 0;

	long done_comps[tasks->size()];
	long_schedule tempsch;

	for (unsigned int z = 0; z < intervals->size(); z++) {

		double total[CORE];
		long earliness[CORE];

		for (unsigned int i = 0; i < CORE; i++) {
			earliness[i] = 0;
			total[i] = 0;
		}

		for (unsigned int i = 0; i < tasks->size(); i++) {
			comps_req[i] =
					round(total_comps[z][i] - net_comps_done[i]) >= 0 ?
							total_comps[z][i] - net_comps_done[i] : 0;
			//rate[i]=((double)(intervals[z].computations[i]))/((double)((intervals[z].end-intervals[z].start)));
			total[task_core_mapping[z][i]] = total[task_core_mapping[z][i]]
					+ round(comps_req[i] / interval_task_speed[z][i]);
			cout << "task " << i << " comps_req " << comps_req[i] << " scaled "
					<< round(comps_req[i] / interval_task_speed[z][i])
					<< " speed " << interval_task_speed[z][i] << endl;
			instance_num[i] = (*intervals)[z].start / (*tasks)[i].period;
			instance_max_comps[i] =
					(*inst)[inst_index[i] + instance_num[i]].computations
							/ W_INT;
			total_worst_comps[i] =total_worst_comps[i]+ instance_worst_comps[inst_index[i]
					+ instance_num[i]];
			//		cout<<"task "<<i<<" computations required "<<comps_req[i]<<" base computations "<<(*intervals)[z].computations[i]<<endl;

			//		assert(comps_req[i]<=(*intervals)[z].computations[i][0]);
			if ((*intervals)[z].start % (*tasks)[i].period == 0) {
				instance_comps[i] = 0;
			}
		}

		cout << "\n\n REQUIREMENT" << endl;
		for (unsigned int i = 0; i < tasks->size(); i++) {
			cout << "task " << i << " done " << net_comps_done[i]
					<< " total required " << total_comps[z][i] << " core "
					<< task_core_mapping[z][i] << " comps req " << comps_req[i]
					<< endl;
		}

		cout << "totals " << total[0] << "|" << total[1] << "|" << total[2]
				<< endl;

		cin.get();

		cout << "earliness ";

		for (unsigned int i = 0; i < CORE; i++) {
			earliness[i] = ((*intervals)[z].end - (*intervals)[z].start)
					- total[i];
			cout << earliness[i] << "|";
			assert(earliness[i]>=0);
		}

		//	cout<<"earliness="<<earliness<<" start "<<(*intervals)[z].start<<" end "<<(*intervals)[z].end<<" total "<<total<<endl;
		//	assert(earliness>=0);
//		cin.get();

		if (earliness > 0 && speed_scaling_enable == 1) {
			for (unsigned int i = 0; i < CORE; i++) {
				double core_comps[tasks->size()];

				for (unsigned int j = 0; j < tasks->size(); j++) {
					if (comps_req[j] > 0 && task_core_mapping[z][j] == i) {
						core_comps[j] = comps_req[j];
					} else {
						core_comps[j] = 0;
					}
				}
				cout << "calling interval maxfirst" << endl;
				earliness[i] = maxfirst_interval_speed(core_comps, earliness[i],
						interval_task_speed[z], tasks,
						(*intervals)[z].end - (*intervals)[z].start);
			}
		}

		//calling scheduling scheme to construct schedule

		for (unsigned int i = 0; i < tasks->size(); i++) {
			//COMMENTED		cout<<" task "<<i<<"speed "<<speeds[i]<<" computations "<<((double)comps_req[i])/speeds[i]<<" power "<<(*tasks)[i].power*pow(speeds[i],3)<<endl;
		}

		for (unsigned int i = 0; i < tasks->size(); i++) {
			max_comps[i] =
					(instance_comps[i] + comps_req[i]) > instance_max_comps[i] ?
							instance_max_comps[i] - instance_comps[i] :
							comps_req[i];
			max_comps[i] = max_comps[i] > 0.00 ? max_comps[i] : 0.00;
			net_comps_done[i] =
					(instance_comps[i] + comps_req[i])
							>= instance_max_comps[i] ?
							total_worst_comps[i] :
							net_comps_done[i] + comps_req[i];
			if ((instance_comps[i] + comps_req[i]) >= instance_max_comps[i]) {
				cout << "INSTANCE FOR TASK " << i << " completed "
						<< "next arrival "
						<< ((*intervals)[z].start / (*tasks)[i].period + 1)
								* (*tasks)[i].period << endl;
			}

			cout<<"pre instance comps "<<instance_comps[i] <<" comps req "<< comps_req[i]<<endl;
			instance_comps[i] = instance_comps[i] + comps_req[i];
			cout << "new instance comps for task " << i << " "<< instance_comps[i] << endl;
		}

		schedule_index = sch->size();
		for (unsigned int i = 0; i < CORE; i++) {
			cout << "calling wf2q " << i << endl;

			double core_comps[tasks->size()];

			for (unsigned int j = 0; j < tasks->size(); j++) {
				if (comps_req[j] > 0 && task_core_mapping[z][j] == i) {
					core_comps[j] = comps_req[j];
				} else {
					core_comps[j] = 0;
				}

			}

			w2fq_interval_speed(sch, core_comps, interval_task_speed[z],
					max_comps, tasks, (*intervals)[z].start,
					(*intervals)[z].end, i);
			//break;
		}

		sort(sch->begin() + schedule_index, sch->end(), sort_sch);

		long end_slack[CORE];

		for (unsigned int i = schedule_index; i < sch->size(); i++) {
			end_slack[(*sch)[i].core] = (*intervals)[z].end - (*sch)[i].end;
		}

		cout << "schedule size " << sch->size() << endl;
		for (unsigned int i = 0; i < sch->size(); i++) {

			//	if((*sch)[i].start>=(*intervals)[z].start)
			//	cout<<"start "<<(*sch)[i].start<<" end "<<(*sch)[i].end<<" id "<<(*sch)[i].task_id<<" speed "<<(*sch)[i].speed<<" power "
			//	<<(*sch)[i].power<<" float end "<<(*sch)[i].float_end<<" core "<<(*sch)[i].core<<endl;
		}

		cout << "END SLACKS " << end_slack[0] << "|" << end_slack[1] << "|"
				<< end_slack[2] << endl;

		cin.get();

		cout << "calling end scheduler" << endl;

		for (unsigned int i = 0; i < CORE; i++) {
			while (end_slack[i] >= 5 && speed_scaling_enable == 1 && false) {
				exit(1);
				cout << "end slack core " << i << " value " << end_slack[i]
						<< endl;
				end_slack[i] = end_scheduler_speed(sch, intervals, tasks, z,
						instance_max_comps, instance_comps, net_comps_done,
						total_worst_comps, i);
				//	cout<<"end_slack "<<end_slack<<endl;
			}
		}

		//cin.get();

	}

	double comps_completed[tasks->size()];
	double total_max_comps[tasks->size()];

	for (unsigned int i = 0; i < tasks->size(); i++) {
		comps_completed[i] = 0;
		total_max_comps[i] = 0;
	}

	for (unsigned int i = 0; i < sch->size(); i++) {
		comps_completed[(*sch)[i].task_id] = comps_completed[(*sch)[i].task_id]
				+ ((double) (*sch)[i].float_end - (double) (*sch)[i].start)
						* (*sch)[i].speed;
	}

	for (unsigned int i = 0; i < inst->size(); i++) {
		total_max_comps[(*inst)[i].task_id] =
				total_max_comps[(*inst)[i].task_id]
						+ (*inst)[i].computations / W_INT;
	}

	for (unsigned int i = 0; i < tasks->size(); i++) {
		cout << "task " << i << " done computations " << comps_completed[i]
				<< " max computations " << total_max_comps[i] << endl;
	}



}

