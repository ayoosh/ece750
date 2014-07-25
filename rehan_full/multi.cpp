/*
 * multi.cpp
 *
 *  Created on: Jan 2, 2014
 *      Author: rehan
 */

#include "scheduler.h"

extern double beta;
extern double corrected_threshold;

void generate_taskset_multi(vector<float_task> *tasks, long hyperperiod, int num_tasks, float comp_util)
{
	generate_taskset(tasks, hyperperiod, num_tasks, comp_util);
	float recur=false;
	for(unsigned int i=0;i<tasks->size();i++)
	{
		if((*tasks)[i].computation_time/(*tasks)[i].period>=1 ||
		   (*tasks)[i].computation_time*(*tasks)[i].power/(beta*(*tasks)[i].period*corrected_threshold)>=1.0)
		{
			recur=true;
		}
	}
	if(recur)
	{
		tasks->clear();
		generate_taskset_multi(tasks, hyperperiod, num_tasks, comp_util);
	}

}

void read_taskset_multi(vector<float_task> *tasks, string file)
{
	ifstream f(file.c_str());
	if(!f.is_open())
	{
		cout<<"file not opened "<<file<<endl;
		exit(1);
	}
	float_task temp;
	string line;

	vector<string>delim,elem;
	delim.push_back("\t");
	delim.push_back(" ");

	temp.computations=0;
	temp.next_start=0;

	while(getline(f,line))
	{
		elem.clear();
		split(&line,&delim,&elem);
		if(elem.size()!=4)
		{
			cout<<" incorrect size in read_taskset_multi"<<elem.size()<<endl;
			for(unsigned int i=0;i<elem.size();i++)
			{
				cout<<elem[i]<<endl;
			}
			exit(1);
		}

		temp.computation_time=atof(elem[1].c_str());
		temp.period=atof(elem[2].c_str());
		temp.index=tasks->size();
		temp.power=atof(elem[3].c_str());

		tasks->push_back(temp);
	}
}


int gams_include(vector<float_task>*tasks, int** matrix)
{
	stringstream task_str,period_str,computation_str,power_str, intervals_str, intstart_str, intend_str,
	inst_str, instdeadline_str, instarrival_str, instpower_str, instcomputation_str, resource_str, final, speed_str;
	task_str<<"set T/";
	period_str<<"parameter period(T)/";
	computation_str<<"parameter computation(T)/";
	power_str<<"parameter power(T)/";
	intervals_str<<"set I /";
	intstart_str<<"parameter start(I) /";
	intend_str<<"parameter end(I) /";
	inst_str<<"set Inst /";
	instarrival_str<<"parameter arrival(Inst) /";
	instdeadline_str<<"parameter deadline(Inst)/";
	instpower_str<<"parameter inst_power(Inst)/";
	instcomputation_str<<"parameter inst_comp(Inst)/";
	resource_str<<"table resource_cons(T,T)"<<endl;
	speed_str<<"scalar smin minimum speed /0.5/;\nscalar smax maximum speed/1.0/\ntable speed_dep()"<<endl;



	int position[tasks->size()];

	int initial_length=resource_str.str().length();

	for(unsigned int i=0;i<tasks->size();i++)
	{

		if(i<tasks->size()-1)
		{
			task_str<<"task"<<i<<",";
			period_str<<"task"<<i<<" "<<(*tasks)[i].period<<",";
			computation_str<<"task"<<i<<" "<<(*tasks)[i].computation_time<<",";
			power_str<<"task"<<i<<" "<<(*tasks)[i].power<<",";
			resource_str<<"          task"<<i;
			position[i]=resource_str.str().length()-2-initial_length;

		}
		else
		{
			task_str<<"task"<<i<<"/;";
			period_str<<"task"<<i<<" "<<(*tasks)[i].period<<"/;";
			computation_str<<"task"<<i<<" "<<(*tasks)[i].computation_time<<"/;";
			power_str<<"task"<<i<<" "<<(*tasks)[i].power<<"/;";
			resource_str<<"          task"<<i<<endl;
			position[i]=resource_str.str().length()-3-initial_length;
		}
	}

	vector<interval_s>intervals;
	long hyperperiod=compute_lcm(tasks);
	interval_s tint;

	long start=0;

	for(unsigned int i=0;i<tasks->size();i++)
	{
		start=0;
		int j=0;
		while(start<hyperperiod)
		{

			tint.start=start;
			tint.end=0;

			intervals.push_back(tint);
			start=start+(*tasks)[i].period;
			if(start<hyperperiod || i<(tasks->size()-1))
			{
				inst_str<<"job"<<i<<"_"<<j<<",";
				instarrival_str<<"job"<<i<<"_"<<j<<" "<<start-(*tasks)[i].period<<",";
				instdeadline_str<<"job"<<i<<"_"<<j<<" "<<start<<",";
				instpower_str<<"job"<<i<<"_"<<j<<" "<<(*tasks)[i].power<<",";
				instcomputation_str<<"job"<<i<<"_"<<j<<" "<<(*tasks)[i].computation_time<<",";
			}
			else
			{
				inst_str<<"job"<<i<<"_"<<j<<"/;";
				instarrival_str<<"job"<<i<<"_"<<j<<" "<<start-(*tasks)[i].period<<"/;";
				instdeadline_str<<"job"<<i<<"_"<<j<<" "<<start<<"/;";
				instpower_str<<"job"<<i<<"_"<<j<<" "<<(*tasks)[i].power<<"/;";
				instcomputation_str<<"job"<<i<<"_"<<j<<" "<<(*tasks)[i].computation_time<<"/;";
			}
			j=j+1;
		}
	}

	sort(intervals.begin(), intervals.end(),interval_ascend);

	for(unsigned int i=0;i<intervals.size();i++)
	{
		if(i>0)
		{
			if(intervals[i].start==intervals[i-1].start)
			{
				intervals.erase(intervals.begin()+i);
				i=i-1;
			}
		}
	}

	for(unsigned int i=0;i<intervals.size();i++)
	{
		if(i<intervals.size()-1)
		{
			intervals_str<<"I"<<i<<",";
			intstart_str<<"I"<<i<<" "<<intervals[i].start<<",";
			intervals[i].end=intervals[i+1].start;
			intend_str<<"I"<<i<<" "<<intervals[i].end<<",";

		}
		else
		{
			intervals[i].end=hyperperiod;
			intervals_str<<"I"<<i<<"/;";
			intstart_str<<"I"<<i<<" "<<intervals[i].start<<"/;";
			intend_str<<"I"<<i<<" "<<intervals[i].end<<"/;";
		}
	}

	//cout<<"generating resource matrix"<<endl;
	for(unsigned int i=0;i<tasks->size();i++)
	{
		int start=resource_str.str().length();
		resource_str<<"task"<<i;
		for(unsigned int j=0;j<tasks->size();j++)
		{

				while((resource_str.str().length()-start)<position[j])
				{
					resource_str<<" ";
				}
				resource_str<<matrix[i][j];
		}

		if(i<(tasks->size()-1))
		{
			resource_str<<endl;
		}
		else
		{
			resource_str<<";";
		}
	}

//	cout<<"generated resource matrix"<<endl;

	final<<task_str.str()<<"\n\n";
	final<<period_str.str()<<"\n\n";
	final<<computation_str.str()<<"\n\n";
	final<<power_str.str()<<"\n\n";

	final<<intervals_str.str()<<"\n\n";
	final<<intstart_str.str()<<"\n\n";
	final<<intend_str.str()<<"\n\n";


	final<<inst_str.str()<<"\n\n";
	final<<instarrival_str.str()<<"\n\n";
	final<<instdeadline_str.str()<<"\n\n";
	final<<instpower_str.str()<<"\n\n";
	final<<instcomputation_str.str()<<"\n\n";
	final<<resource_str.str()<<"\n\n";

	final<<"scalar hyperperiod /"<<intervals[intervals.size()-1].end<<"/;"<<"\n\n";

	string temp=final.str();





	write_to_file("gams_files/gams_inc", &temp);

	task_str.str(std::string());
	period_str.str(std::string());
	computation_str.str(std::string());
	power_str.str(std::string());
	intervals_str.str(std::string());
	intstart_str.str(std::string());
	intend_str.str(std::string());
	inst_str.str(std::string());
	instdeadline_str.str(std::string());
	instarrival_str.str(std::string());
	instpower_str.str(std::string());
	instcomputation_str.str(std::string());
	resource_str.str(std::string());
	final.str(std::string());
	temp="";
	return intervals.size();

}


void generate_intervals_multi(vector<interval_s>*intervals,string fname, vector<float_task>*tasks)
{
	ifstream f(fname.c_str());
	string line;
	vector<string>delim,elem;
	delim.push_back("\t");
	delim.push_back(" ");

	interval_s temp_int;

	int core=0;
	int start;
	while(getline(f,line))
	{
		elem.clear();
		split(&line,&delim,&elem);
		if(core==0)
		{
			temp_int.start=round(atof(elem[1].c_str())/W_INT);
			temp_int.end=round(atof(elem[2].c_str())/W_INT);
			start=4;
		}
		else
		{
			start=1;
		}

		long total=0;

		for(unsigned int i=start;i<elem.size();i++)
		{
			if((elem.size()-start)!=tasks->size())
			{
				cout<<"error detected element size " <<elem.size()-start<<" task size "<<tasks->size()<<" core "<<core<<endl;

				for(unsigned int z=0;z<elem.size();z++)
				{
					cout<<z<<": "<<elem[z]<<endl;
				}

				exit(1);
			}
			temp_int.computations[i-start][core]=round(atof(elem[i].c_str())/W_INT);
			total=total+temp_int.computations[i-start][core];
			if(intervals->size()>0)
			{
				temp_int.net_computations[i-start][core]=(*intervals)[intervals->size()-1].net_computations[i-start][core]+temp_int.computations[i-start][core];

			}
			else
			{
				temp_int.net_computations[i-start][core]=temp_int.computations[i-start][core];
			}

		}

		if(total>(temp_int.end-temp_int.start))
		{
			int correction=total-(temp_int.end-temp_int.start);
			cout<<"correction "<<correction<<" correct_total"<<total<<" length "<<temp_int.end-temp_int.start<<endl;
			for(unsigned int j=0;j<tasks->size();j++)
			{
				if(temp_int.computations[j][core]>correction)
				{
					cout<<"correcting"<<endl;
					temp_int.computations[j][core]=temp_int.computations[j][core]-correction;
					temp_int.net_computations[j][core]=temp_int.net_computations[j][core]-correction;
					break;
				}

			}
		}

		core=core+1;
		if(core>=CORE)
		{

			core=0;
			intervals->push_back(temp_int);
			cout<<" interval start"<<temp_int.start<<" end "<<temp_int.end<<" ";
			for(unsigned int j=0;j<CORE;j++)
			{
				for(unsigned int k=0;k<tasks->size();k++)
				{
					cout<<temp_int.computations[k][j]<<" ";
				}
			}
			cout<<endl;
			if(getline(f,line))
			{
				if(line.size()>1)
				{
					cout<<"unexpected line size "<<line.size()<<" line: "<<line<<endl;
					exit(1);
				}
			}
			else
			{
				break;
			}
		}
	}
}


void generate_intervals_multi(vector<interval>*intervals,string fname)
{
	ifstream f(fname.c_str());
	string line;
	vector<string>delim,elem;
	delim.push_back("\t");
	delim.push_back(" ");

	interval temp_int;
	execution temp_exec;

	long total[CORE];

	while(getline(f,line))
	{
		elem.clear();
		split(&line,&delim,&elem);
		if(elem.size()>1)
		{
			if(elem[0].compare(0,4,"NEW_")==0)
			{
				if(temp_int.get_size()>0)
				{
			//		cout<<temp_int.start<<"|"<<temp_int.end<<"|"<<total[0]<<"|"<<total[1]<<"|"<<total[2]<<endl;
					for(unsigned int i=0;i<CORE;i++)
					{
						if(total[i]>(temp_int.end-temp_int.start))
						{
							long correction=total[i]-(temp_int.end-temp_int.start);
							for(unsigned int j=0;j<temp_int.get_size();j++)
							{
								if(temp_int.exec[j].exec>correction && temp_int.exec[j].core==i)
								{
									temp_int.exec[j].exec=temp_int.exec[j].exec-correction;
									break;
								}

							}
						}
					}
					intervals->push_back(temp_int);
				}

				for(unsigned int i=0;i<CORE;i++)
				{
					total[i]=0;
				}

				temp_int.clear();
				temp_int.set_interval(round(atof(elem[1].c_str())/W_INT),round(atof(elem[2].c_str())/W_INT));

			}
			else
			{
				if(elem.size()!=4)
				{
					cout<<"error in element size "<<elem.size()<<endl;
					for(unsigned int i=0;i<elem.size();i++)
					{
						cout<<i<<" "<<elem[i]<<endl;
					}
					exit(1);
				}

				temp_exec.core=atoi(elem[0].substr(4,elem[0].size()-4).c_str())-1;
				temp_exec.task=atoi(elem[1].substr(4,elem[1].size()-4).c_str());
				temp_exec.speed_index=atoi(elem[2].substr(1,elem[2].size()-1).c_str())-1;
				temp_exec.exec=round(atof(elem[3].c_str())/W_INT);
				temp_exec.speed=speed_levels[temp_exec.speed_index];
				temp_exec.unit_exec=((double)temp_exec.exec)*temp_exec.speed;

				total[temp_exec.core]=total[temp_exec.core]+temp_exec.exec;

				temp_int.add_exec(temp_exec);

			}
		}
	}
	intervals->push_back(temp_int);
}



void multi_schedule(vector<long_schedule>*sch,vector<interval_s>*intervals, vector<long_task>*tasks)
{
	for(int i=0;i<CORE;i++)
	{
		w2fq_schedule(sch, tasks, intervals, i);
	}
	sort(sch->begin(),sch->end(),sort_sch);
}

/*void edf_schedule_multi(vector<long_schedule>*sch, vector<long_task>*tasks)
{
	vector<long> times;
	imp_times(tasks, &times);
#if(ENABLE_PRINTS)

	for(unsigned int i=0;i<times.size();i++)
	{
		cout<<"times: "<<i<<":"<<times[i]<<endl;
	}
#endif

	for (unsigned int i = 0; i < tasks->size(); i++) {
		(*tasks)[i].computations = 0;
		(*tasks)[i].next_start = 0;
	}

	long_schedule temp;
	for (unsigned int i = 0; i < times.size() - 1; i++)
	{
		int start = times[i];
		while (start < times[i + 1])
		{
			int id = min_deadline(tasks, start);
			if (id == -1)
			{
				break;
			}

			int computations_left = (*tasks)[id].computation_time - (*tasks)[id].computations;
			(*tasks)[id].computations = (times[i + 1] - start) >= computations_left ?(*tasks)[id].computation_time :
										(*tasks)[id].computations + times[i + 1] - start;
			temp.start = start;
			temp.end =(times[i + 1] - start) >= computations_left ?temp.start + computations_left : times[i + 1];
			temp.task_id = id;
			temp.arrival=(start/(*tasks)[temp.task_id].period)*(*tasks)[temp.task_id].period;

			if ((*tasks)[id].computations == (*tasks)[id].computation_time&& start >= (*tasks)[id].next_start)
			{
				(*tasks)[id].computations = 0;
				(*tasks)[id].next_start = start / (*tasks)[id].period * (*tasks)[id].period + (*tasks)[id].period;
			}

			start = temp.end;
			edf->push_back(temp);
		}
	}
#if(ENABLE_PRINTS)

	for(unsigned int i=0;i<edf->size();i++)
	{
		cout<<i<<": Task:"<<(*edf)[i].task_id<<" start:"<<(*edf)[i].start<<" end: "<<(*edf)[i].end<<endl;
	}
#endif

	verify(edf, tasks);
}*/


void generate_power_trace(vector<long_schedule>*sch, string fname, long hyperperiod, float * average_power)
{
	ofstream profile;

	stringstream tname;
	tname<<"hotspot_files/"<<fname<<".ptrace";

	profile.open(tname.str().c_str());

	tname.str("");
	tname<<"hotspot_files/"<<fname<<"_steady";

	ofstream power_steady;
	power_steady.open(tname.str().c_str());


	for(int i=0;i<CORE;i++)
	{
		profile<<"core"<<i<<"\t";
		power_steady<<"core"<<i<<"\t";
	}
	profile<<"\n";
	power_steady<<"\n";


	for(unsigned int i=0;i<CORE;i++)
	{
		average_power[i]=0.00;
	}

	unsigned int index=0;
	int lines=0;
	while(index<sch->size())
	{
		if(index>0)
		{
			if((*sch)[index].start>(*sch)[index-1].end)
			{
				int st=(*sch)[index-1].end;
				while(st<(*sch)[index].start)
				{
					for(int j=0;j<CORE;j++)
					{
						profile<<"0.00";
						if(j<CORE-1)
						{
							profile<<"\t";
						}
						else
						{
							profile<<"\n";
						}
					}
					st=st+1;
					lines=lines+1;
				}
			}
		}
		for(int j=0;j<CORE;j++)
		{
			if((*sch)[index].core!=j || lines<(*sch)[index].start)
			{
				profile<<"0.00";
			}
			else
			{
				profile<<(*sch)[index].power;
				average_power[j]=average_power[j]+(*sch)[index].power;
				index=index+1;
			}
			if(j<CORE-1)
			{
				profile<<"\t";
			}
		}
		profile<<"\n";
		lines=lines+1;

	//	cout<<"lines "<<lines<<" sch start "<<(*sch)[index].start<<endl;

		if( false && lines<(*sch)[index].start)
		{
			cout<<" error encountered "<<endl;

			for(unsigned int z=index-10;z<=index;z++)
			{
				cout<<"execution "<<z<<" core "<<(*sch)[z].core<<" power "<<(*sch)[z].power<<" start "<<(*sch)[z].start<<" end "<<(*sch)[z].end<<endl;
			}
			profile.close();

			exit(1);
		}

	}

	long end=(*sch)[sch->size()-1].end;

	//cout<<"lines "<<lines<<" end "<<end<<endl;

	while(end<hyperperiod)
	{
		for(int j=0;j<CORE;j++)
		{
			profile<<"0.00";
			if(j<CORE-1)
			{
				profile<<"\t";
			}
			else
			{
				profile<<"\n";
			}
		}
		end=end+1;
	}
	profile.close();

	for(unsigned int i=0;i<CORE;i++)
	{
		average_power[i]=average_power[i]/((float)hyperperiod);
		power_steady<<average_power[i];
		if(i<CORE-1)
		{
			power_steady<<"\t";
		}
	}
	power_steady<<endl;
	power_steady.close();

}


void generate_power_profile(vector<mprofile> *profile,vector<long_schedule>*sch, long hyperperiod)
{
	unsigned int index=0;

	mprofile temp;
	long lines;

	while(index<sch->size())
	{
		if(index>0)
		{
			if((*sch)[index].start>(*sch)[index-1].end)
			{
				int st=(*sch)[index-1].end;
				temp.time=((double)st)/W_INT*GRANULARITY;
				while(st<(*sch)[index].start)
				{
					for(int j=0;j<CORE;j++)
					{
						temp.val[j]=0.00;
					}
					st=st+1;
					lines=lines+1;
					profile->push_back(temp);
				}
			}
		}
		temp.time=((double)lines)/W_INT*GRANULARITY;
		for(int j=0;j<CORE;j++)
		{
			if((*sch)[index].core!=j || lines<(*sch)[index].start)
			{
				temp.val[j]=0.00;
			}
			else
			{
				temp.val[j]=(*sch)[index].power;
				index=index+1;
			}
		}
		profile->push_back(temp);
		lines=lines+1;

	//	cout<<"lines "<<lines<<" sch start "<<(*sch)[index].start<<endl;

		if( false && lines<(*sch)[index].start)
		{
			cout<<" error encountered "<<endl;

			for(unsigned int z=index-10;z<=index;z++)
			{
				cout<<"execution "<<z<<" core "<<(*sch)[z].core<<" power "<<(*sch)[z].power<<" start "<<(*sch)[z].start<<" end "<<(*sch)[z].end<<endl;
			}

			exit(1);
		}
	}

	long end=(*sch)[sch->size()-1].end;

	while(end<hyperperiod)
	{
		temp.time=end*W_INT/GRANULARITY;
		for(int j=0;j<CORE;j++)
		{
			temp.val[j]=0.00;

		}
		profile->push_back(temp);
		end=end+1;
	}


}




void total_comps(vector<long_schedule>*sch,vector<long_task>*tasks, vector<interval_s>*intervals)
{
	long total_comps[tasks->size()];

	long total_comps2[tasks->size()];

	float hyperperiod=compute_lcm(tasks);

	for(unsigned int i=0;i<tasks->size();i++)
	{
		total_comps[i]=0;
		total_comps2[i]=0;
	}

	for(unsigned int i=0;i<sch->size();i++)
	{
		total_comps[(*sch)[i].task_id]=total_comps[(*sch)[i].task_id]+((float)(*sch)[i].end)-((float)((*sch)[i].start));
	}

	for(unsigned int i=0;i<intervals->size();i++)
	{
		for(unsigned int j=0;j<tasks->size();j++)
		{
			for(unsigned int k=0;k<CORE;k++)
			{
				total_comps2[j]=total_comps2[j]+(*intervals)[i].computations[j][k];
			//	if(j==4 && (*intervals)[i].computations[j][k]>0)
			//	{
			//		cout<< " start "<<(*intervals)[i].start<<" end"<<(*intervals)[i].end<<" computations "<<total_comps2[j]<<endl;
			//	}
			}
		}
	}

	for(unsigned int i=0;i<tasks->size();i++)
	{
		cout<<"task "<<i<<" done computations "<<total_comps[i]<<" done comps 2 "<<total_comps2[i]<<" required computations "<<((float)(*tasks)[i].computation_time)/((float)(*tasks)[i].period)*hyperperiod<<endl;
	}


	cout<<" last interval start"<<(*intervals)[intervals->size()-1].start<<" end "<<(*intervals)[intervals->size()-1].end<<endl;


}

void populate_beta()
{

	//cout<<"populating beta matrix"<<endl;
	ifstream f("gams_files/TTI_Coeff");

	if(!f.is_open())
	{
		cout<<"error opening file in populate beta"<<endl;
		exit(1);
	}

	string line;
	getline(f,line);
	getline(f,line);
	getline(f,line);
	getline(f,line);
	getline(f,line);
	getline(f,line);
	getline(f,line);

	vector<string>elem;
	vector<string>delim;
	delim.push_back("\t");
	delim.push_back(" ");

	int i=0;
	while(getline(f,line))
	{
		split(&line,&delim,&elem);

		if(elem.size()>0)
		{
			if(elem.size()!=CORE+1)
			{
				cout<<"error in populate beta size"<<elem.size()<<" required size "<<CORE+1<<endl;

				for(unsigned int z=0;z<elem.size();z++)
				{
					cout<<elem[z]<<endl;
				}

				exit(1);
			}

			//cout<<line<<endl;

			for(unsigned int j=1;j<elem.size();j++)
			{
				beta_multi[i][j-1]=1.0/atof(elem[j].c_str());
			}
			i=i+1;
		}

		elem.clear();

	}

}







