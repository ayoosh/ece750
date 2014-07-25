/*
 * exec_interval.cpp
 *
 *  Created on: Mar 10, 2014
 *      Author: rehan
 */

#include "scheduler.h"


interval::interval(){};

interval::interval(double st,double en)
{
	start=st;
	end=en;
}
interval::~interval(){
	exec.clear();
};

void interval::add_exec(execution newexec)
{
	exec.push_back(newexec);
}
long interval::get_length()
{
	return (end-start);
}

void interval::set_interval(long st,long en)
{
	start=st;
	end=en;
}

int interval::get_size()
{
	return exec.size();
}

execution interval::get_element(int index)
{
	if(index>=exec.size())
	{
		cout<<"index out of bounds"<<endl;
		exit(1);
	}
	return (exec[index]);
}

void interval::clear()
{
	exec.clear();
}







