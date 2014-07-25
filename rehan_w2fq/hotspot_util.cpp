/*
 * hotspot_util.cpp
 *
 *  Created on: Jan 13, 2014
 *      Author: rehan
 */

#include "scheduler.h"

void multi_simulate(string power_profile, string flp,int mode, double avg_power)
{

	ofstream outfile;
	vector<trace>ttrace;
	stringstream temp1,temp2;
	temp1<<power_profile<<"_steady";
	call_hotspot(temp1.str(), "multi.flp");

	temp1.str("");
	temp1<<power_profile<<".ptrace";
	temp2<<power_profile<<".ttrace";


	bool cont=true;

	call_hotspot(temp1.str(), temp2.str(), flp,"hotspot_files/steady");
	trace end_temp;
	read_ttrace(temp2.str(), &ttrace);
	end_temp=ttrace[ttrace.size()-1];

	double max_temp;

	while(cont)
	{
		ttrace.clear();
		cout<<"new iteration"<<endl;
		cout<<end_temp.val[0]<<"\t"<<end_temp.val[1]<<"\t"<<end_temp.val[2]<<endl;
		cont=false;
		call_hotspot(temp1.str(), temp2.str(), flp,"end_temp");
		max_temp=read_ttrace(temp2.str(), &ttrace);

		for(unsigned int i=0;i<CORE;i++)
		{
			cout<<"maximum temperature "<<max_temp<<endl;
			cout<<end_temp.val[i]<<"|"<<ttrace[ttrace.size()-1].val[i]<<endl;
			if(fabs(end_temp.val[i]-ttrace[ttrace.size()-1].val[i])>0.1)
			{
				cont=true;
				end_temp=ttrace[ttrace.size()-1];
				break;
			}
		}
	}

	switch(mode)
	{
	case 0:
		outfile.open("results_task_assign",std::fstream::app);
		break;
	case 1:
		outfile.open("results_instance_assign",std::fstream::app);
		break;
	default:
		outfile.open("results_defaultassign",std::fstream::app);
		break;
	}

	bool violation=max_temp>75.00;

	outfile<<avg_power<<"\t"<<violation<<"\t"<<max_temp<<endl;

}



void call_hotspot(string ptrace, string flp)
{
	stringstream command;

	string config;
	int wint_temp=round(W_INT*1000000/GRANULARITY);

	switch(wint_temp)
	{
		case 10000:
			config="10ms.config";
			break;
		case 1000:
			config="1ms.config";
			break;
		case 100:
			config="100us.config";
			break;
		case 10:
			config="10us.config";
			break;
		case 1:
			config="1us.config";
			break;
		default:
			cout<<"config case not recognized. W_INT"<<wint_temp<<endl;
			exit(1);
			break;
	}

	cout<<"wint temp"<<wint_temp<<endl;
	cout<<" calling hotspot"<<endl;
	command<<"~rehan/HotSpot-5.02/hotspot -c hotspot_files/"<<config<<" -f hotspot_files/"<< flp<<" -p hotspot_files/"<<ptrace<<" -steady_file hotspot_files/steady";

	cout<<command.str()<<endl;

	system(command.str().c_str());

}

void call_hotspot(string ptrace, string ttrace,string flp, string init)
{
	stringstream command;

	string config;
	int wint_temp=round(W_INT*1000000/GRANULARITY);

	switch(wint_temp)
	{
		case 10000:
			config="10ms.config";
			break;
		case 1000:
			config="1ms.config";
			break;
		case 100:
			config="100us.config";
			break;
		case 10:
			config="10us.config";
			break;
		case 1:
			config="1us.config";
			break;
		default:
			cout<<"config case not recognized. W_INT"<<W_INT<<endl;
			exit(1);
			break;
	}

	cout<<"calling hotspot"<<endl;

	command<<"~rehan/HotSpot-5.02/hotspot -c hotspot_files/"<<config<<" -f hotspot_files/"<< flp<<" -init_file "<<init<<" -p hotspot_files/"<<ptrace<<" -steady_file hotspot_files/steady -o hotspot_files/"<<ttrace;
	cout<<command.str()<<endl;

	system(command.str().c_str());

}

double read_ttrace(string fname, vector<trace>*therm)
{
	double max_temp=0.00;
	stringstream tf;
	tf<<"hotspot_files/"<<fname;

	cout<<"reading ttrace "<<fname<<endl;
	ifstream f(tf.str().c_str());
	if(!f.is_open())
	{
		cout<<" error reading input file in read ttrace function"<<endl;
		exit(1);
	}

	string line;
	vector<string>delim,elem;
	delim.push_back("\t");
	delim.push_back(" ");


	trace temp;

	getline(f,line);
	while(getline(f,line))
	{
		elem.clear();
		split(&line,&delim,&elem);
		assert(elem.size()==CORE);

		for(unsigned int i=0;i<CORE;i++)
		{
			temp.val[i]=atof(elem[i].c_str());
			max_temp=max_temp<temp.val[i]?temp.val[i]:max_temp;
		}
		therm->push_back(temp);
	}

	cout<<"ttrace size "<<therm->size()<<endl;
	return max_temp;
}
