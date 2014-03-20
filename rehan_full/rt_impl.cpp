/*
 * rt_impl.cpp
 *
 *  Created on: Jun 18, 2013
 *      Author: rehan
 */


#include "scheduler.h"




extern vector<task>*int_pointer;
extern vector<taskset>tasksets;


extern int running_tasks[NUM_PROCESSORS];
timespec schedule_start;
float scheduling_overhead;

int *next_arrival;

void init_all(vector<task>*tasks)
{
	for(unsigned int i=0;i<tasks->size();i++)
	{
		initialize_task(i, CORE, tasks);
	}
}

float global_power_instance(vector<instance>*run_queue,vector<task>*tasks, int earliness)
{
	float total_comps=0.00;
	float scaled_activity=0.00;
	for(unsigned int i=0;i<run_queue->size();i++)
	{
		total_comps=total_comps+floor((*run_queue)[i].computations);
		scaled_activity=scaled_activity+(floor((*run_queue)[i].computations))*pow((*run_queue)[i].power,1/3.0);
	}

	return(pow((scaled_activity/(total_comps+earliness)),3));
}

float global_power_instance2(vector<comps_req>*comps,vector<task>*tasks, int earliness)
{
	float total_comps=0.00;
	float scaled_activity=0.00;
	for(unsigned int i=0;i<comps->size();i++)
	{
//		float power=(*tasks)[(*comps)[i].id].power*pow((*tasks)[(*comps)[i].id].,3)
		total_comps=total_comps+(*comps)[i].computations;
		scaled_activity=scaled_activity+((*comps)[i].computations)*pow((*tasks)[(*comps)[i].id].power,1/3.0);
	}

	return(pow((scaled_activity/(total_comps+earliness)),3));
}


int instance_maxfirst(vector<instance>*dyn_queue, vector<instance>*run_queue, vector<task>*tasks, int earliness, int start)
{

#if(INST_MF_PRINT)
	cout<<"\n****************starting new instance maxfirst******************"<<endl;
#endif

	vector<int> tid;
	double g_power; //=global_power(tasks,0,temp_tasks->size(),available_util);
	vector<instance>local_queue;

	for(unsigned int i=0;i<run_queue->size();i++)
	{
		if((*run_queue)[i].arrival<=start)
		{
			local_queue.push_back((*run_queue)[i]);
			assert((*run_queue)[i].computations>=0 && (*run_queue)[i].speed>0);
			local_queue[local_queue.size()-1].mapping=i;
		}
	}
	bool resolved=false;
	while (!resolved)
	{
		resolved = true;
		g_power = global_power_instance(&local_queue, tasks, earliness);

#if(ENABLE_PRINTS)
		cout<<"global power:"<<g_power<<"utilization "<<used_util<<endl;
#endif
		for (unsigned int i = 0; i < local_queue.size(); i++)
		{
			float speed = pow(g_power / local_queue[i].power,1 / 3.0);
			float scale_speed=speed;
			speed=speed*local_queue[i].speed;
			if (speed > MAX_SPEED)
			{
				float previous_computations=local_queue[i].computations;
				float new_computations=((float)(previous_computations)*local_queue[i].speed)/MAX_SPEED;
				speed=MAX_SPEED;
#if(INST_MF_PRINT)
				cout<<"task "<<local_queue[i].task_id<<" power "<< (*tasks)[local_queue[i].task_id].power<<" speed "
									<<speed<<" previous computations "<<previous_computations<<" previous speed "<<local_queue[i].speed
									<<" new computations "<< new_computations<<endl;
#endif
				int id=local_queue[i].mapping;

				if((*run_queue)[id].task_id!=(*dyn_queue)[id].task_id)
				{
					cout<<"ERROR DETECTED at "<<id<<endl;
					for(unsigned int x=0;x<(*run_queue).size();x++)
					{
						cout<<x<<":"<<(*run_queue)[x].task_id<<" computations "<<(*run_queue)[x].computations<<"|"<<(*dyn_queue)[x].task_id<<" computations "<<(*dyn_queue)[x].computations<<endl;
					}
					cout<<" run queue size "<<run_queue->size()<<" dynamic queue size "<<dyn_queue->size()<<endl;
					exit(1);
				}

#if(INST_MF_PRINT)
						cout<<"previous. task id "<<(*run_queue)[j].task_id<<" speed "<<(*run_queue)[j].speed<<" computations "
								<<(*run_queue)[j].computations<<" computation time "<<(*run_queue)[j].computation_time<<" power "<<(*run_queue)[j].power<<endl;
#endif
						(*run_queue)[id].speed=speed;
						(*run_queue)[id].computations=new_computations;
						(*run_queue)[id].power=(*run_queue)[id].power*pow(scale_speed,3);

						(*dyn_queue)[id].computations=(*dyn_queue)[id].comps_left/speed;//floor(((float)((*dyn_queue)[j].computations)*(*dyn_queue)[j].speed)/newspeed);
						(*dyn_queue)[id].speed=speed;
						(*dyn_queue)[id].power=(*run_queue)[id].power;
						earliness=earliness-ceil(new_computations-previous_computations);
#if(INST_MF_PRINT)
						cout<<"new. task id "<<(*run_queue)[id].task_id<<" speed "<<(*run_queue)[id].speed<<"|"<<(*dyn_queue)[id].speed<<" computations "
								<<(*run_queue)[id].computations<<"|"<<(*dyn_queue)[id].computations<<" computation time "
								<<(*run_queue)[id].computation_time<<"|"<<(*dyn_queue)[id].computation_time<<endl;
#endif
				local_queue.erase(local_queue.begin()+i);
				i=i-1;
				resolved = false;
				break;
			}
		}
	}
	resolved = false;
	while (!resolved)
	{
		resolved = true;
		g_power = global_power_instance(&local_queue, tasks, earliness);
#if(ENABLE_PRINTS)
		cout<<"global power:"<<g_power<<"utilization "<<used_util<<endl;
#endif

		for (unsigned int i = 0; i < local_queue.size(); i++)
		{
			float speed = pow(g_power / local_queue[i].power,1 / 3.0);
			float scale_speed=speed;
			speed=speed*local_queue[i].speed;
			if (speed <MIN_SPEED)
			{
				int previous_computations=local_queue[i].computations;
				int new_computations=((float)(previous_computations)*local_queue[i].speed)/MIN_SPEED;
				speed=MIN_SPEED;
#if(INST_MF_PRINT)
				cout<<"task "<<local_queue[i].task_id<<" power "<< (*tasks)[local_queue[i].task_id].power<<" speed "
									<<speed<<" previous computations "<<previous_computations<<" previous speed "<<local_queue[i].speed
									<<" new computations "<< new_computations<<endl;
	//			cout<<"iteration "<<i<<":"<<local_queue.size()<<endl;
#endif
				int id=local_queue[i].mapping;
#if(INST_MF_PRINT)
						cout<<"previous. task id "<<(*run_queue)[id].task_id<<" speed "<<(*run_queue)[id].speed<<" computations "
								<<(*run_queue)[id].computations<<" computation time "<<(*run_queue)[id].computation_time<<" power "<<(*run_queue)[id].power<<endl;
#endif
				if((*run_queue)[id].task_id!=(*dyn_queue)[id].task_id)
				{
					cout<<"ERROR DETECTED at "<<id<<endl;
					for(unsigned int x=0;x<(*run_queue).size();x++)
					{
						cout<<x<<":"<<(*run_queue)[x].task_id<<" computations "<<(*run_queue)[x].computations<<"|"<<(*dyn_queue)[x].task_id<<" computations "<<(*dyn_queue)[x].computations<<endl;
					}
					cout<<" run queue size "<<run_queue->size()<<" dynamic queue size "<<dyn_queue->size()<<endl;

					exit(1);
				}

				(*run_queue)[id].speed=speed;
				(*run_queue)[id].computations=new_computations;
				(*run_queue)[id].power=(*run_queue)[id].power*pow(scale_speed,3);

				(*dyn_queue)[id].computations=(*dyn_queue)[id].comps_left/speed;//floor(((float)((*dyn_queue)[j].computations)*(*dyn_queue)[j].speed)/newspeed);
				(*dyn_queue)[id].speed=speed;
				(*dyn_queue)[id].power=(*run_queue)[id].power;
				earliness=earliness-ceil(new_computations-previous_computations);

				assert(previous_computations<=new_computations);
#if(INST_MF_PRINT)
				cout<<"new. task id "<<(*run_queue)[j].task_id<<" speed "<<(*run_queue)[j].speed<<"|"<<(*dyn_queue)[j].speed<<" computations "
												<<(*run_queue)[j].computations<<"|"<<(*dyn_queue)[j].computations<<" computation time "
												<<(*run_queue)[j].computation_time<<"|"<<(*dyn_queue)[j].computation_time<<endl;
#endif

				local_queue.erase(local_queue.begin()+i);
				i=i-1;
				resolved = false;
				break;
			}
		}
	}
	g_power = global_power_instance(&local_queue, tasks, earliness);

	for (unsigned int i = 0; i < local_queue.size(); i++) {
	float speed = pow(g_power / local_queue[i].power,1 / 3.0);
	float scale_speed=speed;
	speed=speed*local_queue[i].speed;
	float previous_computations=local_queue[i].computations;
	float new_computations=(previous_computations)*local_queue[i].speed/speed;
#if(INST_MF_PRINT)
		cout<<"task "<<local_queue[i].task_id<<" power "<< (*tasks)[local_queue[i].task_id].power<<" speed "
							<<speed<<" previous computations "<<previous_computations<<" previous speed "<<local_queue[i].speed
							<<" new computations "<< new_computations<<endl;
#endif
		int id=local_queue[i].mapping;
#if(INST_MF_PRINT)
				cout<<"previous. task id "<<(*run_queue)[j].task_id<<" speed "<<(*run_queue)[j].speed<<" computations "
						<<(*run_queue)[j].computations<<" computation time "<<(*run_queue)[j].computation_time<<" power "<<(*run_queue)[j].power<<endl;
#endif
		if((*run_queue)[id].task_id!=(*dyn_queue)[id].task_id)
		{
			cout<<"ERROR DETECTED at "<<id<<endl;
			for(unsigned int x=0;x<(*run_queue).size();x++)
			{
				cout<<x<<":"<<(*run_queue)[x].task_id<<" computations "<<(*run_queue)[x].computations<<"|"<<(*dyn_queue)[x].task_id<<" computations "<<(*dyn_queue)[x].computations<<endl;
			}
			cout<<" run queue size "<<run_queue->size()<<" dynamic queue size "<<dyn_queue->size()<<endl;

			exit(1);
		}

		(*run_queue)[id].speed=speed;
		(*run_queue)[id].computations=new_computations;
		(*run_queue)[id].power=(*run_queue)[id].power*pow(scale_speed,3);
		(*dyn_queue)[id].computations=(*dyn_queue)[id].comps_left/speed;//floor(((float)((*dyn_queue)[j].computations)*(*dyn_queue)[j].speed)/newspeed);
		(*dyn_queue)[id].speed=speed;
		(*dyn_queue)[id].power=(*run_queue)[id].power;
		earliness=earliness-ceil(new_computations-previous_computations);
#if(INST_MF_PRINT)
		cout<<"new. task id "<<(*run_queue)[j].task_id<<" speed "<<(*run_queue)[j].speed<<"|"<<(*dyn_queue)[j].speed<<" computations "
										<<(*run_queue)[j].computations<<"|"<<(*dyn_queue)[j].computations<<" computation time "
										<<(*run_queue)[j].computation_time<<"|"<<(*dyn_queue)[j].computation_time<<endl;
#endif
		local_queue.erase(local_queue.begin()+i);
		i=i-1;
	}
	return(earliness);
}


int instance_maxfirst(vector<comps_req>*comps,vector<schedule>*edf, float *speeds,
									vector<task>*tasks, int earliness, int start)
{

#if(INST_MF_PRINT)
	cout<<"\n****************starting new instance maxfirst******************"<<endl;
#endif

	vector<int> tid;
	double g_power; //=global_power(tasks,0,temp_tasks->size(),available_util);
	vector<comps_req>local_queue;

	instance temp;

	for(unsigned int i=0;i<comps->size();i++)
	{
		if((*comps)[i].arrival<=start)
		{
			local_queue.push_back((*comps)[i]);
		}
	}
	bool resolved=false;
	while (!resolved)
	{
		resolved = true;
		g_power = global_power_instance2(&local_queue, tasks, earliness);

#if(ENABLE_PRINTS)
		cout<<"global power:"<<g_power<<"utilization "<<used_util<<endl;
#endif
		for (unsigned int i = 0; i < local_queue.size(); i++)
		{
			float base_power=(*tasks)[local_queue[i].id].power/pow(speeds[local_queue[i].id],3);
			float speed = pow(g_power / base_power,1 / 3.0);
			if (speed > MAX_SPEED)
			{
				float previous_computations=local_queue[i].computations;
				float new_computations=ceil(local_queue[i].comps);
				speed=local_queue[i].comps/ceil(local_queue[i].comps);
#if(INST_MF_PRINT)
				cout<<"task "<<local_queue[i].task_id<<" power "<< (*tasks)[local_queue[i].task_id].power<<" speed "
									<<speed<<" previous computations "<<previous_computations<<" previous speed "<<local_queue[i].speed
									<<" new computations "<< new_computations<<endl;
#endif

#if(INST_MF_PRINT)
						cout<<"previous. task id "<<(*run_queue)[j].task_id<<" speed "<<(*run_queue)[j].speed<<" computations "
								<<(*run_queue)[j].computations<<" computation time "<<(*run_queue)[j].computation_time<<" power "<<(*run_queue)[j].power<<endl;
#endif
						(*comps)[local_queue[i].mapping].speed=speed;
//						(*run_queue)[id].computations=new_computations;
						earliness=earliness-round(new_computations-previous_computations);
#if(INST_MF_PRINT)
						cout<<"new. task id "<<(*run_queue)[id].task_id<<" speed "<<(*run_queue)[id].speed<<"|"<<(*dyn_queue)[id].speed<<" computations "
								<<(*run_queue)[id].computations<<"|"<<(*dyn_queue)[id].computations<<" computation time "
								<<(*run_queue)[id].computation_time<<"|"<<(*dyn_queue)[id].computation_time<<endl;
#endif
				local_queue.erase(local_queue.begin()+i);
				i=i-1;
				resolved = false;
				break;
			}
		}
	}
	resolved = false;
	while (!resolved)
	{
		resolved = true;
		g_power = global_power_instance2(&local_queue, tasks, earliness);
#if(ENABLE_PRINTS)
		cout<<"global power:"<<g_power<<"utilization "<<used_util<<endl;
#endif

		for (unsigned int i = 0; i < local_queue.size(); i++)
		{
			float base_power=(*tasks)[local_queue[i].id].power/pow(speeds[local_queue[i].id],3);
			float speed = pow(g_power / base_power,1 / 3.0);

			if (speed <MIN_SPEED)
			{
				float previous_computations=local_queue[i].computations;
				float new_computations=floor(local_queue[i].comps/MIN_SPEED);
				speed=previous_computations/new_computations;

#if(INST_MF_PRINT)
				cout<<"task "<<local_queue[i].task_id<<" power "<< (*tasks)[local_queue[i].task_id].power<<" speed "
									<<speed<<" previous computations "<<previous_computations<<" previous speed "<<local_queue[i].speed
									<<" new computations "<< new_computations<<endl;
	//			cout<<"iteration "<<i<<":"<<local_queue.size()<<endl;
#endif

#if(INST_MF_PRINT)
						cout<<"previous. task id "<<(*run_queue)[id].task_id<<" speed "<<(*run_queue)[id].speed<<" computations "
								<<(*run_queue)[id].computations<<" computation time "<<(*run_queue)[id].computation_time<<" power "<<(*run_queue)[id].power<<endl;
#endif

				(*comps)[local_queue[i].mapping].speed=speed;
				earliness=earliness-round(new_computations-previous_computations);

				assert(previous_computations<=new_computations);
#if(INST_MF_PRINT)
				cout<<"new. task id "<<(*run_queue)[j].task_id<<" speed "<<(*run_queue)[j].speed<<"|"<<(*dyn_queue)[j].speed<<" computations "
												<<(*run_queue)[j].computations<<"|"<<(*dyn_queue)[j].computations<<" computation time "
												<<(*run_queue)[j].computation_time<<"|"<<(*dyn_queue)[j].computation_time<<endl;
#endif

				local_queue.erase(local_queue.begin()+i);
				i=i-1;
				resolved = false;
				break;
			}
		}
	}
	g_power = global_power_instance2(&local_queue, tasks, earliness);

	for (unsigned int i = 0; i < local_queue.size(); i++) {
		float base_power=(*tasks)[local_queue[i].id].power/pow(speeds[local_queue[i].id],3);
		float speed = pow(g_power / base_power,1 / 3.0);
		float previous_computations=local_queue[i].computations;
		float new_computations=floor(local_queue[i].comps/speed);
		speed=previous_computations/new_computations;
#if(INST_MF_PRINT)
		cout<<"task "<<local_queue[i].task_id<<" power "<< (*tasks)[local_queue[i].task_id].power<<" speed "
							<<speed<<" previous computations "<<previous_computations<<" previous speed "<<local_queue[i].speed
							<<" new computations "<< new_computations<<endl;
#endif
		int id=local_queue[i].mapping;
#if(INST_MF_PRINT)
				cout<<"previous. task id "<<(*run_queue)[j].task_id<<" speed "<<(*run_queue)[j].speed<<" computations "
						<<(*run_queue)[j].computations<<" computation time "<<(*run_queue)[j].computation_time<<" power "<<(*run_queue)[j].power<<endl;
#endif

		(*comps)[local_queue[i].mapping].speed=speed;
		earliness=earliness-round(new_computations-previous_computations);
#if(INST_MF_PRINT)
		cout<<"new. task id "<<(*run_queue)[j].task_id<<" speed "<<(*run_queue)[j].speed<<"|"<<(*dyn_queue)[j].speed<<" computations "
										<<(*run_queue)[j].computations<<"|"<<(*dyn_queue)[j].computations<<" computation time "
										<<(*run_queue)[j].computation_time<<"|"<<(*dyn_queue)[j].computation_time<<endl;
#endif
		local_queue.erase(local_queue.begin()+i);
		i=i-1;
	}
	return(earliness);
}

void dynamic_instances(vector<task>*tasks,float* speeds ,vector<instance>*inst)
{
//	std::default_random_engine generator;
//	std::exponential_distribution<double> distribution(1.42);
	instance temp;
	for(int i=0;i<tasks->size();i++)
	{
		int start=0;
		temp.task_id=i;
		while(start<tasksets[0].hyperperiod)
		{
		//	double number=distribution(generator);
		//	number=number>1.0?1.0:number;
			temp.computation_time=myceil(((float)(*tasks)[i].computation_time)*0.7);
			temp.computations=temp.computation_time;
			temp.comps_left=round(float(temp.computation_time)*speeds[temp.task_id]);
			temp.computation_time=temp.computation_time*speeds[temp.task_id];
			temp.comps_done=0.00;

			temp.arrival=start;
			temp.deadline=(*tasks)[i].period+temp.arrival;
			temp.speed=speeds[temp.task_id];
			temp.power=(*tasks)[temp.task_id].power;
			start=temp.deadline;
			inst->push_back(temp);
		}
	}
	sort(inst->begin(),inst->end(),sort_inst_arrival);

	cout<<"dynamic instances after generation"<<endl;

	cout<<"printing task set"<<endl;
	for(unsigned int i=0;i<tasks->size();i++)
	{
		cout<<"task "<<i<<" computations time "<<(*tasks)[i].computation_time<<" period "<<(*tasks)[i].period<<
				" speed "<<speeds[i]<<" unscaled computations "<<(*tasks)[i].computation_time*speeds[i]<<" power "<<(*tasks)[i].power<<endl;
	}
}


int dispatcher(vector<schedule>*sch,vector<instance>*inst, vector<schedule>*o_sch)
{
	cout<<"Dynamic Schedule "<<endl;
	for(unsigned int i=0;i<sch->size();i++)
	{
//		cout<<"task "<<(*sch)[i].task_id<<" start "<<(*sch)[i].start<<" end "<<(*sch)[i].end<<" speed "<<(*sch)[i].speed<<endl;
	}

	vector<schedule>c_sch;
	for(unsigned int i=0;i<sch->size();i++)
	{
		for(unsigned int j=0;j<inst->size();j++)
		{
			if((*inst)[j].task_id==(*sch)[i].task_id && (*inst)[j].arrival<=(*sch)[i].start)
			{
				if((((float)((*sch)[i].end-(*sch)[i].start)*(*sch)[i].speed))>=(*inst)[j].comps_left)
				{
					c_sch.push_back((*sch)[i]);
					c_sch[c_sch.size()-1].end=(*sch)[i].start+myceil((*inst)[j].comps_left/(*sch)[i].speed);
					c_sch[c_sch.size()-1].power=(*inst)[j].power;
					inst->erase(inst->begin()+j);
					cout<<"removed id "<<(*sch)[i].task_id<<" start "<<(*sch)[i].start<<" end "<<(*sch)[i].end<<" speed "<<(*sch)[i].speed<<endl;
				}
				else
				{
					c_sch.push_back((*sch)[i]);
					c_sch[c_sch.size()-1].power=(*inst)[j].power;
					(*inst)[j].comps_left=(*inst)[j].comps_left-((float)((*sch)[i].end-(*sch)[i].start))*(*sch)[i].speed;
					(*inst)[j].comps_done=(*inst)[j].comps_done+((float)((*sch)[i].end-(*sch)[i].start))*(*sch)[i].speed;
					cout<<"id "<<(*sch)[i].task_id<<" start "<<(*sch)[i].start<<" end "<<(*sch)[i].end<<" speed "<<(*sch)[i].speed<<endl;
					cout<<"unscaled computations left "<<(*inst)[j].comps_left<<endl;
				}
				break;
			}
		}
	}

	for(unsigned int i=1;i<c_sch.size();i++)
	{
		if(c_sch[i].start>c_sch[i-1].end)
		{
			int computations=c_sch[i].end-c_sch[i].start;
			c_sch[i].start=c_sch[i].arrival>c_sch[i-1].end?c_sch[i].arrival:c_sch[i-1].end;
			c_sch[i].end=c_sch[i].start+computations;
		}
	}

	cout<<"printing cschaedule"<<endl;
	for(unsigned int i=0;i<c_sch.size();i++)
	{
		o_sch->push_back(c_sch[i]);
		cout<<"id "<<c_sch[i].task_id<<" start "<<c_sch[i].start<<" end "<<c_sch[i].end<<endl;
	}



	int schedule_end=c_sch.size()>0?c_sch[c_sch.size()-1].end:-1;

	c_sch.clear();

	return(schedule_end);

}


int dispatcher2(vector<schedule>*sch,vector<instance>*inst, vector<schedule>*o_sch)
{
	cout<<"Dynamic Schedule "<<endl;
	vector<schedule>c_sch;
	for(unsigned int i=0;i<sch->size();i++)
	{
		for(unsigned int j=0;j<inst->size();j++)
		{
			if((*inst)[j].task_id==(*sch)[i].task_id && (*inst)[j].arrival<=(*sch)[i].start)
			{
				if((round((float)((*sch)[i].end-(*sch)[i].start)*(*sch)[i].speed))-(*inst)[j].comps_left>0.01)
				{
					c_sch.push_back((*sch)[i]);
					c_sch[c_sch.size()-1].end=(*sch)[i].start+myceil((*inst)[j].comps_left/(*sch)[i].speed);
					c_sch[c_sch.size()-1].power=((*inst)[j].power/pow((*inst)[j].speed,3))*pow((*sch)[i].speed,3);
					inst->erase(inst->begin()+j);
					cout<<"removed id "<<(*sch)[i].task_id<<" start "<<(*sch)[i].start<<" end "<<(*sch)[i].end<<" speed "<<(*sch)[i].speed<<endl;
				}
				else
				{
					c_sch.push_back((*sch)[i]);
					c_sch[c_sch.size()-1].power=((*inst)[j].power/pow((*inst)[j].speed,3))*pow((*sch)[i].speed,3);
					(*inst)[j].comps_left=(*inst)[j].comps_left-((float)((*sch)[i].end-(*sch)[i].start))*(*sch)[i].speed;
					(*inst)[j].comps_done=(*inst)[j].comps_done+((float)((*sch)[i].end-(*sch)[i].start))*(*sch)[i].speed;
					cout<<"id "<<(*sch)[i].task_id<<" start "<<(*sch)[i].start<<" end "<<(*sch)[i].end<<" speed "<<(*sch)[i].speed<<endl;
					cout<<"unscaled computations left "<<(*inst)[j].comps_left<<endl;
				}
				break;
			}
		}
	}

	for(unsigned int i=1;i<c_sch.size();i++)
	{
		if(c_sch[i].start>c_sch[i-1].end)
		{
			int computations=c_sch[i].end-c_sch[i].start;
			c_sch[i].start=c_sch[i].arrival>c_sch[i-1].end?c_sch[i].arrival:c_sch[i-1].end;
			c_sch[i].end=c_sch[i].start+computations;
		}
	}

	cout<<"printing cschaedule"<<endl;
	for(unsigned int i=0;i<c_sch.size();i++)
	{
		o_sch->push_back(c_sch[i]);
		cout<<"id "<<c_sch[i].task_id<<" start "<<c_sch[i].start<<" end "<<c_sch[i].end<<endl;
	}
	int schedule_end=c_sch.size()>0?c_sch[c_sch.size()-1].end:-1;
	c_sch.clear();
	return(schedule_end);
}



void computations_required(vector<comps_req> * comps,vector<schedule>*partial, vector<schedule>*edf,int start,int end)
{
	partial->clear();
	comps_req temp;
	temp.start=start;
	temp.end=end;
	while((*edf)[0].start<end)
	{

		if(next_arrival[(*edf)[0].task_id]>(*edf)[0].arrival)
		{
			edf->erase(edf->begin());
			if(edf->size()<1)
			{
				break;
			}
		}
		else{
			temp.id=(*edf)[0].task_id;
			temp.arrival=(*edf)[0].arrival;
			temp.mapping=comps->size();
			temp.speed=(*edf)[0].speed;

			if((*edf)[0].end<=end)
			{
				temp.comps=((float)((*edf)[0].end)-(*edf)[0].start)*(*edf)[0].speed;
				temp.computations=(*edf)[0].end-(*edf)[0].start;
				partial->push_back((*edf)[0]);
				edf->erase(edf->begin());
				comps->push_back(temp);
				if(edf->size()<1)
				{
					break;
				}
			}
			else
			{
				temp.comps=((float)(end-(*edf)[0].start))*(*edf)[0].speed;
				temp.computations=end-(*edf)[0].start;
				partial->push_back((*edf)[0]);
				(*partial)[partial->size()-1].end=end;
				(*edf)[0].start=end;
				comps->push_back(temp);
				break;
			}
		}
	}
}


void scheduler(vector<schedule>*o_sch,vector<task>*tasks,vector<instance>*dyn_inst, float* speeds, int period)
{
	vector<float> scheduler_times;
	vector<float> maxfirst_times;
	vector<float> inst_times;
	vector<float> schgen_times;




	int uscaled_computations[tasks->size()];
	int done_computations[tasks->size()];
	int worst_computations[tasks->size()];
//	int worst_total[tasks->size()];
	for(unsigned int i=0;i<tasks->size();i++)
	{
		uscaled_computations[i]=0;
		done_computations[i]=0;
		worst_computations[i]=0;
	//	worst_total[i]=tasksets[0].hyperperiod/(*tasks)[i].period*round((((float)(*tasks)[i].computation_time)*speeds[i]));
	}

	for(unsigned int i=0;i<dyn_inst->size();i++)
	{
		uscaled_computations[(*dyn_inst)[i].task_id]=uscaled_computations[(*dyn_inst)[i].task_id]+round((float)((*dyn_inst)[i].computations)*(*dyn_inst)[i].speed);
	}

	for(unsigned int i=0;i<tasks->size();i++)
	{
		cout<<"Task "<<i<<" unscaled computations "<<uscaled_computations[i]<<endl;
	}

	vector<instance>worst_inst;
	vector<instance>scale_inst;
	vector<schedule>sch;
	schedule temp;

	int inst=0;
	int start=0;
	int end=10000;
	cout<<" entering while loop"<<endl;
	start=inst*period;

	struct timespec start_time,start_time2,end_time;

	int earliness=0;
	clock_gettime(1,&start_time);

	while(start<tasksets[0].hyperperiod)
	{
		end=(inst+1)*period;
		cout<<"START "<<start<<" END "<<end<<" INSTANCE "<<inst<<endl;
		inst=inst+1;

		clock_gettime(1,&start_time2);
		partial_instances(tasks,&worst_inst,dyn_inst,speeds,start,end);  //finding instances that arrive between start and end

		cout<<"Worst case instances size "<<worst_inst.size()<<" hyperperiod "<<tasksets[0].hyperperiod<<endl;
		for(unsigned int i=0;i<worst_inst.size();i++)
		{
			cout<<i<<": id "<<worst_inst[i].task_id<<" computations "<<worst_inst[i].computations<<" speed "<<worst_inst[i].speed<<" arrival "<<worst_inst[i].arrival<<endl;
		}

		sort(worst_inst.begin(),worst_inst.end(),sort_inst_arrival);
		clock_gettime(1,&end_time);
		inst_times.push_back(time_diff(&start_time2,&end_time));

		for(unsigned int i=0;i<worst_inst.size();i++)
		{
			if(worst_inst[i].computations==0)
			{
				worst_inst.erase(worst_inst.begin()+i);
			}
		}

		for(unsigned int i=0;i<worst_inst.size();i++)
		{
			if((*dyn_inst)[i].computations==0)
			{
				dyn_inst->erase(dyn_inst->begin()+i);
			}
		}

		earliness=worst_inst[0].arrival>start?0:earliness;//earliness set to zero if there are no tasks to execute
		clock_gettime(1,&start_time2);

		cout<<"earliness: "<<earliness<<""<<endl;
		if(earliness>0)
		{
			earliness=instance_maxfirst(dyn_inst,&worst_inst, tasks, earliness,start);//maxfirst called to scale tasks
		}

		clock_gettime(1,&end_time);
		maxfirst_times.push_back(time_diff(&start_time2,&end_time));
		start=worst_inst[0].arrival>start?worst_inst[0].arrival:start;//correcting value of start

		clock_gettime(1,&start_time2);

		while(start<end && worst_inst.size()>0)
		{
			int index=0;
			int inversion_at=end;
			unsigned int i;
			for(i=0;i<worst_inst.size() && worst_inst[i].arrival<=start;i++)
			{
				if(worst_inst[i].deadline<worst_inst[index].deadline)
				{
					index=i;
				}
			}
			for(;i<worst_inst.size();i++)
			{
				if(worst_inst[i].deadline<worst_inst[index].deadline)
				{
					inversion_at=worst_inst[i].arrival;
					break;
				}
			}

			temp.task_id=worst_inst[index].task_id;
			temp.start=start;
			temp.speed=worst_inst[index].speed;
			temp.arrival=worst_inst[index].arrival;

			if(start+worst_inst[index].computations<=inversion_at)
			{
				cout<<"current size "<<worst_inst.size()<<endl;
				temp.end=start+worst_inst[index].computations;
				worst_inst.erase(worst_inst.begin()+index);
			}
			else
			{
				temp.end=inversion_at;
				worst_inst[index].computations=worst_inst[index].computations-(temp.end-temp.start);
			}
			start=temp.end>=worst_inst[0].arrival?temp.end:worst_inst[0].arrival;
			sch.push_back(temp);
			worst_computations[temp.task_id]=worst_computations[temp.task_id]+round(((float)(temp.end-temp.start))*temp.speed);
		}
		clock_gettime(1,&end_time);
		schgen_times.push_back(time_diff(&start_time2,&end_time));

		int temp_start=dispatcher(&sch,dyn_inst,o_sch);//-1 returned if svjedule is empty. End time of schedule entered otherwise
		cout<<"worst schedule **************************************"<<endl;
		for (unsigned int i=0;i<sch.size();i++)
		{
			cout<<"task id "<<sch[i].task_id<<" start "<<sch[i].start<<" end "<<sch[i].end<<" speed "<<sch[i].speed<<endl;
		}

		for(unsigned int i=0;i<tasks->size();i++)
		{
			done_computations[i]=0;
		}
		for(unsigned int i=0;i<o_sch->size();i++)
		{
			done_computations[(*o_sch)[i].task_id]=done_computations[(*o_sch)[i].task_id]+round(((float)((*o_sch)[i].end-(*o_sch)[i].start))*(*o_sch)[i].speed);
		}
		scheduler_times.push_back(time_diff(&start_time,&end_time));
		clock_gettime(1,&start_time);
		start=temp_start>=0?temp_start:inst*period;
		earliness=(inst*period-start);
		cout<<"earliness at end "<<earliness<<endl;
		assert(earliness>=0);
		sch.clear();
		worst_inst.clear();
	}

	cout<<" schedule size "<<o_sch->size()<<endl;
	for(unsigned int i=0;i<o_sch->size();i++)
	{
		cout<<"task "<<(*o_sch)[i].task_id<<" start "<<(*o_sch)[i].start<<" end "<<(*o_sch)[i].end<<" speed "<<(*o_sch)[i].speed<<endl;
	}

	for(unsigned int i=0;i<scheduler_times.size();i++)
	{
		cout<<"iteration "<<i<<" total time "<<scheduler_times[i]<<" ms"<<" maxfirst_time "<<maxfirst_times[i]<<" inst_gen time "
				<<inst_times[i]<<" schgen time "<<schgen_times[i]<<endl;
	}
	int scaled_computations[tasks->size()];
	for(unsigned int i=0;i<tasks->size();i++)
	{
		scaled_computations[i]=0;
	}


	int total=0;
	for(unsigned int i=0;i<o_sch->size();i++)
	{
		scaled_computations[(*o_sch)[i].task_id]=scaled_computations[(*o_sch)[i].task_id]+((*o_sch)[i].end-(*o_sch)[i].start)*(*o_sch)[i].speed;
		total=total+(*o_sch)[i].end-(*o_sch)[i].start;
	}

	for(unsigned int i=0;i<tasks->size();i++)
	{
		cout<<"task" <<i<<" computations performed="<<scaled_computations[i]<<" unscaled computations="<<uscaled_computations[i]<<endl;
	}

	cout<<"total computations="<<total<<endl;

}

void scheduler2(vector<schedule>*o_sch,vector<task>*tasks, 	vector<instance>*dyn_inst, float* speeds, int period)
{
	vector<float> scheduler_times;
	vector<float> maxfirst_times;
	vector<float> inst_times;
	vector<float> schgen_times;

	vector<schedule>edf;
	vector<schedule>worst_sch;
	vector<schedule>partial_schedule;
	edf_schedule(tasks,&edf);

	int edf_total=0;
	for(unsigned int i=0;i<edf.size();i++)
	{
		edf_total=edf_total+edf[i].end-edf[i].start;
	}

	int dynamic_total=0;
	for(unsigned int i=0;i<edf.size();i++)
	{
		dynamic_total=dynamic_total+(*dyn_inst)[i].computations;
	}



	int uscaled_computations[tasks->size()];
	float done_computations[tasks->size()];
//	int worst_total[tasks->size()];
	for(unsigned int i=0;i<tasks->size();i++)
	{
		uscaled_computations[i]=0;
		done_computations[i]=0;
	//	worst_total[i]=tasksets[0].hyperperiod/(*tasks)[i].period*round((((float)(*tasks)[i].computation_time)*speeds[i]));
	}

	for(unsigned int i=0;i<dyn_inst->size();i++)
	{
		uscaled_computations[(*dyn_inst)[i].task_id]=uscaled_computations[(*dyn_inst)[i].task_id]+round((float)((*dyn_inst)[i].computations)*(*dyn_inst)[i].speed);
	}


	next_arrival=new int[tasks->size()];

	for(unsigned int i=0;i<tasks->size();i++)
	{
		next_arrival[i]=0;
	}


	cout<<"printing base edf schedule"<<endl;

	for(unsigned int i=0;i<edf.size();i++)
	{
		edf[i].speed=speeds[edf[i].task_id];
		cout<<"task "<<edf[i].task_id<<" start "<<edf[i].start<<" end "<<
				edf[i].end<<" speed "<<edf[i].speed<<" arrival "<<edf[i].arrival<<endl;
	}

	vector<instance>worst_inst;
	vector<instance>scale_inst;
//	schedule temp;


	int inst=0;
	int start=0;
	int end=10000;
	cout<<" entering while loop"<<endl;
	start=inst*period;

	struct timespec start_time,start_time2,end_time;

	int earliness=0;
	clock_gettime(1,&start_time);

	vector<comps_req> comps;

	while((inst*period)<tasksets[0].hyperperiod && dyn_inst->size()>0)
	{
		end=(inst+1)*period;
		cout<<"START "<<start<<" END "<<end<<" INSTANCE "<<inst<<endl;
		inst=inst+1;

		computations_required(&comps,&partial_schedule, &edf,start,end);

//		for(unsigned int i=0;i<comps.size();i++)
//		{
//			cout<<"task "<<comps[i].id<<" computations "<<comps[i].computations<<" arrival "<<comps[i].arrival<<endl;
//		}

		if(earliness>0)
		{
	//		cout<<"calling instance maxfirst"<<endl;
			instance_maxfirst(&comps,&edf, speeds,tasks, earliness, start);
		}

//		cout<<"returned from instance maxfirst"<<endl;

		schedule temp;
		int id=0;
		int local_start=start;
		while(id<partial_schedule.size() && partial_schedule[id].arrival<=start)
		{
			temp.start=local_start;
			temp.end=local_start+round(comps[id].comps/comps[id].speed);
			if(comps[id].id!=partial_schedule[id].task_id)
			{
				cout<<"discrepancy in partial schedule at id "<<id<<endl;
				for(unsigned int i=0;i<comps.size();i++)
				{
					cout<<"id:"<<comps[i].id<<" arrival "<<comps[i].arrival<<" computations "<<comps[i].computations<<endl;
				}

				cout<<"printing schedule "<<endl;

				for(unsigned int i=0;i<partial_schedule.size();i++)
				{
					cout<<"id:"<<partial_schedule[i].task_id<<" arrival "<<partial_schedule[i].arrival<<" start "<<partial_schedule[i].start<<" end "<<partial_schedule[i].end<<endl;
				}
				exit(1);
			}

			temp.task_id=comps[id].id;
			temp.arrival=comps[id].arrival;
			temp.power=((*tasks)[temp.task_id].power/(pow(speeds[temp.task_id],3)))*pow(comps[id].speed,3);
			temp.speed=comps[id].speed;
			worst_sch.push_back(temp);
			local_start=temp.end;
			id++;

		}

		if(temp.end>partial_schedule[id].start && id<partial_schedule.size())
		{
			cout<<" end "<<temp.end<<" start "<<partial_schedule[id].start<<endl;
			exit(1);
		}
		for(unsigned int i=id;i<partial_schedule.size();i++)
		{
			worst_sch.push_back(partial_schedule[i]);
		}
//		cout<<"\nprinting worst case schedule\n "<<endl;
//		for(unsigned int i=0;i<worst_sch.size();i++)
//		{
//			cout<<"task "<<worst_sch[i].task_id<<" start "<<worst_sch[i].start<<" end "<<worst_sch[i].end<<" speed "<<worst_sch[i].speed<<endl;
//		}

		int temp_start=dispatcher2(&worst_sch,dyn_inst, o_sch);
		start=temp_start>=0?temp_start:inst*period;
		earliness=(inst*period-start);


		comps.clear();
		partial_schedule.clear();
		worst_sch.clear();

//		cout<<"schedule print"<<endl;
//		for(unsigned int i=0;i<edf.size();i++)
//		{
//			edf[i].speed=speeds[edf[i].task_id];
//			cout<<"task "<<edf[i].task_id<<" start "<<edf[i].start<<" end "<<
//					edf[i].end<<" speed "<<edf[i].speed<<" arrival "<<edf[i].arrival<<endl;
//		}


/*		clock_gettime(1,&start_time2);

		earliness=worst_inst[0].arrival>start?0:earliness;//earliness set to zero if there are no tasks to execute
		clock_gettime(1,&start_time2);

		cout<<"earliness: "<<earliness<<""<<endl;
		if(earliness>0)
		{
//			earliness=instance_maxfirst(&dyn_inst,&worst_inst, tasks, earliness,start);//maxfirst called to scale tasks
		}

		clock_gettime(1,&end_time);
		maxfirst_times.push_back(time_diff(&start_time2,&end_time));
		start=worst_inst[0].arrival>start?worst_inst[0].arrival:start;//correcting value of start

		clock_gettime(1,&start_time2);

		while(start<end && worst_inst.size()>0)
		{
			int index=0;
			int inversion_at=end;
			unsigned int i;
			for(i=0;i<worst_inst.size() && worst_inst[i].arrival<=start;i++)
			{
				if(worst_inst[i].deadline<worst_inst[index].deadline)
				{
					index=i;
				}
			}
			for(;i<worst_inst.size();i++)
			{
				if(worst_inst[i].deadline<worst_inst[index].deadline)
				{
					inversion_at=worst_inst[i].arrival;
					break;
				}
			}

			temp.task_id=worst_inst[index].task_id;
			temp.start=start;
			temp.speed=worst_inst[index].speed;
			temp.arrival=worst_inst[index].arrival;

			if(start+worst_inst[index].computations<=inversion_at)
			{
				cout<<"current size "<<worst_inst.size()<<endl;
				temp.end=start+worst_inst[index].computations;
				worst_inst.erase(worst_inst.begin()+index);
			}
			else
			{
				temp.end=inversion_at;
				worst_inst[index].computations=worst_inst[index].computations-(temp.end-temp.start);
			}
			start=temp.end>=worst_inst[0].arrival?temp.end:worst_inst[0].arrival;
			sch.push_back(temp);
		}
		clock_gettime(1,&end_time);
		schgen_times.push_back(time_diff(&start_time2,&end_time));

		int temp_start=dispatcher(&sch,dyn_inst,o_sch);//-1 returned if svjedule is empty. End time of schedule entered otherwise
		cout<<"worst schedule **************************************"<<endl;
		for (unsigned int i=0;i<sch.size();i++)
		{
			cout<<"task id "<<sch[i].task_id<<" start "<<sch[i].start<<" end "<<sch[i].end<<" speed "<<sch[i].speed<<endl;
		}

		scheduler_times.push_back(time_diff(&start_time,&end_time));
		clock_gettime(1,&start_time);
		start=temp_start>=0?temp_start:inst*period;
		earliness=(inst*period-start);
		cout<<"earliness at end "<<earliness<<endl;
		assert(earliness>=0);
		sch.clear();
		worst_inst.clear();*/
	}

	int total=0;
	for(unsigned int i=0;i<o_sch->size();i++)
	{
		done_computations[(*o_sch)[i].task_id]=done_computations[(*o_sch)[i].task_id]+((*o_sch)[i].end-(*o_sch)[i].start)*(*o_sch)[i].speed;
		total=total+(*o_sch)[i].end-(*o_sch)[i].start;
	}

	for(unsigned int i=0;i<tasks->size();i++)
	{
		cout<<"task" <<i<<" computations performed="<<done_computations[i]<<" unscaled computations="<<uscaled_computations[i]<<endl;
	}

	cout<<"total computations="<<total<<" baseline computations "<<dynamic_total<<" edf worst computations "<<edf_total<<endl;

}



void run_schedule(vector<schedule>*sch, vector<task>*tasks)
{

	scheduling_overhead=0;
	cout<<" schedule size inside function "<<sch->size()<<endl;

	cout<<"printing schedule"<<endl;
//	int computations[tasks->size()];


	cout<<" schedule size "<<sch->size()<<endl;
	for(unsigned int i=0;i<sch->size();i++)
	{
		cout<<"task "<<(*sch)[i].task_id<<" start "<<(*sch)[i].start<<" end "<<(*sch)[i].end<<endl;
	}

	cout<<"printed schedule"<<endl;

	timespec sleep_time,current_time;
	clock_gettime(1,&schedule_start);
	schedule_start.tv_sec=schedule_start.tv_sec+2;
	schedule_start.tv_nsec=0;
	sleep_time.tv_nsec=0;
	for(unsigned int i=0;i<sch->size();i++)
	{
		sleep_time.tv_sec=schedule_start.tv_sec + (*sch)[i].start/((int)RT_GRANULARITY);//+(*sch)[i].start;
		sleep_time.tv_nsec=schedule_start.tv_nsec+((float)((*sch)[i].start%((int)RT_GRANULARITY))/RT_GRANULARITY)*1000000000;//multiplied by nsec/sec

		clock_nanosleep(1,1,&sleep_time,NULL);
		run_task2((*sch)[i].task_id, CORE,tasks);
		status_loop((*sch)[i].task_id,tasks,'T');
		clock_gettime(1,&current_time);
		scheduling_overhead=scheduling_overhead+time_diff(&schedule_start,&current_time)-(((float)(*sch)[i].start)/RT_GRANULARITY)*1000 ;//milliseconds

		sleep_time.tv_sec=schedule_start.tv_sec + (*sch)[i].end/((int)RT_GRANULARITY);//+(*sch)[i].start;
		sleep_time.tv_nsec=schedule_start.tv_nsec+((float)((*sch)[i].end%((int)RT_GRANULARITY))/RT_GRANULARITY)*1000000000;//multiplied by nsec/sec
		while(sleep_time.tv_nsec>=1000000000)
		{
			sleep_time.tv_nsec=sleep_time.tv_nsec-1000000000;
			sleep_time.tv_sec++;
		}

		clock_nanosleep(1,1,&sleep_time,NULL);
		p_stop((*sch)[i].task_id,tasks);
		running_tasks[CORE]=-1;
		status_loop((*sch)[i].task_id,tasks,'R');
	}

	for(unsigned int i=0;i<tasks->size();i++)
	{
		p_start(i,tasks);
	}
	clock_gettime(1,&sleep_time);
	sleep_time.tv_sec=sleep_time.tv_sec+2;
	clock_nanosleep(1,1,&sleep_time,NULL);

	for(unsigned int i=0;i<tasks->size();i++)
	{
		cout<<" task "<<i<< "status "<< read_status((*tasks)[i].stat_stream)<<endl;
	}

	cout << "sending terminate signals" << endl;
	for (unsigned int i = 0; i < int_pointer->size(); i++) {
		kill((*int_pointer)[i].pid, SIGINT);
	}

	pid_t childpid;
	for (unsigned int i = 0; i < int_pointer->size(); i++) {
		childpid = waitpid(-1, NULL, 0);
		cout << "successfully ended process " << childpid << endl;
	}

	for(unsigned int i=0;i<tasks->size();i++)
	{
		cout<<" task "<<i<< "status "<< read_status((*tasks)[i].stat_stream)<<endl;
	}

	cout<<"total scheduling overhead "<<scheduling_overhead<<" number of preemptions "<<sch->size()<<endl<<endl;

	clock_gettime(1,&sleep_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);
	clock_gettime(1,&current_time);

	cout<<"time taken to evaluate time "<<time_diff(&sleep_time,&current_time)/50.00<<endl;

}


