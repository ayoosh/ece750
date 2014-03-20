/*
 * slack.cpp
 *
 *  Created on: Jun 16, 2013
 *      Author: rehan
 */

#include "scheduler.h"
extern vector<taskset> tasksets;

bool sort_inst_deadline(instance a, instance b) {
	if(a.deadline!=b.deadline)
	{
		return (a.deadline < b.deadline);
	}
	else
	{
		return(a.arrival<b.arrival);
	}
}

bool sort_inst_arrival(instance a, instance b) {
	if(a.arrival!=b.arrival)
	{
		return(a.arrival<b.arrival);
	}
	else
	{
		return(a.task_id<b.task_id);
	}
}





int compute_slack2(int start, int *computations, vector<schedule>*sch,
		vector<task>*tasks, vector<int>*possible_tasks) {

	possible_tasks->clear();
	possible_tasks->push_back(-1);
	for (unsigned int i = 0; i < tasks->size(); i++) {
		if (start >= (*tasks)[i].next_start
				&& (*tasks)[i].computations < (*tasks)[i].computation_time) {
			possible_tasks->push_back(i);
		}
	}

	vector<int> deadlines;
	for (unsigned int i = 0; i < tasks->size(); i++) {
		deadlines.push_back(
				(start / (*tasks)[i].period + 1) * (*tasks)[i].period);
	}

//	sort(deadlines.begin(), deadlines.end());
	//computing discrete deadline levels
	vector<cd_computations> cdc; //common deadline computations
	vector<cd_computations> e_cdc; //edl common deadline computations
	cd_computations cd_temp;

	for (unsigned int i = 0; i < tasks->size(); i++) {
		int cdc_index=-1;
		for(unsigned int j=0;j<cdc.size();j++)
		{
			if (deadlines[i] == cdc[j].deadline) {
				cdc_index=j;
				break;
			}
		}
		if(cdc_index==-1)
		{
			cd_temp.deadline = deadlines[i];
			cd_temp.computations = 0;
			cdc.push_back(cd_temp);
			e_cdc.push_back(cd_temp);
		}
		for (unsigned int j = 0; j < cdc.size(); j++) {
			if (cdc[j].deadline == deadlines[i]) {
				cdc[j].computations = cdc[j].computations + computations[i];
			}
		}

	}

	int deadline_times[cdc.size()];

	for(unsigned int i=0;i<cdc.size();i++)
	{
		deadline_times[i] = tasksets[0].hyperperiod;
	}


	for (unsigned int i = 0; i < sch->size(); i++) {
		for (unsigned int j = 0; j < e_cdc.size(); j++) {

			if (e_cdc[j].deadline == deadlines[(*sch)[i].task_id]
					&& e_cdc[j].computations <= cdc[j].computations) {
				e_cdc[j].computations = e_cdc[j].computations + (*sch)[i].end
						- (*sch)[i].start;
				if (e_cdc[j].computations > cdc[j].computations) {
					deadline_times[j] = (*sch)[i].end
							- (e_cdc[j].computations - cdc[j].computations);
				}
				break;
			}

		}
	}
/*
	cout << " start time " << start << endl;
	for (unsigned int i = 0; i < cdc.size(); i++) {
		cout << " cdc deadline " << cdc[i].deadline << " computations "
				<< cdc[i].computations << " edl computations "
				<< e_cdc[i].computations << " times " << deadline_times[i]
				<< endl;
	}
*/
	int slack = 0;



#if(ENABLE_PRINTS)
	cout<<"printing compulsary times from compute_slack"<<endl;
	cout<<"start: "<<start<<endl;
	for(unsigned int i=0;i<tasks->size();i++)
	{
		cout<<"times for task "<< i<< "|"<< sch_times[i]<<" local slack: "<<sch_times[i]-start<<endl;
	}
	cout<<"printing done from compute_slack"<<endl;
#endif

	slack = tasksets[0].hyperperiod;

	int min_group = 0;

	for (unsigned int i = 0; i < cdc.size(); i++) {
		if (deadline_times[i] - start < slack) {
			slack = deadline_times[i] - start;
			min_group = i;
		}
	}

	if(slack==0)
	{
	//	cout<<"0 slack encountered"<<endl;
		assert((*possible_tasks)[0]==-1);
		possible_tasks->erase(possible_tasks->begin());
		for(unsigned int i=0;i<possible_tasks->size();i++)
		{

			if(deadlines[(*possible_tasks)[i]]!=cdc[min_group].deadline)
			{
				possible_tasks->erase(possible_tasks->begin()+i);
				i=i-1;
			}
		}
/*		for(int i=0;i<possible_tasks->size();i++)
		{
			cout<<"possible task "<<(*possible_tasks)[i]<<" deadline "<<
					(start/(*tasks)[(*possible_tasks)[i]].period+1)*(*tasks)[(*possible_tasks)[i]].period<<endl;
		}*/
	}


	if (slack < 0) {
		cout << "warning 0 slack detedted: " << slack << endl;

		//return slack;
		cout << "slack error detected " << endl;

		exit(1);

	}
	return slack;

}



int compute_slack(int start, int * tid, int *computations, vector<schedule>*sch,
		vector<task>*tasks, vector<int>*possible_tasks) {

	possible_tasks->clear();
	for (unsigned int i = 0; i < tasks->size(); i++) {
		if (start >= (*tasks)[i].next_start
				&& (*tasks)[i].computations < (*tasks)[i].computation_time) {
			possible_tasks->push_back(i);
		}
	}

	vector<int> deadlines;
	for (unsigned int i = 0; i < tasks->size(); i++) {
		deadlines.push_back(
				(start / (*tasks)[i].period + 1) * (*tasks)[i].period);
	}

	sort(deadlines.begin(), deadlines.end());
	//computing discrete deadline levels
	vector<cd_computations> cdc; //common deadline computations
	vector<cd_computations> e_cdc; //edl common deadline computations
	cd_computations cd_temp;
	int current_deadline = -1;

	for (unsigned int i = 0; i < tasks->size(); i++) {
		if (deadlines[i] != current_deadline) {
			current_deadline = deadlines[i];
			cd_temp.deadline = current_deadline;
			cd_temp.computations = 0;
			cdc.push_back(cd_temp);

			e_cdc.push_back(cd_temp);
		}
		for (unsigned int j = 0; j < cdc.size(); j++) {
			if (cdc[j].deadline == deadlines[i]) {
				cdc[j].computations = cdc[j].computations + computations[i];
			}
		}

	}

	int deadline_times[cdc.size()];

	for(unsigned int i=0;i<cdc.size();i++)
	{
		deadline_times[i] = tasksets[0].hyperperiod;
	}


	for (unsigned int i = 0; i < sch->size(); i++) {
		for (unsigned int j = 0; j < e_cdc.size(); j++) {

			if (e_cdc[j].deadline == deadlines[(*sch)[i].task_id]
					&& e_cdc[j].computations <= cdc[j].computations) {
				e_cdc[j].computations = e_cdc[j].computations + (*sch)[i].end
						- (*sch)[i].start;
				if (e_cdc[j].computations > cdc[j].computations) {
					deadline_times[j] = (*sch)[i].end
							- (e_cdc[j].computations - cdc[j].computations);
				}
				break;
			}

		}
	}

	cout << " start time " << start << endl;
	for (unsigned int i = 0; i < cdc.size(); i++) {
		cout << " cdc deadline " << cdc[i].deadline << " computations "
				<< cdc[i].computations << " edl computations "
				<< e_cdc[i].computations << " times " << deadline_times[i]
				<< endl;
	}

	for (unsigned int i = 0; i < cdc.size(); i++) {
//		cout<<"combined deadline task "<<i<<" with deadline "<<cdc[i].deadline<<" computations "<<cdc[i].computations
//				<<" in edl "<<e_cdc[i].computations<<" at time "<<start<<endl;
	}

	int slack = 0;
	int sch_computations[sch->size()];
	int sch_times[sch->size()];

	for (unsigned int i = 0; i < tasks->size(); i++) {
		sch_computations[i] = 0;
		sch_times[i] = tasksets[0].hyperperiod;
	}

	for (unsigned int i = 0; i < sch->size(); i++) {
		schedule temp;
		temp = (*sch)[i];
		if ((sch_computations[temp.task_id] + temp.end - temp.start)
				<= computations[temp.task_id]) {
			sch_computations[temp.task_id] = sch_computations[temp.task_id]
					+ temp.end - temp.start;
		} else if (sch_computations[temp.task_id] <= computations[temp.task_id]
				&& (sch_computations[temp.task_id] + temp.end - temp.start)
						> computations[temp.task_id]) {
			sch_times[temp.task_id] = temp.start + computations[temp.task_id]
					- sch_computations[temp.task_id];
			sch_computations[temp.task_id] = sch_computations[temp.task_id]
					+ temp.end - temp.start;
		}
	}

#if(ENABLE_PRINTS)
	cout<<"printing compulsary times from compute_slack"<<endl;
	cout<<"start: "<<start<<endl;
	for(unsigned int i=0;i<tasks->size();i++)
	{
		cout<<"times for task "<< i<< "|"<< sch_times[i]<<" local slack: "<<sch_times[i]-start<<endl;
	}
	cout<<"printing done from compute_slack"<<endl;
#endif

	slack = tasksets[0].hyperperiod;

	int min_task = 0;

	for (unsigned int i = 0; i < tasks->size(); i++) {
		if (sch_times[i] - start < slack) {
			slack = sch_times[i] - start;
			min_task = i;
		}
	}

	(*tid) = min_task;

	if (slack < 0) {
		cout << "warning 0 slack detected: " << slack << endl;

		//return slack;
		cout << "slack error detected " << endl;
		cout << "time from schedule " << sch_times[min_task];

		cout << "computations for task " << min_task << "|"
				<< computations[min_task] << endl;

		int cumulative_comps = 0;
		for (unsigned int i = 0; i < sch->size(); i++) {
			if ((*sch)[i].task_id == min_task) {
				cumulative_comps = cumulative_comps + (*sch)[i].end
						- (*sch)[i].start;
				cout << "start " << (*sch)[i].start << " end " << (*sch)[i].end
						<< " computations " << cumulative_comps << endl;
			}
		}

		exit(1);

	}
	return slack;

}


int compute_slack_exact(int start_time, int* tid, int * computations,
		vector<task>*tasks, vector<int>*possible_tasks) {
	possible_tasks->clear();
	possible_tasks->push_back(-1);
	for (unsigned int i = 0; i < tasks->size(); i++) {
		if (start_time >= (*tasks)[i].next_start
				&& (*tasks)[i].computations < (*tasks)[i].computation_time) {
			possible_tasks->push_back(i);
		}
	}

	int slack = 0;
	vector<schedule> sch;
	schedule s;
	vector<instance> inst;
	instance temp_inst;
	for (unsigned int i = 0; i < tasks->size(); i++) {
		int start = computations[i] / (*tasks)[i].computation_time
				* (*tasks)[i].period;
		temp_inst.computation_time = (*tasks)[i].computation_time
				- computations[i] % (*tasks)[i].computation_time;
		temp_inst.arrival = start
				+ computations[i] % (*tasks)[i].computation_time;
		temp_inst.deadline = start + (*tasks)[i].period;
		temp_inst.task_id = i;
		if (temp_inst.computation_time > 0
				&& temp_inst.arrival < tasksets[0].hyperperiod) {
			inst.push_back(temp_inst);
			start = temp_inst.deadline;
		}
		while (start < tasksets[0].hyperperiod) {
			temp_inst.computation_time = (*tasks)[i].computation_time;
			temp_inst.arrival = start;
			temp_inst.deadline = start + (*tasks)[i].period;
			start = temp_inst.deadline;
			temp_inst.task_id = i;
			inst.push_back(temp_inst);
		}
	}

	if (inst.size() == 0) {
		return (tasksets[0].hyperperiod - start_time);
	}

	sort(inst.begin(), inst.end(), sort_inst_deadline);
	/*	for(int i=0;i<inst.size();i++)
	 {
	 cout<<i<<" type "<<inst[i].task_id <<" arrival time: "<<inst[i].arrival<<" deadline: "<<inst[i].deadline<<" computation time: "<<inst[i].computation_time<<endl;
	 }*/
	//transforming task instances for edl;
	for (unsigned int i = 0; i < inst.size(); i++) {
		temp_inst = inst[i];
		inst[i].arrival = tasksets[0].hyperperiod - temp_inst.deadline;
		inst[i].deadline = tasksets[0].hyperperiod - temp_inst.arrival;
	}

//	cout<<"reversed taskset"<<endl;

	sort(inst.begin(), inst.end(), sort_inst_deadline);
	/*	for(int i=0;i<inst.size();i++)
	 {
	 cout<<i<<" type "<<inst[i].task_id <<" arrival time: "<<inst[i].arrival<<" deadline: "<<inst[i].deadline<<" computation time: "<<inst[i].computation_time<<endl;
	 }*/

	int start = 0;
	while (inst.size() > 0) {
		int inversion_at = tasksets[0].hyperperiod;
		bool taskadded = false;
		for (unsigned int i = 0; i < inst.size(); i++) {
			if (inst[i].arrival > start) {
				inversion_at =
						inst[i].arrival < inversion_at ?
								inst[i].arrival : inversion_at;
			} else {
				if (start + inst[i].computation_time <= inversion_at) {
					s.start = start;
					s.end = start + inst[i].computation_time;
					s.task_id = inst[i].task_id;
					inst.erase(inst.begin() + i);
				} else {
					s.start = start;
					s.end = inversion_at;
					s.task_id = inst[i].task_id;
					inst[i].computation_time = inst[i].computation_time
							+ s.start - s.end;
				}
				sch.push_back(s);
				start = s.end;
				taskadded = true;
				break;
			}
		}
		if (!taskadded) {
			start = inversion_at;
		}
	}
	int total_computations = 0;
	for (unsigned int i = 0; i < tasks->size(); i++) {
		total_computations = total_computations + computations[i];
	}

	(*tid) = sch[sch.size() - 1].task_id;
	slack = tasksets[0].hyperperiod - sch[sch.size() - 1].end - start_time;

	int common_deadline=(start_time/(*tasks)[(*tid)].period+1)*(*tasks)[(*tid)].period;

	if(slack==0)
	{
//		cout<<"0 slack encountered"<<endl;
		assert((*possible_tasks)[0]==-1);
		possible_tasks->erase(possible_tasks->begin());
		for(unsigned int i=0;i<possible_tasks->size();i++)
		{
			int current_deadline=(start_time/(*tasks)[(*possible_tasks)[i]].period+1)*(*tasks)[(*possible_tasks)[i]].period;
			if(current_deadline!=common_deadline)
			{
				possible_tasks->erase(possible_tasks->begin()+i);
				i=i-1;
			}
		}
	}

	if (slack < 0) {
		cout << "error in slack" << endl;
//*********************************recomputing instances**********************************************************************
		for (unsigned int i = 0; i < tasks->size(); i++) {
			int start = computations[i] / (*tasks)[i].computation_time
					* (*tasks)[i].period;
			temp_inst.computation_time = (*tasks)[i].computation_time
					- computations[i] % (*tasks)[i].computation_time;
			temp_inst.arrival = start
					+ computations[i] % (*tasks)[i].computation_time;
			temp_inst.deadline = start + (*tasks)[i].period;
			temp_inst.task_id = i;
			if (temp_inst.computation_time > 0
					&& temp_inst.arrival < tasksets[0].hyperperiod) {
				inst.push_back(temp_inst);
				start = temp_inst.deadline;
			}
			while (start < tasksets[0].hyperperiod) {
				temp_inst.computation_time = (*tasks)[i].computation_time;
				temp_inst.arrival = start;
				temp_inst.deadline = start + (*tasks)[i].period;
				start = temp_inst.deadline;
				temp_inst.task_id = i;
				inst.push_back(temp_inst);
			}
		}
//****************************************************************************************************************************

		cout << "instance size " << inst.size() << endl;

		for (unsigned int i = 0; i < sch.size(); i++) {
			cout << "task " << sch[i].task_id << " start " << sch[i].start
					<< " end " << sch[i].end << endl;
		}

		cout << " start of evaluation " << start_time << " returned slack "
				<< slack << endl;
		exit(1);
	}

/*	if(slack==0)
	{
		cout<<" minimum deadline task "<<(*tid)<<endl;
		cout << "task " << sch[sch.size()-2].task_id << " start " << sch[sch.size()-2].start
				<< " end " << sch[sch.size()-2].end << endl;
		cout << "task " << sch[sch.size()-1].task_id << " start " << sch[sch.size()-1].start
				<< " end " << sch[sch.size()-1].end << endl;

	}*/


	sch.clear();
	inst.clear();


	return (slack);

}



