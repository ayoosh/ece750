/*
 * w2fq.cpp
 *
 *  Created on: Dec 11, 2013
 *      Author: rehan
 */
#include "scheduler.h"


bool interval_ascend(interval_s a,interval_s b)
{
	return(a.start<b.start);
}

void w2fq_task_convert(vector<long_task>*tasks, vector<float_task>*f_tasks)
{
	long_task temp;

	for(unsigned int i=0;i<f_tasks->size();i++)
	{
		temp.computation_time=round((*f_tasks)[i].computation_time/W_INT);
		temp.computations=round((*f_tasks)[i].computations/W_INT);
		temp.index=(*f_tasks)[i].index;
		temp.next_start=round((*f_tasks)[i].next_start/W_INT);
		temp.period=round((*f_tasks)[i].period/W_INT);
		temp.power=(*f_tasks)[i].power;
		tasks->push_back(temp);
	}
}


void generate_intervals(vector<interval_s>* intervals, vector<long_task>*tasks)
{
	long total;
	interval_s temp_int;
	for(unsigned int i=0;i<interval_size;i++)
	{
		total=0;
		temp_int.start=round(interval_req[i][1]/W_INT);
		temp_int.end=round(interval_req[i][2]/W_INT);

		for(unsigned int j=0;j<tasks->size();j++)
		{
			temp_int.computations[j][0]=round(interval_req[i][3+j]/W_INT);
			total=total+temp_int.computations[j][0];
			if(i>0)
			{
				temp_int.net_computations[j][0]=(*intervals)[intervals->size()-1].net_computations[j][0]+temp_int.computations[j][0];

			}
			else
			{
				temp_int.net_computations[j][0]=temp_int.computations[j][0];
			}
		}


		if(total>(temp_int.end-temp_int.start))
		{
			int correction=total-(temp_int.end-temp_int.start);
			cout<<"correction "<<correction<<" correct_total"<<total<<" length "<<temp_int.end-temp_int.start<<endl;
			for(unsigned int j=0;j<tasks->size();j++)
			{
				if(temp_int.computations[j][0]>correction)
				{
					cout<<"correcting"<<endl;
					temp_int.computations[j][0]=temp_int.computations[j][0]-correction;
					temp_int.net_computations[j][0]=temp_int.net_computations[j][0]-correction;
					break;
				}

			}
		}
	}
}


void generate_intervals_gps(vector<interval_s>* intervals, vector<long_task>*tasks)
{


	long hyperperiod=compute_lcm(tasks);
	interval_s tint;

	long start=0;

	for(unsigned int i=0;i<tasks->size();i++)
	{
		start=0;
		while(start<hyperperiod)
		{
			tint.start=start;
			tint.end=0;
			intervals->push_back(tint);
			start=start+(*tasks)[i].period;
		}
	}

	sort(intervals->begin(), intervals->end(),interval_ascend);

	for(unsigned int i=0;i<intervals->size();i++)
	{
		if(i>0)
		{
			if((*intervals)[i].start==(*intervals)[i-1].start)
			{
				intervals->erase(intervals->begin()+i);
				i=i-1;
			}
		}
	}

	for(unsigned int i=0;i<intervals->size();i++)
	{
		if(i<intervals->size()-1)
		{
			(*intervals)[i].end=(*intervals)[i+1].start;
		}
		else
		{
			(*intervals)[i].end=hyperperiod;
		}
	}

	for(unsigned int i=0;i<intervals->size();i++)
	{
		for(unsigned int j=0;j<tasks->size();j++)
		{
			(*intervals)[i].computations[j][0]= round(((double)((*intervals)[i].end-(*intervals)[i].start)) * ((double)(*tasks)[j].computation_time)/((double)(*tasks)[j].period));
			if(i>0)
			{
				(*intervals)[i].net_computations[j][0]=(*intervals)[i-1].net_computations[j][0]+(*intervals)[i].computations[j][0];

			}
			else
			{
				(*intervals)[i].net_computations[j][0]=(*intervals)[i].computations[j][0];
			}
		}
	}
}

/*
void w2fq_schedule(vector<long_schedule>*sch, vector<long_task>*tasks, vector<interval>*intervals)
{
	long_schedule temp_sch;
	long start;
	long end;
	long total_comps[tasks->size()];
	long done_comps[tasks->size()];
	double rate[tasks->size()];

	long hyperperiod=compute_lcm(tasks);

	vector<interval> intv;
	interval tint;

	for(unsigned int i=0;i<tasks->size();i++)
	{
		start=0;
		while(start<hyperperiod)
		{
			tint.start=start;
			tint.end=0;
			intv.push_back(tint);
			start=start+(*tasks)[i].period;
		}
	}

	sort(intv.begin(), intv.end(),interval_ascend);

	for(unsigned int i=0;i<intv.size();i++)
	{
		if(i>0)
		{
			if(intv[i].start==intv[i-1].start)
			{
				intv.erase(intv.begin()+i);
				i=i-1;
				//cout<<"erasing"<<endl;
			}
		}
	}

	for(unsigned int i=0;i<intv.size();i++)
	{
		if(i<intv.size()-1)
		{
			intv[i].end=intv[i+1].start;
		}
		else
		{
			intv[i].end=hyperperiod;
		}
	}

	long done_comps_test[tasks->size()];

	temp_sch.arrival=0;
	temp_sch.speed=1;

	for(int z=0;z<intv.size();z++)
	{

		start=intv[z].start;
		end=intv[z].end;


		for(unsigned int i=0;i<tasks->size();i++)
		{
			rate[i]=((double)((*tasks)[i].computation_time))/((double)(*tasks)[i].period);

			total_comps[i]=round((double(end-start))*	rate[i]);


//			cout<<"rate "<<i<<" computations "<<rate[i]<<endl;
			done_comps[i]=0;
			done_comps_test[i]=0;

		}

		bool slack_end=true;
		while(start<end)
		{
			//cout<<" start "<<start<<endl;
			int id=-1;
			double etime=end+1;
			for(unsigned int i=0;i<tasks->size();i++)
			{
				if(done_comps[i]<=round((double((start-intv[z].start))*rate[i])) 	&&
								 (double)(done_comps[i]+1)/rate[i]<=(etime + W_INT/10)  &&
								 (done_comps[i]+1)<=total_comps[i])
				{
					etime=round((double((done_comps[i]+1)))/rate[i]);
					id=i;
				}
				if((total_comps[i]-done_comps[i])>1)
				{
					slack_end=false;
				}
			}
			if(id==-2 && !slack_end)
			{
				for(unsigned int i=0;i<tasks->size();i++)
				{
					if((double)(done_comps[i]+1)/rate[i]<=(etime + W_INT/10)  && (done_comps[i]+1)<=total_comps[i])
					{
						etime=round((double((done_comps[i]+1)))/rate[i]);
						id=i;
					}
				}

			}
			if(id>=0)
			{

				temp_sch.start=start;
				temp_sch.end=start+1;
				temp_sch.float_end=temp_sch.end;
				//temp_sch.end=round(temp_sch.end * round(1/W_INT)) * W_INT;


				temp_sch.task_id=id;
				temp_sch.power=(*tasks)[id].power;
				done_comps[id]=done_comps[id]+1;
				//done_comps[id]=round(done_comps[id] * round(1/W_INT)) * W_INT;
				sch->push_back(temp_sch);
				start=temp_sch.end;
				//start=round(start * round(1/W_INT)) * W_INT;
			}
			else
			{
				start=start+1;
			}

		}

		for(unsigned int i=0;i<tasks->size();i++)
		{
			if (done_comps[i]!=total_comps[i])
			{
				cout<<"less computations for task "<<i<<" in interval "<<z<<" start "<<intv[z].start<<" end "<<intv[z].end<<endl;

				for(unsigned int y=0;y<sch->size();y++)
				{
					if((*sch)[y].start>=round(interval_req[z][1]/W_INT) && (*sch)[y].end<=round(interval_req[z][2]/W_INT))
					{
						done_comps_test[(*sch)[y].task_id]=done_comps_test[(*sch)[y].task_id]+(*sch)[y].end-(*sch)[y].start;
						//cout<<"id "<<(*sch)[y].task_id<<" start "<<(*sch)[y].start<<" end "<<(*sch)[y].end<<endl;
					}
				}


				for(unsigned int j=0;j<tasks->size();j++)
				{
					cout<<"task "<<j<<" total computations "<<done_comps[j]<<" required computations "<<total_comps[j]<<" test computations "<<done_comps_test[j]<<" rate "<<rate[j]<<endl;
				}
				cout<<"start "<<round(intv[z].start)<<" end "<<end<<endl;
				exit(1);
			}
		}
	}

	double total_done_comps[tasks->size()];

	for(unsigned int i=0;i<tasks->size();i++)
	{
		total_done_comps[i]=0;
	}


	for(unsigned int i=0;i<sch->size();i++)
	{
		total_done_comps[(*sch)[i].task_id]=total_done_comps[(*sch)[i].task_id]+(*sch)[i].end-(*sch)[i].start;
//		cout<<"id "<<(*sch)[i].task_id<<" start "<<(*sch)[i].start<<" end "<<(*sch)[i].end<<endl;
	}

	for(unsigned int i=0;i<tasks->size();i++)
	{
		cout<<"task "<<i<<" total computations "<<total_done_comps[i]<<" required computations "<<round(intv[intv.size()-1].end*rate[i])<<endl;
//		cout<<"task "<<i<<" total computations "<<total_done_comps[i]<<" required computations "<<total_comps[i]<<endl;
	}
}
*/

void w2fq_schedule(vector<long_schedule>*sch, vector<long_task>*tasks, vector<interval_s>*intervals, int core)
{
	long_schedule temp_sch;
	long start;
	long end;
	long done_comps[tasks->size()];
	double rate[tasks->size()];

	long hyperperiod=compute_lcm(tasks);



	long done_comps_test[tasks->size()];

	temp_sch.arrival=0;
	temp_sch.speed=1;

	for(int z=0;z<intervals->size();z++)
	{

		start=(*intervals)[z].start;
		end=(*intervals)[z].end;


		for(unsigned int i=0;i<tasks->size();i++)
		{
			rate[i]=((double)((*intervals)[z].computations[i][core]))/(((double)((*intervals)[z].end-(*intervals)[z].start)));


//			cout<<"rate "<<i<<" computations "<<rate[i]<<endl;
			done_comps[i]=0;
			done_comps_test[i]=0;

		}

		bool slack_end=true;
		while(start<end)
		{
			//cout<<" start "<<start<<endl;
			int id=-1;
			double etime=end+1;
			for(unsigned int i=0;i<tasks->size();i++)
			{
				if(done_comps[i]<=round((double((start-(*intervals)[z].start))*rate[i])) 	&&
								 (double)(done_comps[i]+1)/rate[i]<=(etime + W_INT/10)  &&
								 (done_comps[i]+1)<=(*intervals)[z].computations[i][core])
				{
					etime=round((double((done_comps[i]+1)))/rate[i]);
					id=i;
				}
			}
			if(id>=0)
			{
				temp_sch.start=start;
				temp_sch.end=start+1;
				temp_sch.float_end=temp_sch.end;
				temp_sch.task_id=id;
				temp_sch.power=(*tasks)[id].power;
				temp_sch.core=core;
				done_comps[id]=done_comps[id]+1;
				sch->push_back(temp_sch);
				start=temp_sch.end;
			}
			else
			{
				start=start+1;
			}

		}

		for(unsigned int i=0;i<tasks->size();i++)
		{
			if (done_comps[i]!=(*intervals)[z].computations[i][core])
			{
				cout<<"less computations for task "<<i<<" in interval "<<z<<" start "<<(*intervals)[z].start<<" end "<<(*intervals)[z].end<<endl;

				for(unsigned int y=0;y<sch->size();y++)
				{
					if((*sch)[y].start>=round(interval_req[z][1]/W_INT) && (*sch)[y].end<=round(interval_req[z][2]/W_INT))
					{
						done_comps_test[(*sch)[y].task_id]=done_comps_test[(*sch)[y].task_id]+(*sch)[y].end-(*sch)[y].start;
						//cout<<"id "<<(*sch)[y].task_id<<" start "<<(*sch)[y].start<<" end "<<(*sch)[y].end<<endl;
					}
				}


				for(unsigned int j=0;j<tasks->size();j++)
				{
					cout<<"task "<<j<<" total computations "<<done_comps[j]<<" required computations "<<(*intervals)[z].computations[j]<<" test computations "<<done_comps_test[j]<<" rate "<<rate[j]<<endl;
				}
	//			cout<<"start "<<(*intervals)[z].start<<" end "<<end<<endl;
				exit(1);
			}
		}
	}

	double total_done_comps[tasks->size()];

	for(unsigned int i=0;i<tasks->size();i++)
	{
		total_done_comps[i]=0;
	}


	for(unsigned int i=0;i<sch->size();i++)
	{
		total_done_comps[(*sch)[i].task_id]=total_done_comps[(*sch)[i].task_id]+(*sch)[i].end-(*sch)[i].start;
//		cout<<"id "<<(*sch)[i].task_id<<" start "<<(*sch)[i].start<<" end "<<(*sch)[i].end<<endl;
	}

	//for(unsigned int i=0;i<tasks->size();i++)
	//{
	//	cout<<"task "<<i<<" total computations "<<total_done_comps[i]<<" required computations "<<(*intervals)[intervals->size()-1].net_computations[i]<<endl;
//	/	cout<<"task "<<i<<" total computations "<<total_done_comps[i]<<" required computations "<<total_comps[i]<<endl;
	//}
}


void w2fq_schedule_convert(vector<float_schedule>*fsch, vector<long_schedule>*lsch)
{
	float_schedule temp;

	for(unsigned int i=0;i<lsch->size();i++)
	{
		temp.arrival=(double)((*lsch)[i].arrival)*W_INT;
		temp.end=(double)((*lsch)[i].float_end)*W_INT;
		temp.power=(*lsch)[i].power;
		temp.speed=(*lsch)[i].speed;
		temp.start=(double)((*lsch)[i].start)*W_INT;
		temp.task_id=(*lsch)[i].task_id;
		fsch->push_back(temp);
	}
}




