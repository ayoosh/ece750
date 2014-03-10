#define GRANULARITY 10.00


float beta=3.71;



struct float_task {
	float computation_time;
	float computations;
	float next_start;
	float period;
	float power;
	int index;
};


struct float_schedule {
	int task_id;
	double start;
	double end;
	double speed;
	double arrival;
	double power;
};


struct profile {
	double time;
	double temperature;

};




double heat(double init_temp, double power, double time) {
	double t = time / GRANULARITY;
	return (power / beta * (1 - exp(-1 * beta * t))
			+ init_temp * exp(-1 * beta * t));
}


double cool(double init_temp, double time) {
	double t = time / GRANULARITY;
	return (init_temp * exp(-1 * beta * t));
}



void compute_profile(vector<float_schedule>* sch, vector<float_task>*tasks,int mode)
{
#if(ENABLE_PRINTS)

	cout<<"Hyperperiod:"<<tasksets[0].hyperperiod<<" total thermal impact:"<<tasksets[0].TTI<<" utilization:"<<tasksets[0].c_util<<" thermal util:"<<tasksets[0].t_util<<" average_power"<<tasksets[0].average_power<<endl;
#endif
	float initial_temperature = 0;
	vector<profile> temperature;
	profile ttemp;
	ttemp.time = 0;
	ttemp.temperature = initial_temperature;
	temperature.push_back(ttemp);

	ofstream schedule;

	double hyperperiod=compute_lcm(tasks);
	//cout<<"hyperperiod="<<hyperperiod<<endl;

	for (unsigned int i = 0; i < sch->size(); i++)
	{
		if (i < sch->size() - 1)
		{
			ttemp.time = (*sch)[i].end;
			ttemp.temperature = heat(temperature[temperature.size() - 1].temperature,(*sch)[i].power,ttemp.time - temperature[temperature.size() - 1].time);
			temperature.push_back(ttemp);

			double next_start = (*sch)[i + 1].start;

			if (next_start > temperature[temperature.size() - 1].time)
			{
				ttemp.time = (*sch)[i + 1].start;
				ttemp.temperature = cool(temperature[temperature.size() - 1].temperature,ttemp.time - temperature[temperature.size() - 1].time);
				temperature.push_back(ttemp);
			}

		}

		else {
			ttemp.time = hyperperiod;
			ttemp.temperature = cool(temperature[temperature.size() - 1].temperature,ttemp.time - (*sch)[sch->size() - 1].end);
			temperature.push_back(ttemp);
#if(ENABLE_PRINTS)
			cout<<"end temperature"<<ttemp.temperature<<endl;
#endif
		}
	//	if(mode==2)
	//	cout<<" task "<<(*sch)[i].task_id<<" start "<<(*sch)[i].start<<" end "<<(*sch)[i].end<<" power "<<(*sch)[i].power<<endl;

	//	cout<<" task "<<(*sch)[i].task_id<<" start "<<(*sch)[i].start<<" end "<<(*sch)[i].end<<" power "<<(*sch)[i].power<<endl;
///	cout<<" time "<<ttemp.time<<" temperature "<<ttemp.temperature<<endl;
//		cout<<" beta "<<beta<<endl;
	}


	double steady_temperature = (temperature[temperature.size() - 1].temperature)/ (1 - exp(-1 * beta * hyperperiod/GRANULARITY ));

	for (unsigned int i = 0; i < temperature.size(); i++)
	{
		temperature[i].temperature = temperature[i].temperature+ cool(steady_temperature, temperature[i].time);
	}

	cout<<" steady temperature "<<steady_temperature<<" threshold "<<corrected_threshold<<endl;
	ofstream thermal_profile;

	switch (mode) {
	case 1:
		thermal_profile.open("profile_float");
		break;
	case 2:
		thermal_profile.open("profile_dynamic");
		break;
	case 3:
		thermal_profile.open("profile_no_scaling");
		break;
	default:
		thermal_profile.open("profile_default");
		break;
	}

	thermal_profile << "#Time\tTemparature\n";

	bool thermal_violation = false;
	float max_temp = 0;
	for (unsigned int i = 0; i < temperature.size(); i++) {
		thermal_profile << temperature[i].time/GRANULARITY << "\t"
				<< temperature[i].temperature +40<< endl;
		if (temperature[i].temperature > corrected_threshold) {
			thermal_violation = true;
		}
		if (temperature[i].temperature > max_temp) {
			max_temp = temperature[i].temperature;
		}
	}

//	cout<<"thermal profile generated "<<endl;

	thermal_profile.close();
	float c_util = 0.00;
	float t_util = 0.00;
	for (unsigned int i = 0; i < tasks->size(); i++) {
		c_util = c_util
				+ ((float) (*tasks)[i].computation_time)
						/ ((float) (*tasks)[i].period);
		t_util = t_util
				+ hyperperiod / ((float) (*tasks)[i].period)
						* (*tasks)[i].power * (*tasks)[i].computation_time
						/ beta;
	}

//	cout<<"task set utilizations generated "<<endl;

	t_util = t_util / ((float) (hyperperiod * corrected_threshold));

	ofstream global_results;

	stringstream fname;

//	cout<<"mode "<<mode<<endl;

	switch (mode) {
	case 1:
		fname.str("");
		fname << "results_float" << seed;
		break;
	case 2:
		fname.str("");
		fname << "results_dynamic" << seed;
		break;
	case 3:
		fname.str("");
		fname << "results_no_scaling" << seed;
		break;
	default:
		fname.str("");
		fname << "results_default" << seed;
		break;
	}

	global_results.open(fname.str().c_str(), fstream::app);

	global_results << c_util << "\t" << t_util << "\t" << thermal_violation
			<< "\t" << max_temp << "\t"
			<< temperature[temperature.size() - 1].time << "\t" << tasks->size()
			<< endl;
	global_results.close();


	cout<<"taskset simulated"<<endl;
#if(ENABLE_PRINTS)

	cout<<"corrected_threshold"<<corrected_threshold<<endl;
#endif
}

