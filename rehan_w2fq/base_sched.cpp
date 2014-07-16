/*
 * base_sched.cpp
 *
 *  Created on: Jun 16, 2013
 *      Author: rehan
 */

#include "scheduler.h"
extern vector<taskset> tasksets;
extern int thermal_optimal;
extern double beta;
extern double corrected_threshold;
extern int seed;
extern float global_sort_power;

#define min(a,b) ({ __typeof__ (a) _a = (a); \
                          __typeof__ (b) _b = (b); \
                        _a < _b ? _a : _b; })


bool compare_deadline(instance a, instance b) {
    if (a.deadline < b.deadline) {
        return true;
    }
    else if(a.deadline == b.deadline) {
        if(a.arrival < b.arrival)
            return true;
    }

    return false;
}

// true: Thermal Violation
bool ab_compute_profile(vector<float_schedule>* sch, bool calculate_steadytemp) {

    
    float initial_temperature = 0;
    double static steady_temperature;
	vector<profile> temperature;
	profile ttemp;
	ttemp.time = 0;
	ttemp.temperature = initial_temperature;
	temperature.push_back(ttemp);

	for (unsigned int i = 0; i < sch->size(); i++) {
		if (i < sch->size() - 1) {
			ttemp.time = (*sch)[i].end;
			ttemp.temperature = heat(
					temperature[temperature.size() - 1].temperature,
					(*sch)[i].power,
					ttemp.time - temperature[temperature.size() - 1].time);
			temperature.push_back(ttemp);
			float next_start = (*sch)[i + 1].start;
			if (next_start > temperature[temperature.size() - 1].time) {
				ttemp.time = (*sch)[i + 1].start;
				ttemp.temperature = cool(
						temperature[temperature.size() - 1].temperature,
						ttemp.time - temperature[temperature.size() - 1].time);
				temperature.push_back(ttemp);
			}

		}

		else {
			ttemp.time = tasksets[0].hyperperiod;
			ttemp.temperature = cool(
					temperature[temperature.size() - 1].temperature,
					tasksets[0].hyperperiod - (*sch)[sch->size() - 1].end);
			temperature.push_back(ttemp);
			//cout<<"end temperature"<<ttemp.temperature<<endl;
		}
	}

    if (calculate_steadytemp) {
	    steady_temperature = (temperature[temperature.size() - 1].temperature)
			    / (1 - exp(-1 * beta * tasksets[0].hyperperiod / GRANULARITY));
        //cout << "Steady Temperature : " << steady_temperature << endl;
    }

	for (unsigned int i = 0; i < temperature.size(); i++) {
		temperature[i].temperature = temperature[i].temperature \
				+ cool(steady_temperature, temperature[i].time);
	}

	ofstream thermal_profile;

    thermal_profile.open("profile");
	thermal_profile << "#Time\tTemparature\n";

	bool thermal_violation = false;
	float max_temp = 0;
	for (unsigned int i = 0; i < temperature.size(); i++) {
		thermal_profile << temperature[i].time << "\t"
				<< temperature[i].temperature << endl;
		if (temperature[i].temperature > corrected_threshold) {
			thermal_violation = true;
		}
		if (temperature[i].temperature > max_temp) {
			max_temp = temperature[i].temperature;
		}
	}

    //cout << "max temp = " << max_temp << endl;

	thermal_profile.close();
    return thermal_violation;
}

void ab_edf_schedule(vector<float_schedule> *edf, vector<instance> *instances) {

    vector<instance> active;

	for (unsigned int i = 0; i < instances->size(); i++) {
		(*instances)[i].comps_done = 0;
        (*instances)[i].comps_left = (*instances)[i].computation_time;
		(*instances)[i].next_start = 0;
        (*instances)[i].speed = 1;
	}

	float_schedule temp;
    unsigned int i=0;
    double comps_avail=0, comps_left = 0, sched_start=0;
   
    double start = (*instances)[0].arrival;

    while(start < tasksets[0].hyperperiod ) {
        
        for(; i < instances->size(); i++){

            if(start == (*instances)[i].arrival) {
                active.push_back((*instances)[i]);
            }
            else {
                sort(active.begin(), active.end(), compare_deadline);
                break;
            }
        }
        comps_avail = (i < instances->size()) ? ((*instances)[i].arrival - start) : (tasksets[0].hyperperiod - start);
        sched_start = start;
        start += comps_avail;
        while((comps_avail > 0) && active.size()) {
            comps_left = min(active[0].comps_left, comps_avail);
            active[0].comps_done += comps_left;
            active[0].comps_left -= comps_left;
            
            temp.start = sched_start;
            sched_start += comps_left;
            temp.end = temp.start + comps_left;
            temp.power = active[0].power;
            temp.task_id = active[0].task_id;
            temp.speed = active[0].speed;
            if(active[0].comps_left == 0) {
                active.erase(active.begin() + 0);
            }
            comps_avail -= comps_left;
            edf->push_back(temp);
        }

    }
	
	for(unsigned int i=0;i<edf->size();i++)
	{
//		cout<<i<<": Task:"<<(*edf)[i].task_id<<"\t"<<" start:"<<(*edf)[i].start<<"\t"<<" end: "<<(*edf)[i].end<<"\t"<<" power: "<<(*edf)[i].power<<endl;
	}
	//verify(edf, tasks);

}
/*
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!#####################
 * We do not want the code below this
 */


void edf_schedule(vector<task> * tasks, vector<schedule>*edf) {
	vector<int> times;
	imp_times(tasks, &times);
//#if(ENABLE_PRINTS)

	for(unsigned int i=0;i<times.size();i++)
	{
		cout<<"times: "<<i<<":"<<times[i]<<endl;
	}
//#endif

	for (unsigned int i = 0; i < tasks->size(); i++) {
		(*tasks)[i].computations = 0;
		(*tasks)[i].next_start = 0;
	}

	schedule temp;
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
//#if(ENABLE_PRINTS)

	for(unsigned int i=0;i<edf->size();i++)
	{
		cout<<i<<": Task:"<<(*edf)[i].task_id<<" start:"<<(*edf)[i].start<<" end: "<<(*edf)[i].end<<endl;
	}
//#endif

	verify(edf, tasks);

}


void edf_schedule(vector<float_task>*tasks, vector<float_schedule>*edf)
{
	vector<float> times;
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

	float_schedule temp;
	for (unsigned int i = 0; i < times.size() - 1; i++)
	{
		float start = times[i];
		while (start < times[i+1])
		{
			int id = min_deadline(tasks, start);
			if (id == -1)
			{
				break;
			}

			float computations_left = (*tasks)[id].computation_time - (*tasks)[id].computations;
			(*tasks)[id].computations = (times[i + 1] - start) >= computations_left ?(*tasks)[id].computation_time :
										(*tasks)[id].computations + times[i + 1] - start;
			temp.start = start;
			temp.end =(times[i + 1] - start) >= computations_left ?temp.start + computations_left : times[i + 1];
			temp.task_id = id;
			temp.arrival=floor(start/(*tasks)[temp.task_id].period)*(*tasks)[temp.task_id].period;

			temp.power=(*tasks)[id].power;
			if ((*tasks)[id].computations == (*tasks)[id].computation_time && start >= (*tasks)[id].next_start)
			{
				(*tasks)[id].computations = 0;
				(*tasks)[id].next_start = floor(start / (*tasks)[id].period) * (*tasks)[id].period + (*tasks)[id].period;
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


}







void populate_slacks(vector<slack>*slacks, vector<schedule>*sch) {
	slack temp;
	temp.start = 0;
	temp.end = 0;
	for (unsigned int i = 0; i < sch->size(); i++) {
		temp.end = (*sch)[i].start;
		if (temp.end > temp.start) {
			slacks->push_back(temp);
		}
		temp.start = (*sch)[i].end;
	}
	temp.start = (*sch)[sch->size() - 1].end;
	temp.end = tasksets[0].hyperperiod;

	slacks->push_back(temp);
}

/*double heat(double init_temp, double power, int time) {
	double t = ((float) time) / GRANULARITY;
	return (power / beta * (1 - exp(-1 * beta * t))
			+ init_temp * exp(-1 * beta * t));
}*/
double heat(double init_temp, double power, double time) {
	double t = time / GRANULARITY;
	return (power / beta * (1 - exp(-1 * beta * t))
			+ init_temp * exp(-1 * beta * t));
}

void heat(double init_temp[CORE], double power[CORE], double time, double* out) {


cout<<"time"<<time<<endl;
	for(int i=0;i<CORE;i++)
	{
		out[i]=0;
		for(int j=0;j<CORE;j++)
		{
			out[i]=out[i]+power[j]/beta_multi[i][j]*(1-exp(-1*beta_multi[i][j]*time))+ init_temp[j]*exp(-1*beta_multi[i][j]*time);
		}
	}
	//return (power / beta * (1 - exp(-1 * beta * t))
	//		+ init_temp * exp(-1 * beta * t));
}


double integral(double initial, double final, double power, int time)
{
	return (initial+((power/beta)*((double)time)/GRANULARITY)-final/beta);
}

/*double cool(double init_temp, int time) {
	double t = ((double) time) / GRANULARITY;
	return (init_temp * exp(-1 * beta * t));
}*/

double cool(double init_temp, double time) {
	double t = time / GRANULARITY;
	return (init_temp * exp(-1 * beta * t));
}


void compute_profile(vector<schedule>* sch, vector<task>*tasks,
		double thermal_util) {
//#if(ENABLE_PRINTS)

	cout<<"Hyperperiod:"<<tasksets[0].hyperperiod<<" total thermal impact:"<<tasksets[0].TTI<<" utilization:"<<tasksets[0].c_util<<" thermal util:"<<tasksets[0].t_util<<" average_power"<<tasksets[0].average_power<<endl;
//#endif
	float initial_temperature = 0;
	vector<profile> temperature;
	profile ttemp;
	ttemp.time = 0;
	ttemp.temperature = initial_temperature;
	temperature.push_back(ttemp);

	for (unsigned int i = 0; i < sch->size(); i++) {
		if (i < sch->size() - 1) {
			ttemp.time = (*sch)[i].end;
			ttemp.temperature = heat(
					temperature[temperature.size() - 1].temperature,
					(*tasks)[(*sch)[i].task_id].power,
					ttemp.time - temperature[temperature.size() - 1].time);
			temperature.push_back(ttemp);
			int next_start = (*sch)[i + 1].start;
			if (next_start > temperature[temperature.size() - 1].time) {
				ttemp.time = (*sch)[i + 1].start;
				ttemp.temperature = cool(
						temperature[temperature.size() - 1].temperature,
						ttemp.time - temperature[temperature.size() - 1].time);
				temperature.push_back(ttemp);
			}

		}

		else {
			ttemp.time = tasksets[0].hyperperiod;
			ttemp.temperature = cool(
					temperature[temperature.size() - 1].temperature,
					tasksets[0].hyperperiod - (*sch)[sch->size() - 1].end);
			temperature.push_back(ttemp);
//#if(ENABLE_PRINTS)
			cout<<"end temperature"<<ttemp.temperature<<endl;
//#endif
		}
	}

	double steady_temperature = (temperature[temperature.size() - 1].temperature)
			/ (1 - exp(-1 * beta * tasksets[0].hyperperiod / GRANULARITY));
	for (unsigned int i = 0; i < temperature.size(); i++) {
		temperature[i].temperature = temperature[i].temperature
				+ cool(steady_temperature, temperature[i].time);
	}

	ofstream thermal_profile;

	switch (thermal_optimal) {
	case 1:
		thermal_profile.open("profile");
		break;
	case 2:
		thermal_profile.open("profile_opt");
		break;
	case 3:
		thermal_profile.open("profile_disc");
		break;
	case 4:
		thermal_profile.open("profile_maxfirst");
		break;
	case 5:
		thermal_profile.open("profile_staticspeed");
		break;
	case 6:
		thermal_profile.open("profile_matlab");
		break;
	case 7:
		thermal_profile.open("profile_nocons");
		break;
	default:
		thermal_profile.open("profile_default");
		break;
	}

	thermal_profile << "#Time\tTemparature\n";

	bool thermal_violation = false;
	float max_temp = 0;
	for (unsigned int i = 0; i < temperature.size(); i++) {
		thermal_profile << temperature[i].time << "\t"
				<< temperature[i].temperature << endl;
		if (temperature[i].temperature > corrected_threshold) {
			thermal_violation = true;
		}
		if (temperature[i].temperature > max_temp) {
			max_temp = temperature[i].temperature;
		}
	}

	thermal_profile.close();
	float c_util = 0.00;
	float t_util = 0.00;
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

	ofstream global_results;

	stringstream fname;

	switch (thermal_optimal) {
	case 1:
		fname.str("");
		fname << "results" << seed;
		break;
	case 2:
		fname.str("");
		fname << "results_opt" << seed;
		break;
	case 3:
		fname.str("");
		fname << "results_disc" << seed;
		break;
	case 4:
		fname.str("");
		fname << "results_maxfirst" << seed;
		t_util = thermal_util;
		break;
	case 5:
		fname.str("");
		fname << "results_staticspeed" << seed;
		t_util = thermal_util;
		break;
	case 6:
		fname.str("");
		fname << "results_matlab" << seed;
		t_util = thermal_util;
		break;
	case 7:
		fname.str("");
		fname << "results_nocons" << seed;
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
//#if(ENABLE_PRINTS)

	cout<<"corrected_threshold"<<corrected_threshold<<thermal_violation<<endl;
	
//#endif
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



void compute_profile(vector<schedule>* sch, vector<double> speeds,
		vector<task>*tasks, double thermal_util) {
#if(ENABLE_PRINTS)

	cout<<"Hyperperiod:"<<tasksets[0].hyperperiod<<" total thermal impact:"<<tasksets[0].TTI<<" utilization:"<<tasksets[0].c_util<<" thermal util:"<<tasksets[0].t_util<<" average_power"<<tasksets[0].average_power<<endl;
#endif
	float initial_temperature = 0;
	vector<profile> temperature;
	profile ttemp;
	ttemp.time = 0;
	ttemp.temperature = initial_temperature;
	temperature.push_back(ttemp);

	for (unsigned int i = 0; i < sch->size(); i++) {
		if (i < sch->size() - 1) {
			ttemp.time = (*sch)[i].end;
			ttemp.temperature = heat(
					temperature[temperature.size() - 1].temperature,
					(*tasks)[(*sch)[i].task_id].power,
					ttemp.time - temperature[temperature.size() - 1].time);
			temperature.push_back(ttemp);
			int next_start = (*sch)[i + 1].start;
			if (next_start > temperature[temperature.size() - 1].time) {
				ttemp.time = (*sch)[i + 1].start;
				ttemp.temperature = cool(
						temperature[temperature.size() - 1].temperature,
						ttemp.time - temperature[temperature.size() - 1].time);
				temperature.push_back(ttemp);
			}

		}

		else {
			ttemp.time = tasksets[0].hyperperiod;
			ttemp.temperature = cool(
					temperature[temperature.size() - 1].temperature,
					tasksets[0].hyperperiod - (*sch)[sch->size() - 1].end);
			temperature.push_back(ttemp);
#if(ENABLE_PRINTS)
			cout<<"end temperature"<<ttemp.temperature<<endl;
#endif
		}
	}

	double steady_temperature = (temperature[temperature.size() - 1].temperature)
			/ (1 - exp(-1 * beta * tasksets[0].hyperperiod / GRANULARITY));
	for (unsigned int i = 0; i < temperature.size(); i++) {
		temperature[i].temperature = temperature[i].temperature
				+ cool(steady_temperature, temperature[i].time);
	}

	ofstream thermal_profile;

	switch (thermal_optimal) {
	case 1:
		thermal_profile.open("profile");
		break;
	case 2:
		thermal_profile.open("profile_opt");
		break;
	case 3:
		thermal_profile.open("profile_disc");
		break;
	case 4:
		thermal_profile.open("profile_maxfirst");
		break;
	case 5:
		thermal_profile.open("profile_staticspeed");
		break;
	case 6:
		thermal_profile.open("profile_matlab");
		break;
	case 7:
		thermal_profile.open("profile_nocons");
		break;
	default:
		thermal_profile.open("profile_default");
		break;
	}

	thermal_profile << "#Time\tTemparature\n";

	bool thermal_violation = false;
	float max_temp = 0;
	for (unsigned int i = 0; i < temperature.size(); i++) {
		thermal_profile << temperature[i].time << "\t"
				<< temperature[i].temperature << endl;
		if (temperature[i].temperature > corrected_threshold) {
			thermal_violation = true;
		}
		if (temperature[i].temperature > max_temp) {
			max_temp = temperature[i].temperature;
		}
	}

	thermal_profile.close();
	// cout<<"steady_temp"<<steady_temperature<<endl;
	float c_util = 0.00;
	float t_util = 0.00;
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

	ofstream global_results;

	stringstream fname;

	switch (thermal_optimal) {
	case 1:
		fname.str("");
		fname << "results" << seed;
		break;
	case 2:
		fname.str("");
		fname << "results_opt" << seed;
		break;
	case 3:
		fname.str("");
		fname << "results_disc" << seed;
		break;
	case 4:
		fname.str("");
		fname << "results_maxfirst" << seed;
		t_util = thermal_util;
		break;
	case 5:
		fname.str("");
		fname << "results_staticspeed" << seed;
		t_util = thermal_util;
		break;
	case 6:
		fname.str("");
		fname << "results_matlab" << seed;
		t_util = thermal_util;
		break;
	case 7:
		fname.str("");
		fname << "results_nocons" << seed;
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
#if(ENABLE_PRINTS)

	cout<<"corrected_threshold"<<corrected_threshold<<endl;
#endif
}


void compute_profile_multi(vector<long_schedule>* sch, vector<long_task>*tasks)
{
	ofstream schedule;


	double hyperperiod=compute_lcm(tasks);
	hyperperiod=hyperperiod*W_INT/GRANULARITY;

	vector<mprofile>pprofile;
	generate_power_profile(&pprofile,sch, hyperperiod);

	vector<mprofile>tprofile;
	mprofile temp;
	temp.time=0;
	for(unsigned int i=0;i<CORE;i++)
	{
		temp.val[i]=0.00;
	}

	tprofile.push_back(temp);


	for (unsigned int i = 0; i < pprofile.size(); i++)
	{
		if (i < pprofile.size() - 1)
		{
			temp.time=pprofile[i+1].time;
			heat(tprofile[tprofile.size()-1].val, pprofile[i].val, temp.time-tprofile[tprofile.size()-1].time, temp.val);
			tprofile.push_back(temp);
		}

		else {
			temp.time = hyperperiod;
			heat(tprofile[tprofile.size()-1].val, pprofile[i].val, temp.time-tprofile[tprofile.size()-1].time, temp.val);
			tprofile.push_back(temp);
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

	double cooling_power[CORE];

	double steady_temperature[CORE];
	for(unsigned int i=0;i<CORE;i++)
	{
		cooling_power[i]=0;
		steady_temperature[i]=0;
		for(unsigned int j=0;j<CORE;j++)
		{
			steady_temperature[i]=steady_temperature[i]+tprofile[tprofile.size() - 1].val[j]/ (1 - exp(-1 * beta_multi[i][j] * hyperperiod ));
		}
	}//= (temperature[temperature.size() - 1].temperature)/ (1 - exp(-1 * beta * hyperperiod ));

	double cooling[CORE];
	for (unsigned int i = 0; i < tprofile.size() && false; i++)
	{
		heat(steady_temperature, cooling_power,tprofile[i].time,cooling);
		for(unsigned int j=0;j<CORE;j++)
		{
			tprofile[i].val[j] = tprofile[i].val[j]+ cooling[j];
		}
	}

	cout<<" steady temperature "<<steady_temperature<<" threshold "<<corrected_threshold<<endl;
	ofstream thermal_profile;

	thermal_profile.open("profile_multi");

	thermal_profile << "#Time\tTemparature\n";

	bool thermal_violation = false;
	float max_temp = 0;
	for (unsigned int i = 0; i < tprofile.size(); i++) {
		thermal_profile << tprofile[i].time << "\t";
		for(unsigned int j=0;j<CORE;j++)
		{
			thermal_profile<<tprofile[i].val[j]<<"\t";
			if (tprofile[i].val[j] > corrected_threshold) {
				thermal_violation = true;
			}
			if (tprofile[i].val[j] > max_temp) {
				max_temp = tprofile[i].val[j];
			}

		}
		thermal_profile<<endl;
	}

	thermal_profile.close();
	float c_util = 0.00;
	float average_power = 0.00;
	for (unsigned int i = 0; i < tasks->size(); i++) {
		c_util = c_util
				+ ((float) (*tasks)[i].computation_time)
						/ ((float) (*tasks)[i].period);
		average_power = average_power
				+ ((float)((*tasks)[i].power * (*tasks)[i].computation_time)) / ((float) (*tasks)[i].period);
	}


	ofstream global_results;

	stringstream fname;

	fname.str("");
	fname << "results_default" << seed;

	global_results.open(fname.str().c_str(), fstream::app);

	global_results << c_util << "\t" << average_power << "\t" << thermal_violation
			<< "\t" << max_temp << "\t"
			<< tprofile[tprofile.size() - 1].time << "\t" << tasks->size()
			<< endl;
	global_results.close();
#if(ENABLE_PRINTS)

	cout<<"corrected_threshold"<<corrected_threshold<<endl;
#endif
}


void compute_profile_dynamic(vector<schedule>* sch, vector<task>*tasks, double thermal_util,string append) {
#if(ENABLE_PRINTS)

	cout<<"Hyperperiod:"<<tasksets[0].hyperperiod<<" total thermal impact:"<<tasksets[0].TTI<<" utilization:"<<tasksets[0].c_util<<" thermal util:"<<tasksets[0].t_util<<" average_power"<<tasksets[0].average_power<<endl;
#endif
	double initial_temperature = 0;
	vector<profile> temperature;
	profile ttemp;
	ttemp.time = 0;
	ttemp.temperature = initial_temperature;
	temperature.push_back(ttemp);

	double integral=0;

	for (unsigned int i = 0; i < sch->size(); i++) {
		if (i < sch->size() - 1) {

			ttemp.time = (*sch)[i].end;
			ttemp.temperature = heat(temperature[temperature.size() - 1].temperature,(*tasks)[(*sch)[i].task_id].power*pow((*sch)[i].speed,3.0),ttemp.time - temperature[temperature.size() - 1].time);
			integral=integral + ((*tasks)[(*sch)[i].task_id].power*pow((*sch)[i].speed,3.0)/beta)*
					(((double)(ttemp.time - temperature[temperature.size() - 1].time))/GRANULARITY)
					-(ttemp.temperature-temperature[temperature.size() - 1].temperature)/beta;

			temperature.push_back(ttemp);
			int next_start = (*sch)[i + 1].start;
			if (next_start > temperature[temperature.size() - 1].time) {
				ttemp.time = (*sch)[i + 1].start;
				ttemp.temperature = cool(
						temperature[temperature.size() - 1].temperature,
						ttemp.time - temperature[temperature.size() - 1].time);
				temperature.push_back(ttemp);
				integral=integral+((temperature[temperature.size() - 1].temperature-ttemp.temperature)/beta);
			}

		}

		else {
			ttemp.time = (*sch)[i].end;
			ttemp.temperature = heat(temperature[temperature.size() - 1].temperature,(*tasks)[(*sch)[i].task_id].power*pow((*sch)[i].speed,3.0),ttemp.time - temperature[temperature.size() - 1].time);
			integral=integral + ((*tasks)[(*sch)[i].task_id].power*pow((*sch)[i].speed,3.0)/beta)*
								(((double)(ttemp.time - temperature[temperature.size() - 1].time))/GRANULARITY)
								-(ttemp.temperature-temperature[temperature.size() - 1].temperature)/beta;
			temperature.push_back(ttemp);

			ttemp.time = tasksets[0].hyperperiod;
			ttemp.temperature = cool(
					temperature[temperature.size() - 1].temperature,
					tasksets[0].hyperperiod - (*sch)[sch->size() - 1].end);

			integral=integral+((temperature[temperature.size() - 1].temperature-ttemp.temperature)/beta);
			temperature.push_back(ttemp);
#if(ENABLE_PRINTS)
			cout<<"end temperature"<<ttemp.temperature<<endl;
#endif
		}
	}



	double steady_temperature = (temperature[temperature.size() - 1].temperature)
			/ (1 - exp(-1 * beta * tasksets[0].hyperperiod / GRANULARITY));

	integral=integral+((steady_temperature-cool(steady_temperature, tasksets[0].hyperperiod))/beta);


	double average_temperature=integral/(((double)tasksets[0].hyperperiod)/GRANULARITY);

	for (unsigned int i = 0; i < temperature.size(); i++) {
		temperature[i].temperature = temperature[i].temperature
				+ cool(steady_temperature, temperature[i].time);
	}

	ofstream thermal_profile;

		thermal_profile.open("profile_dynamic");

	thermal_profile << "#Time\tTemparature\n";

	bool thermal_violation = false;
	double max_temp = 0;
	for (unsigned int i = 0; i < temperature.size(); i++) {
		thermal_profile << temperature[i].time << "\t"
				<< temperature[i].temperature << endl;
		if (temperature[i].temperature > corrected_threshold) {
			thermal_violation = true;
		}
		if (temperature[i].temperature > max_temp) {
			max_temp = temperature[i].temperature;
		}
	}



	thermal_profile.close();
	double c_util = 0.00;
	double t_util = 0.00;
	for (unsigned int i = 0; i < tasks->size(); i++) {
		c_util = c_util
				+ ((double) (*tasks)[i].computation_time)
						/ ((double) (*tasks)[i].period);
		t_util = t_util
				+ tasksets[0].hyperperiod / ((float) (*tasks)[i].period)
						* (*tasks)[i].power * (*tasks)[i].computation_time
						/ beta;
	}

	t_util = t_util / ((double) (tasksets[0].hyperperiod * corrected_threshold));

	ofstream global_results;

	stringstream fname;

		fname << "results_dynamic" <<append.c_str() <<seed;

	global_results.open(fname.str().c_str(), fstream::app);

	global_results << c_util << "\t" << t_util << "\t" << thermal_violation
			<< "\t" << max_temp << "\t"
			<< temperature[temperature.size() - 1].time << "\t" << tasks->size()<< "\t"<<average_temperature
			<< endl;
	global_results.close();
#if(ENABLE_PRINTS)

	cout<<"corrected_threshold"<<corrected_threshold<<endl;
#endif
}






void consolidate_schedule(vector<schedule>*sch, vector<task>*tasks) {

	if (sch->size() <= 0) {
		cout << "WARNING. EMPTY SCHEDULE ENTERED" << endl;
		return;
	}

	vector<schedule> temp;
	for (unsigned int i = 0; i < sch->size(); i++) {
		temp.push_back((*sch)[i]);
	}
	sch->clear();

	sch->push_back(temp[0]);

	for (unsigned int i = 1; i < temp.size(); i++) {
		if (temp[i].task_id == temp[i - 1].task_id
				&& temp[i].start == temp[i - 1].end
				&& temp[i].start % (*tasks)[temp[i].task_id].period != 0) {
			(*sch)[sch->size() - 1].end = temp[i].end;
		} else {
			sch->push_back(temp[i]);
		}
	}
	temp.clear();

}

void edl_schedule(vector<schedule>*edl, vector<schedule>*edf,
		vector<task>* tasks, vector<slack>*slacks) {
	vector<int> times;
	imp_times(tasks, &times);
	vector<slack> edl_slacks;
	slack temp;
	int hyperperiod = tasksets[0].hyperperiod;
	for (unsigned int i = 0; i < slacks->size(); i++) {
		temp.start = hyperperiod - (*slacks)[slacks->size() - 1 - i].end;
		temp.end = hyperperiod - (*slacks)[slacks->size() - 1 - i].start;
		edl_slacks.push_back(temp);
	}

	temp.start = hyperperiod;
	temp.end = hyperperiod + edl_slacks[0].end;
	edl_slacks.push_back(temp);
	edl_slacks.push_back(temp);
	schedule temp_sch;

	for (unsigned int i = 0; i < tasks->size(); i++) {
		(*tasks)[i].computations = 0;
		(*tasks)[i].next_start = 0;
	}
	unsigned int slack_index = 0;
	unsigned int times_index = 0;
	times.push_back(hyperperiod);

	int start = 0;
	int end = 0;
	while (end < hyperperiod) {

		while (edl_slacks[slack_index].end <= end
				&& slack_index < (edl_slacks.size() - 1)) {
			slack_index = slack_index + 1;
		}
		while (times[times_index] <= end && times_index < (times.size() - 1)) {
			times_index = times_index + 1;
		}
		if (start >= edl_slacks[slack_index].start
				&& start <= edl_slacks[slack_index].end) {
			start = edl_slacks[slack_index].end;
		}

		end = times[times_index] < edl_slacks[slack_index + 1].start ?
				times[times_index] : edl_slacks[slack_index + 1].start;

		if (end <= start) {
			cout << "encountered error " << start << "|" << end << endl;
			cout << "\nprinting times " << endl;

			for (unsigned int i = 0; i < times.size(); i++) {
				cout << "times " << i << ":" << times[i] << endl;
			}

			cout << "printing slacks" << endl;
			for (unsigned int i = 0; i < edl_slacks.size(); i++) {
				cout << "slack " << i << " start " << edl_slacks[i].start
						<< " end " << edl_slacks[i].end << endl;
			}

			cout << "\nprinting edf slacks" << endl;

			for (unsigned int i = 0; i < edl_slacks.size(); i++) {
				cout << "slack " << i << " start " << (*slacks)[i].start
						<< " end " << (*slacks)[i].end << endl;
			}

			cout << "\n\nprinting edf schedule\n\n";

			for (unsigned int i = 0; i < edf->size(); i++) {
				cout << "task " << (*edf)[i].task_id << " start "
						<< (*edf)[i].start << " end " << (*edf)[i].end << endl;
			}

			cout << "int computation of end: slack index" << slack_index + 1
					<< " slack start" << edl_slacks[slack_index + 1].start
					<< "end " << edl_slacks[slack_index + 1].end
					<< " times index" << times_index << " value"
					<< times[times_index] << endl;
			exit(1);
		}

		while (start < end) {
			int id = min_deadline(tasks, start);
			if (id == -1) {
				cout << "possible error" << endl;
				exit(1);
				break;
			}
			int computations_left = (*tasks)[id].computation_time
					- (*tasks)[id].computations;
			(*tasks)[id].computations =
					(end - start) >= computations_left ?
							(*tasks)[id].computation_time :
							(*tasks)[id].computations + end - start;
			temp_sch.start = start;

			temp_sch.end =
					(end - start) >= computations_left ?
							temp_sch.start + computations_left : end;

			temp_sch.task_id = id;
			// cout<<"edl-------task: "<<temp_sch.task_id<<" start: "<<temp_sch.start<< " end: "<<temp_sch.end<<" computations "<<(*tasks)[id].computations<<" computation_time "<<(*tasks)[id].computation_time<<endl;

			if ((*tasks)[id].computations == (*tasks)[id].computation_time
					&& start >= (*tasks)[id].next_start) {
				(*tasks)[id].computations = 0;
				(*tasks)[id].next_start = start / (*tasks)[id].period
						* (*tasks)[id].period + (*tasks)[id].period;
			}
			start = temp_sch.end;
			edl->push_back(temp_sch);
		}
		start = end;
	}
}

void edl_schedule2(vector<schedule>*edl, vector<schedule>*edf) {
	schedule temp;
	for (int i = edf->size() - 1; i >= 0; i--) {
		//	cout<<i<<endl;
		temp.start = tasksets[0].hyperperiod - (*edf)[i].end;
		temp.end = tasksets[0].hyperperiod - (*edf)[i].start;
		temp.task_id = (*edf)[i].task_id;
		edl->push_back(temp);
	}
}

void verify(vector<schedule>*sch, vector<task>*tasks) {
	int computations[tasks->size()];
	for (unsigned int i = 0; i < tasks->size(); i++) {
		computations[i] = 0;
	}

	for (unsigned int i = 0; i < sch->size(); i++) {
		int id = (*sch)[i].task_id;
		computations[id] = computations[id] + (*sch)[i].end - (*sch)[i].start;
		if ((*sch)[i].start / (*tasks)[id].period
				* (*tasks)[id].computation_time > computations[id]+0.0001) {
			cout << "violation occured at " << i << endl;
			for (unsigned int j = 0; j <= i; j++) {
				cout << j << "task " << (*sch)[j].task_id << " start:"
						<< (*sch)[j].start << " end:" << (*sch)[j].end << endl;
			}

			cout << "possible deadline miss" << endl;
			cout << "start:" << (*sch)[i].start << " task id"
					<< (*sch)[i].task_id << " required computations: "
					<< (*sch)[i].start / (*tasks)[id].period
							* (*tasks)[id].computation_time
					<< " done computations" << computations[id] << endl;
			cout << "task characteristics: period" << (*tasks)[id].period
					<< " computations time" << (*tasks)[id].computation_time
					<< endl;
			exit(1);
		}
	}

	int hyperperiod = compute_lcm(tasks, (*tasks)[0].taskset);

	cout << "verify successful" << endl;

}

void verify(vector<float_schedule>*sch, vector<float_task>*tasks) {
	float computations[tasks->size()];
	for (unsigned int i = 0; i < tasks->size(); i++) {
		computations[i] = 0;
	}

	for (unsigned int i = 0; i < sch->size(); i++) {
		int id = (*sch)[i].task_id;
		computations[id] = computations[id] + (*sch)[i].end - (*sch)[i].start;
		if (floor((*sch)[i].start / (*tasks)[id].period)
				* (*tasks)[id].computation_time > computations[id]+0.0001) {
			cout << "violation occured at " << i << endl;
			for (unsigned int j = 0; j <= i; j++) {
				cout << j << "task " << (*sch)[j].task_id << " start:"
						<< (*sch)[j].start << " end:" << (*sch)[j].end << endl;
			}

			cout << "possible deadline miss" << endl;
			cout << "start:" << (*sch)[i].start << " task id"
					<< (*sch)[i].task_id << " required computations: "
					<< floor((*sch)[i].start / (*tasks)[id].period)
							* (*tasks)[id].computation_time
					<< " done computations" << computations[id] << endl;
			cout << "task characteristics: period" << (*tasks)[id].period
					<< " computations time" << (*tasks)[id].computation_time
					<< endl;

			cout<<"total computations \n"<<endl;
			for(unsigned int i=0;i<tasks->size();i++)
			{
				cout<<"task "<<i<<" computations "<<computations[i]<<endl;
			}
			for(unsigned int i=0;i<tasks->size();i++)
			{
				cout<<"task "<<i<<" computation time "<<(*tasks)[i].computation_time<<" period "<<(*tasks)[i].period<<endl;
			}

			vector<float> times;
			imp_times(tasks, &times);
			cout<<"printing times "<<endl;
			for(unsigned int i=0;i<times.size();i++)
			{
				cout<<i<<"\t"<<times[i]<<endl;
			}


			exit(1);
		}
	}

	cout << "verify successful" << endl;

	float hyperperiod=interval_req[interval_size-1][2];
	cout<<"Hyperperiod is "<<hyperperiod<<endl;
	for(unsigned int i=0;i<tasks->size();i++)
	{
		cout<<"task "<<i<<" computations "<<computations[i]<<" required computations "<<floor(hyperperiod/(*tasks)[i].period)*(*tasks)[i].computation_time <<endl;
	}

}



void verify_s(vector<schedule>*sch, vector<schedule>*edf, vector<task>*tasks) {
	int computations[tasks->size()];
	for (unsigned int i = 0; i < tasks->size(); i++) {
		computations[i] = 0;
	}

	for (unsigned int i = 0; i < sch->size(); i++) {
		int id = (*sch)[i].task_id;
		computations[id] = computations[id] + (*sch)[i].end - (*sch)[i].start;
		if ((*sch)[i].start / (*tasks)[id].period
				* (*tasks)[id].computation_time > computations[id]) {
			cout << "violation occured at " << i << endl;

			cout << "printing edf schedule" << endl;
			for (unsigned int j = 0; j < edf->size(); j++) {
				cout << "task " << (*edf)[j].task_id << " start:"
						<< (*edf)[j].start << " end:" << (*edf)[j].end << endl;
			}

			cout << "generating new edl" << endl;

			schedule temp;
			for (int j = edf->size() - 1; j >= 0; j--) {
				temp.start = tasksets[0].hyperperiod - (*edf)[j].end;
				temp.end = tasksets[0].hyperperiod - (*edf)[j].start;
				temp.task_id = (*edf)[j].task_id;
				cout << "task " << temp.task_id << " start:" << temp.start
						<< " end:" << temp.end << endl;
			}

			cout << "task characteristics" << endl;

			for (unsigned int j = 0; j < tasks->size(); j++) {
				cout << "task " << j << " computation time:"
						<< (*tasks)[j].computation_time << " period:"
						<< (*tasks)[j].period << endl;
			}

			cout << "now printing edl that violated" << endl;

			for (unsigned int j = 0; j <= i; j++) {
				cout << j << "task " << (*sch)[j].task_id << " start:"
						<< (*sch)[j].start << " end:" << (*sch)[j].end << endl;
			}

			cout << "possible deadline miss" << endl;
			cout << "start:" << (*sch)[i].start << " task id"
					<< (*sch)[i].task_id << " required computations: "
					<< (*sch)[i].start / (*tasks)[id].period
							* (*tasks)[id].computation_time
					<< " done computations" << computations[id] << endl;
			cout << "task characteristics: period" << (*tasks)[id].period
					<< " computations time" << (*tasks)[id].computation_time
					<< endl;
			exit(1);
		}
	}

#if(ENABLE_PRINTS)
	int hyperperiod=compute_lcm(tasks,(*tasks)[0].taskset);
	for(unsigned int i=0;i<tasks->size();i++)
	{
		cout<<"task "<<i <<" done computations: "<<computations[i]<<" required computations: "<<hyperperiod/(*tasks)[i].period*(*tasks)[i].computation_time<<endl;
	}

	cout<<"verify successful"<<endl;
#endif

}

//verification where each instance has a different speed

void verify(vector<schedule>*sch, vector<task>*tasks, vector<double>*speed) {
	assert(speed->size()==sch->size());
	int computations[tasks->size()];
	for (unsigned int i = 0; i < tasks->size(); i++) {
		computations[i] = 0;
	}

	for (unsigned int i = 0; i < sch->size(); i++) {
		int id = (*sch)[i].task_id;
		computations[id] = round(
				((double) computations[id])
						+ ((double) ((*sch)[i].end - (*sch)[i].start))
								* (*speed)[i]);

		if ((*sch)[i].start / (*tasks)[id].period
				* (*tasks)[id].computation_time > computations[id]) {
			cout << "violation occured at " << i << endl;
			for (unsigned int j = 0; j <= i; j++) {
				cout << j << "task " << (*sch)[j].task_id << " start:"
						<< (*sch)[j].start << " end:" << (*sch)[j].end << endl;
			}

			cout << "possible deadline miss" << endl;
			cout << "start:" << (*sch)[i].start << " end " << (*sch)[i].end
					<< " task id " << (*sch)[i].task_id << " speed "
					<< (*speed)[i] << " required computations: "
					<< (*sch)[i].start / (*tasks)[id].period
							* (*tasks)[id].computation_time
					<< " done computations" << computations[id] << endl;
			cout << "task characteristics: period" << (*tasks)[id].period
					<< " computations time" << (*tasks)[id].computation_time
					<< endl;
			exit(1);
		}
	}

	int hyperperiod = compute_lcm(tasks, (*tasks)[0].taskset);
	//   #if(ENABLE_PRINTS)

	for (unsigned int i = 0; i < tasks->size(); i++) {
		cout << "task " << i << " done computations: " << computations[i]
				<< " required computations: "
				<< hyperperiod / (*tasks)[i].period
						* (*tasks)[i].computation_time << endl;
	}
	cout << "verify successful" << endl;
	//   #endif

}

void speed_scale_discrete(vector<task>*scaled_tasks, vector<float>*speeds,
		vector<task>*tasks) {
	float g_power = global_power(tasks, 0, tasks->size(), 1.0);
	global_sort_power = g_power;
	for (unsigned int i = 0; i < tasks->size(); i++) {
		scaled_tasks->push_back((*tasks)[i]);
	}
	sort(scaled_tasks->begin(), scaled_tasks->end(), sort_power);
#if(ENABLE_PRINTS)

	for(unsigned int i=0;i<scaled_tasks->size();i++)
	{
		cout<<i<<"|"<<(*scaled_tasks)[i].power<<endl;
	}
#endif
	float total_slack = 1 - tasksets[0].c_util;

	float util = 1.0;
	for (unsigned int i = 0; i < scaled_tasks->size(); i++) {
		g_power = global_power(tasks, i, tasks->size(), util);
#if(ENABLE_PRINTS)

		cout<<"iteration"<<i<<" global_power"<<g_power<<" utilizqation "<<util<<endl;
#endif
		float deviation = 10000.00;
		int speed_index = 0;
#if(ENABLE_PRINTS)

		cout<<"task "<<i<<" computation "<<(*scaled_tasks)[i].computation_time<< " period"<<(*scaled_tasks)[i].period<<" power"<<(*scaled_tasks)[i].power<<endl;
#endif
		for (unsigned int j = 0; j < speeds->size(); j++) {
			float target_power = (*scaled_tasks)[i].power
					* pow((*speeds)[j], 3);
			float corrected_cutil = (ceil(
					((float) ((float) (*scaled_tasks)[i].computation_time)
							/ (*speeds)[j])
							- (*scaled_tasks)[i].computation_time))
					/ ((float) (*scaled_tasks)[i].period);
			if (fabs(target_power - g_power) < deviation
					&& corrected_cutil < total_slack) {
				deviation = fabs(target_power - g_power);
				speed_index = j;
			}
		}

		(*scaled_tasks)[i].power = (*scaled_tasks)[i].power
				* pow((*speeds)[speed_index], 3);
		float corrected_cutil = (ceil(
				((float) ((float) (*scaled_tasks)[i].computation_time)
						/ (*speeds)[speed_index])
						- (*scaled_tasks)[i].computation_time))
				/ ((float) (*scaled_tasks)[i].period);
		(*scaled_tasks)[i].computation_time = ceil(
				((float) (*scaled_tasks)[i].computation_time)
						/ (*speeds)[speed_index]);
#if(ENABLE_PRINTS)

		cout<<"optimized task "<<i<<" computation "<<(*scaled_tasks)[i].computation_time<< " period"<<(*scaled_tasks)[i].period<<" power"<<(*scaled_tasks)[i].power<<endl;
#endif
		total_slack = total_slack - corrected_cutil;
		util = util
				- ((float) (*scaled_tasks)[i].computation_time)
						/ ((float) (*scaled_tasks)[i].period);
	}
}

bool sort_sch(long_schedule a,long_schedule b)
{
	if(a.start!=b.start)
	{
		return(a.start<b.start);
	}
	else
	{
		return (a.core<b.core);
	}
}




