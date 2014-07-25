/*
 * interval_sched.cpp
 *
 *  Created on: Dec 5, 2013
 *      Author: rehan
 */
#include "scheduler.h"

extern float beta;

double average_power(vector<float_task>*tasks)
{
	double power=0;
	for(unsigned int i=0;i<tasks->size();i++)
	{
		power=power+(*tasks)[i].computation_time*(*tasks)[i].power/(*tasks)[i].period;
	}
	return power;
}

bool interval_psort_high(interval_tasks a,interval_tasks b)
{
	return (a.power>b.power);
}

bool interval_psort_low(interval_tasks a,interval_tasks b)
{
	return (a.power<b.power);
}

double high_time(double init_temp,double power,double target)
{
	double time=log((target+MARGIN-(power/beta))	/	(init_temp-(power/beta)))/(-1*beta);
	return time;
}

double low_time(double high_energy, double low_power_delta)
{
	double time=(high_energy)	/	(low_power_delta);
	return time;
}


void interval_sched(vector<float_schedule>*sch,vector<float_task>*tasks)
{

	double avg_power=average_power(tasks);
	double target_temp=avg_power/beta;

	double therm = target_temp;
	vector<profile> temperature;
	profile ttemp;
	ttemp.time=0;
	ttemp.temperature=therm;
	temperature.push_back(ttemp);

	vector<interval_tasks>high;
	vector<interval_tasks>low;
	interval_tasks temp;
	float_schedule temp_sch;
	temp_sch.arrival=0;
	temp_sch.speed=1;
	double error;

	double min_temp=100.00;
	double max_temp=0;

	for(int z=0;z<interval_size;z++)
	{
		double start=interval_req[z][1];
		double total_exec=0;
		cout<<"z "<<z<<" start "<< start<<endl;
		for(unsigned int i=0;i<tasks->size();i++)
		{
			cout<<"task "<<i<<" computations "<<interval_req[z][i+3]<<endl;
			total_exec=total_exec+interval_req[z][i+3];
			if(interval_req[z][i+3]>0.0)
			{
				temp.task_id=i;
				temp.computations=interval_req[z][i+3];
				temp.power=(*tasks)[i].power;
				if(temp.power>avg_power)
				{
					high.push_back(temp);
				}
				else
				{
					low.push_back(temp);
				}
			}
		}
	//	cout<<"total exec"<<total_exec<<endl;
		if(total_exec<(interval_req[z][2]-interval_req[z][1]))
		{
			temp.task_id=-1;
			temp.power=0;
			temp.computations=interval_req[z][2]-interval_req[z][1]-total_exec;
			low.push_back(temp);
		}
		sort(high.begin(),high.end(),interval_psort_high);
		sort(low.begin(),low.end(),interval_psort_low);
/*
		cout<<" printing high list"<<endl;
		for(unsigned int i=0;i<high.size();i++)
		{
			cout<<"task "<<high[i].task_id<<" computations "<<high[i].computations<<" power "<<high[i].power<<endl;
		}

		cout<<" printing low list"<<endl;
		for(unsigned int i=0;i<low.size();i++)
		{
			cout<<"task "<<low[i].task_id<<" computations "<<low[i].computations<<" power "<<low[i].power<<endl;
		}
	*/
		while(!(high.empty() && low.empty()))
		{
			if(!(high.empty() || low.empty()))
			{
				double htime=high_time(ttemp.temperature,high[0].power,target_temp);
				temp_sch.start=start;
				temp_sch.power=high[0].power;
				temp_sch.task_id=high[0].task_id;
				if(htime>=high[0].computations)
				{
					htime=high[0].computations;
					high[0].computations=0;
					high.erase(high.begin());
				}
				else
				{
					high[0].computations=high[0].computations-htime;
				}
				temp_sch.end=temp_sch.start+htime;
				start=temp_sch.end;

				ttemp.time=temp_sch.end;
				ttemp.temperature=heat(ttemp.temperature, temp_sch.power, temp_sch.end-temp_sch.start);
				temperature.push_back(ttemp);

				sch->push_back(temp_sch);

				double high_energy=(temp_sch.power-avg_power)*htime;
				//cout<<"average power "<<avg_power<<endl;
				//cout<<"HIGH start:"<<temp_sch.start<< " end:"<<temp_sch.end<<" id "<<temp_sch.task_id<<" High energy"<<high_energy<<endl;

				double ltime=low_time(	high_energy	,	avg_power-low[0].power);

				while(ltime>low[0].computations && !low.empty())
				{
					temp_sch.start=start;
					temp_sch.power=low[0].power;
					temp_sch.end=temp_sch.start+low[0].computations;
					temp_sch.task_id=low[0].task_id;
					start=temp_sch.end;
					ttemp.time=temp_sch.end;

					start=start>interval_req[z][2]?interval_req[z][2]:start;
					error=temp_sch.end-start;

					temp_sch.end=start;
					ttemp.time=temp_sch.end;
					ttemp.temperature=heat(ttemp.temperature, temp_sch.power, temp_sch.end-temp_sch.start);

					temperature.push_back(ttemp);
				//	cout<<"LOWRECUR start:"<<temp_sch.start<< " end:"<<temp_sch.end<<" Error "<<error<<" id "<<temp_sch.task_id<<endl;
					sch->push_back(temp_sch);
					high_energy=high_energy-(avg_power-temp_sch.power)*(temp_sch.end-temp_sch.start);
					assert(high_energy>0);
					low.erase(low.begin());
					ltime=low_time(	high_energy,	avg_power-low[0].power);
				}
				if(!low.empty())
				{
					temp_sch.start=start;
					temp_sch.power=low[0].power;
					temp_sch.end=temp_sch.start+ltime;
					temp_sch.task_id=low[0].task_id;
					start=temp_sch.end;

					ttemp.time=temp_sch.end;

					start=start>interval_req[z][2]?interval_req[z][2]:start;
					error=temp_sch.end-start;

					temp_sch.end=start;
					ttemp.time=temp_sch.end;
					ttemp.temperature=heat(ttemp.temperature, temp_sch.power, temp_sch.end-temp_sch.start);

					temperature.push_back(ttemp);
				//	cout<<"LOW start:"<<temp_sch.start<< " end:"<<temp_sch.end<<" Error "<<error<<" id "<<temp_sch.task_id<<endl;
					sch->push_back(temp_sch);
					low[0].computations=low[0].computations-ltime;

					if(low[0].computations<0.0000001)
					{
						low.erase(low.begin());
					}
				}
			}
			else
			{
				if(high.empty())
				{
					while(!low.empty())
					{
						temp_sch.start=start;
						temp_sch.power=low[0].power;
						temp_sch.end=temp_sch.start+low[0].computations;
						temp_sch.task_id=low[0].task_id;
						start=temp_sch.end;

						start=start>interval_req[z][2]?interval_req[z][2]:start;
						error=temp_sch.end-start;

						temp_sch.end=start;
						ttemp.time=temp_sch.end;
						ttemp.temperature=heat(ttemp.temperature, temp_sch.power, temp_sch.end-temp_sch.start);

						temperature.push_back(ttemp);
					//	cout<<"LOWREPEAT start:"<<temp_sch.start<< " end:"<<temp_sch.end<<" Error "<<error<<" id "<<temp_sch.task_id<<endl;
						sch->push_back(temp_sch);
						low.erase(low.begin());
					}
				}
				else if(low.empty())
				{
					while(!high.empty())
					{
						temp_sch.start=start;
						temp_sch.power=high[0].power;
						temp_sch.end=temp_sch.start+high[0].computations;
						temp_sch.task_id=high[0].task_id;
						start=temp_sch.end;

						start=start>interval_req[z][2]?interval_req[z][2]:start;
						error=temp_sch.end-start;

						temp_sch.end=start;
						ttemp.time=temp_sch.end;
						ttemp.temperature=heat(ttemp.temperature, temp_sch.power, temp_sch.end-temp_sch.start);

						temperature.push_back(ttemp);
					//	cout<<"HIGHREPEAT start:"<<temp_sch.start<< " end:"<<temp_sch.end<<" Error "<<error<<" id "<<temp_sch.task_id<<endl;
						sch->push_back(temp_sch);

						high.erase(high.begin());
					}
				}
			}
		}


		for(unsigned int i=0;i<temperature.size();i++)
		{
			if(temperature[i].temperature<min_temp)
			{
				min_temp=temperature[i].temperature;
			}
			if(temperature[i].temperature>max_temp)
			{
				max_temp=temperature[i].temperature;
			}
		//	cout<<temperature[i].time<<"\t"<<temperature[i].temperature<<endl;
		}


		high.clear();
		low.clear();

	}

	for (unsigned int i=0;i<temperature.size();i++)
	{
		cout<<"time "<<temperature[i].time<<" temperature "<<temperature[i].temperature<<endl;
	}
	cout<<"Margin:"<<MARGIN<<"\t"<<"intervals "<<temperature.size()<<" min temp "<<min_temp<<"\t"<<"max_temp "<<max_temp<<endl;
	cout<<"Target Temperature "<<target_temp<<endl;
}


