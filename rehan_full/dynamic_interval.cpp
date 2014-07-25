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



long end_scheduler(vector<long_schedule> *sch, vector<interval_s>*intervals,vector<long_task>*tasks
		, int interval_index,double* instance_max_comps,long *instance_comps,long *net_comps_done, long *total_worst_comps)
{
	long end_slack=(*intervals)[interval_index].end-(*sch)[sch->size()-1].end;
	if(end_slack<5)
	{
		return end_slack;
	}

	double possible_comps[tasks->size()];

	int task_id=-1;

	for(unsigned int i=0;i<tasks->size();i++)
	{
		if(net_comps_done[i]<(total_worst_comps[i]))
		{
			possible_comps[i]=instance_max_comps[i]-instance_comps[i];
			assert (possible_comps>0);
			if(task_id==-1)
			{
				task_id=i;
			}
			else if((*tasks)[i].power>(*tasks)[task_id].power)
			{
				task_id=i;
			}

		}
		else
		{
			possible_comps[i]=0;
		}
	}

	if(task_id==-1)
	{
		return 0;
	}

	//cout<<" end slack "<<end_slack<<" task "<<task_id<<" selected power consumption "<<(*tasks)[task_id].power<<endl;

	double average_power=0;

	for(unsigned int i=0;i<sch->size();i++)
	{
		if((*sch)[i].start>=(*intervals)[interval_index].start)
		{
			average_power=average_power+(*sch)[i].power*((*sch)[i].float_end-(double)(*sch)[i].start);
		}
	}

	average_power=average_power/((double)((*sch)[sch->size()-1].end-(*intervals)[interval_index].start));
	//cout<<" average power "<<average_power<<endl;

	double speed=pow(average_power/(*tasks)[task_id].power,1.0/3.0);
	speed=speed>MAX_SPEED?MAX_SPEED:speed<MIN_SPEED?MIN_SPEED:speed;

	//cout<<"speed" <<speed<<endl;

	double computations=floor(((double)((*intervals)[interval_index].end-(*sch)[sch->size()-1].end))*speed);

	cout<<" task id "<<task_id<<" computations "<<computations<<" possible comps "<<possible_comps[task_id]<<endl;
	computations=computations>possible_comps[task_id]?possible_comps[task_id]:computations;

	long_schedule temp_sch;

	temp_sch.arrival=((*intervals)[interval_index].start/(*tasks)[task_id].period)*(*tasks)[task_id].period;
	temp_sch.start=(*sch)[sch->size()-1].end;
	temp_sch.end=temp_sch.start+ceil(computations/speed);
	assert(temp_sch.end<=(*intervals)[interval_index].end);
	temp_sch.float_end=(double)temp_sch.start+computations/speed;
	temp_sch.power=(*tasks)[task_id].power*pow(speed,3);
	temp_sch.speed=speed;
	temp_sch.task_id=task_id;

	cout<<"schedule added: start"<<temp_sch.start<<" end "<<temp_sch.end<<" float end "<<temp_sch.float_end<<" power "<<temp_sch.power<<" speed "<<temp_sch.speed<<endl;

	net_comps_done[task_id]	=	(instance_comps[task_id]+myceil(computations))>=instance_max_comps[task_id]-0.01?
			total_worst_comps[task_id]:net_comps_done[task_id]+myceil(computations);
	instance_comps[task_id]=instance_comps[task_id]+myceil(computations);

	cout<<" net computations done "<<net_comps_done[task_id]<< " instance_comps "<<instance_comps[task_id]<<" max_comps "<<instance_max_comps[task_id]<<endl;

	sch->push_back(temp_sch);
	return((*intervals)[interval_index].end-(*sch)[sch->size()-1].end);

}


void w2fq_interval(vector<long_schedule>*sch, long* computations, double* speed, double *max_computations, vector<long_task>*tasks, long start, long end)
{
	long scaled_comps[tasks->size()];
	double rate[tasks->size()];
	long done_comps[tasks->size()];

	long_schedule temp_sch;

	for(unsigned int i=0;i<tasks->size();i++)
	{
		scaled_comps[i]=round(((double)computations[i])/speed[i]);
		rate[i]=((double)scaled_comps[i])/((double)(end-start));
		done_comps[i]=0;

		cout<<"task "<<i<<" rate "<<rate[i]<<" required computations"<<computations[i]<<" max computations "<<max_computations[i]<<endl;
	}


	long st=start;
	while(st<end)
	{
		//cout<<" start "<<start<<endl;
		int id=-1;
		double etime=end+1;
		for(unsigned int i=0;i<tasks->size();i++)
		{
			if(rate[i]>0.000000001 && done_comps[i]<=round((double((st-start))*rate[i])) 	&&
							 (double)(done_comps[i]+1)/rate[i]<=(etime + W_INT/10)  &&
							 (done_comps[i]+1)<=myceil((max_computations[i])/speed[i]))
			{
				etime=round((double((done_comps[i]+1)))/rate[i]);
				id=i;
			}
		}
		if(id==-1)
		{
			for(unsigned int i=0;i<tasks->size();i++)
			{
				if(rate[i]>0.000000001 && (double)(done_comps[i]+1)/rate[i]<=(etime + W_INT/10)  && (done_comps[i]+1)<=myceil((max_computations[i])/speed[i]))
				{
					etime=round((double((done_comps[i]+1)))/rate[i]);
					id=i;
				}
			}

		}
		if(id>=0)
		{

			temp_sch.start=st;
			temp_sch.end=st+1;
			temp_sch.task_id=id;
			temp_sch.power=(*tasks)[id].power*pow(speed[id],3);
			done_comps[id]=done_comps[id]+1;
			temp_sch.float_end=done_comps[id]>(((double)max_computations[id])/speed[id])?
					((double)temp_sch.end)-(((double)done_comps[id])-((max_computations[id])/speed[id]))
					:temp_sch.end;

			temp_sch.speed=speed[id];
			temp_sch.arrival=(start/(*tasks)[id].period)*(*tasks)[id].period;
			//done_comps[id]=round(done_comps[id] * round(1/W_INT)) * W_INT;
			sch->push_back(temp_sch);
			st=temp_sch.end;
			//start=round(start * round(1/W_INT)) * W_INT;
		}
		else
		{
			st=end;
		}
	}
}


float global_power_interval(long *computations,vector<long_task>*tasks, int earliness)
{
	double total_comps=0.00;
	double scaled_activity=0.00;
	for(unsigned int i=0;i<tasks->size();i++)
	{
		total_comps=total_comps+computations[i];
		scaled_activity=scaled_activity+((double)computations[i])*pow((*tasks)[i].power,1/3.0);
	}

	return(pow((scaled_activity/(total_comps+earliness)),3));
}

int maxfirst_interval(long * computations,long earliness,double *speeds, vector<long_task>*tasks)
{
	#if(INST_MF_PRINT)
		cout<<"\n****************starting new instance maxfirst******************"<<endl;
	#endif

		double g_power; //=global_power(tasks,0,temp_tasks->size(),available_util);
		long local_comps[tasks->size()];

		for(unsigned int i=0;i<tasks->size();i++)
		{
				local_comps[i]=computations[i];
		}
		bool resolved=false;
		while (!resolved)
		{
			resolved = true;
			g_power = global_power_interval(local_comps, tasks, earliness);

		//	cout<<"global power high "<<g_power<<endl;
	#if(ENABLE_PRINTS)
			cout<<"global power:"<<g_power<<"utilization "<<used_util<<endl;
	#endif
			for (unsigned int i = 0; i < tasks->size(); i++)
			{
				float speed = pow(g_power / (*tasks)[i].power,1 / 3.0);
				if (speed > MAX_SPEED && local_comps[i]>0)
				{
					speeds[i]=((double)local_comps[i])/((double)myceil(((double)(local_comps[i]))/MAX_SPEED));
					earliness=earliness-(round(((double)(local_comps[i]))/speeds[i])-local_comps[i]);
					local_comps[i]=0;
					resolved = false;
					break;

				}
			}
		}
		resolved = false;
		while (!resolved)
		{
		//	cout<<"global power low "<<g_power<<endl;
			resolved = true;
			g_power = global_power_interval(local_comps, tasks, earliness);

			for (unsigned int i = 0; i < tasks->size(); i++)
			{
				float speed = pow(g_power / (*tasks)[i].power,1 / 3.0);
				if (speed < MIN_SPEED && local_comps[i]>0)
				{
					speeds[i]=((double)local_comps[i])/((double)myfloor(((double)(local_comps[i]))/MIN_SPEED));
					earliness=earliness-(round(((double)(local_comps[i]))/speeds[i])-local_comps[i]);
					local_comps[i]=0;
					resolved = false;
					break;

				}
			}
		}
		g_power = global_power_interval(local_comps, tasks, earliness);
	//	cout<<"global power regular "<<g_power<<endl;

		for (unsigned int i = 0; i < tasks->size(); i++)
		{
			for (unsigned int i = 0; i < tasks->size(); i++)
			{
				float speed = pow(g_power / (*tasks)[i].power,1 / 3.0);
				if (local_comps[i]>0)
				{
					speeds[i]=((double)local_comps[i])/((double)myfloor(((double)(local_comps[i]))/speed));
					earliness=earliness-(round((double)local_comps[i]/speeds[i])-local_comps[i]);
					local_comps[i]=0;
					resolved = false;
					break;
				}
			}
		}
		return(earliness);
}


void instance_override(vector<float_task>*tasks)
{
	int hyperperiod=compute_lcm(tasks);
	int index=0;

	int total_instances=0;
	for(unsigned int i=0;i<tasks->size();i++)
	{
		total_instances=total_instances+hyperperiod/(*tasks)[i].period;
	}

	cout<<" total instances "<<total_instances<<endl;

	for(unsigned int i=0;i<tasks->size();i++)
	{
		int start=0;
		while(start<hyperperiod)
		{
			global_instances[index][0]=index;
			global_instances[index][1]=i;
			global_instances[index][2]=((double)(*tasks)[i].computation_time)*(0.5+(0.5*rand()/RAND_MAX));
			global_instances[index][3]=start;
			global_instances[index][4]=start+(*tasks)[i].period;
			global_instances[index][5]=(*tasks)[i].power;
			start=global_instances[index][4];
			index=index+1;
		}
	}
	g_instance_size=index;
}


void dynamic_instance_schedule(vector<long_schedule>*sch, vector<long_task>*tasks, vector<interval_s>*intervals,int speed_scaling_enable)
{
	if(tasks->size()>MAX_TASKS)
	{
		cout<<"max task size error. increase parameter"<<endl;
		exit(1);

	}


	for(unsigned int i=0;i<tasks->size();i++)
	{
		cout<<" task "<<i<<" computation time "<<(*tasks)[i].computation_time<<" period "<<(*tasks)[i].period<<" power "<<(*tasks)[i].power<<endl;
	}

	int inst_index[tasks->size()];
	int temp_task=-1;
	for(unsigned int i=0;i<g_instance_size;i++)
	{

		cout<<"task "<<global_instances[i][1]<<" computations "<<global_instances[i][2]<<" start "<<global_instances[i][3]<<" end "<<global_instances[i][4]<<" power "<<global_instances[i][5]<<endl;
		if(global_instances[i][1]!=temp_task)
		{
			temp_task=global_instances[i][1];
			inst_index[temp_task]=i;
		}
	}
	//exit(1);


	long comps_req[tasks->size()];
	long net_comps_done[tasks->size()];
	double speeds[tasks->size()];

	int instance_num[tasks->size()];
	long instance_comps[tasks->size()];
	double instance_max_comps[tasks->size()];
	long total_worst_comps[tasks->size()];

	double max_comps[tasks->size()];

	for(unsigned int i=0;i<tasks->size();i++)
	{
		speeds[i]=1;
		net_comps_done[i]=0;
		instance_num[i]=0;
		instance_comps[i]=0;
		instance_max_comps[i]=0;
		total_worst_comps[i]=0;
	}


	long done_comps[tasks->size()];

	long_schedule tempsch;



	for(unsigned int z=0;z<intervals->size();z++)
	{
		long total=0;
		long earliness=0;
		cout<<" iteration "<<z<<" start "<<(*intervals)[z].start<<" end "<<(*intervals)[z].end<<endl;
		for(unsigned int i=0;i<tasks->size();i++)
		{
			speeds[i]=1;
			comps_req[i]=((*intervals)[z].net_computations[i][0]-net_comps_done[i])>=0?(*intervals)[z].net_computations[i][0]-net_comps_done[i]:0;
			//rate[i]=((double)(intervals[z].computations[i]))/((double)((intervals[z].end-intervals[z].start)));
			total=total+comps_req[i];
			instance_num[i]=(*intervals)[z].start/(*tasks)[i].period;
			instance_max_comps[i]=global_instances[inst_index[i]+instance_num[i]][2]/W_INT;
			if((*intervals)[z].start%(*tasks)[i].period==0)
			{
				instance_comps[i]=0;
//				total_worst_comps[i]=0;
				int y=z;
				while((*intervals)[y].start<(((*intervals)[z].start/(*tasks)[i].period)+1)*(*tasks)[i].period && y<(*intervals).size())
				{
					total_worst_comps[i]=total_worst_comps[i]+(*intervals)[y].computations[i][0];
					y=y+1;
				}
				cout<<"instance worst case computations "<<i<<"|"<<total_worst_comps[i]<<endl;
			}
			cout<<"task "<<i<<" computations required "<<comps_req[i]<<" base computations "<<(*intervals)[z].computations[i]<<endl;

			assert(comps_req[i]<=(*intervals)[z].computations[i][0]);
		}
		earliness=(*intervals)[z].end-(*intervals)[z].start-total;
		cout<<"earliness="<<earliness<<endl;
		assert(earliness>=0);

		cout<<"earliness="<<earliness<<endl;

		if(earliness>0 && speed_scaling_enable==1)
		{
			cout<<"calling interval maxfirst"<<endl;
			earliness=maxfirst_interval(comps_req,earliness,speeds,tasks);
		}

		//calling scheduling scheme to construct schedule


		for(unsigned int i=0;i<tasks->size();i++)
		{
			cout<<" task "<<i<<"speed "<<speeds[i]<<" computations "<<((double)comps_req[i])/speeds[i]<<" power "<<(*tasks)[i].power*pow(speeds[i],3)<<endl;
		}

		for(unsigned int i=0;i<tasks->size();i++)
		{
			max_comps[i]		=	(instance_comps[i]+comps_req[i])>instance_max_comps[i]?instance_max_comps[i]-instance_comps[i]:comps_req[i];
			max_comps[i]=max_comps[i]>0.00?max_comps[i]:0.00;
			net_comps_done[i]	=	(instance_comps[i]+comps_req[i])>=instance_max_comps[i]?total_worst_comps[i]:net_comps_done[i]+comps_req[i];
			if((instance_comps[i]+comps_req[i])>=instance_max_comps[i])
			{
				cout<<"INSTANCE FOR TASK "<<i<< " completed "<< "next arrival "<<((*intervals)[z].start/(*tasks)[i].period+1)*(*tasks)[i].period<<endl;
			}

			instance_comps[i]=instance_comps[i]+comps_req[i];
		}


		cout<<"calling wf2q"<<endl;

		w2fq_interval(sch, comps_req, speeds,  max_comps, tasks,  (*intervals)[z].start, (*intervals)[z].end);
		cout<<"schedule size "<<sch->size()<<endl;
/*		for(unsigned int i=0;i<sch->size();i++)
		{
			if((*sch)[i].start>=intervals[z].start)
			cout<<"start "<<(*sch)[i].start<<" end "<<(*sch)[i].end<<" id "<<(*sch)[i].task_id<<" speed "<<(*sch)[i].speed<<" power "<<(*sch)[i].power<<" float end "<<(*sch)[i].float_end<<endl;
		}
*/


		cout<<"calling end scheduler"<<endl;
		long end_slack=10;
		while(end_slack>=5 && speed_scaling_enable==1)
		{

			end_slack= end_scheduler(sch, intervals,tasks, z,instance_max_comps,instance_comps,net_comps_done,total_worst_comps);
			cout<<"end_slack "<<end_slack<<endl;

			//
		}


		//cin.get();

	}

	double comps_completed[tasks->size()];
	double total_max_comps[tasks->size()];

	for(unsigned int i=0;i<tasks->size();i++)
	{
		comps_completed[i]=0;
		total_max_comps[i]=0;
	}


	for(unsigned int i=0;i<sch->size();i++)
	{
		comps_completed[(*sch)[i].task_id]=		comps_completed[(*sch)[i].task_id]+ ((double)(*sch)[i].float_end-(double)(*sch)[i].start)*(*sch)[i].speed;
	}

	for(unsigned int i=0;i<g_instance_size;i++)
	{
		total_max_comps[(int)global_instances[i][1]]=total_max_comps[(int)global_instances[i][1]]+global_instances[i][2]/W_INT;
	}

	for(unsigned int i=0;i<tasks->size();i++)
	{
		cout<<"task "<<i<< " done computations "<<comps_completed[i]<<" max computations "<<total_max_comps[i]<<endl;
	}
}




