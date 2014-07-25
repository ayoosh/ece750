# -*- coding: utf-8 -*-
"""
Created on Thu Jul 24 11:42:57 2014

@author: rehan
"""

import lib
import numpy as np
import sys

arguments=sys.argv

periodic_file="periodic_taskset"
aperiodic_file="aperiodic_tasks"
mean_arrival=10.0
number_of_aperiodics=1000000

if(len(arguments)!=5):
    print "Argument format <periodic_file><aperiodic_file><mean_arrival><number of aperiodics>"
    print "Selecting default arguments<\"periodic_taskset\"><\"aperiodic_tasks\"><\""+str(mean_arrival)+"\">"+"<\""+str(number_of_aperiodics)+"\">"
else:
    periodic_file=arguments[1]
    aperiodic_file=arguments[2]
    mean_arrival=float(arguments[3])
    number_of_aperiodics=int(arguments[4])
    
periodics=lib.read_periodic_taskset("periodic_taskset")
aperiodics=lib.read_aperiodic_tasks("aperiodic_tasks")

mean_arrival=float(mean_arrival)


periodic_power=0
periodic_cutil=0
periodic_tutil=0

aperiodic_cutil=0
aperiodic_tutil=0

service_time=[]

response_time=[]
arrival_times=[]
empty_count=0
waiting_time=[]
power_profile=[]
thermal_profile=[]

# lists kept for possible WF2Q implementation in the future

periodic_rate=[] 
aperiodic_rate=[]

# *********************************************************

for x in periodics:
    periodic_power=periodic_power+x.ctime/x.period*x.power
    periodic_cutil=periodic_cutil+x.ctime/x.period
    periodic_tutil=periodic_tutil+x.power*x.ctime*lib.ZETA/(x.period*lib.CORRECTED_THRESHOLD)
    periodic_rate.append(x.ctime/x.period)    
    
aperiodic_cutil=1-periodic_cutil
aperiodic_tutil=1-periodic_tutil


aperiodic_list = lib.weighted_random(aperiodics,number_of_aperiodics)
intervals=np.random.exponential(mean_arrival,number_of_aperiodics)


for i in range(len(intervals)):
    if i==0:
        arrival_times.append(intervals[i])
    else:        
        arrival_times.append(arrival_times[i-1]+intervals[i])

for x in aperiodics:
    service_time.append(max(x.ctime/aperiodic_cutil, lib.ZETA*x.ctime*x.power/(lib.CORRECTED_THRESHOLD*aperiodic_tutil)))

set_mean=lib.my_mean(aperiodics,service_time)
set_variance=lib.my_variance(aperiodics,service_time)

utilization=set_mean/mean_arrival
assert(utilization<1)

start=0

for i,arrival in enumerate(arrival_times):
    if start<arrival:
        power_profile.append((start,arrival,periodic_power))
        empty_count=empty_count+1
        start=arrival
        
    current_aperiodic=aperiodics[aperiodic_list[i]]    
    finish_time=start+service_time[aperiodic_list[i]]
    response_time.append(finish_time-arrival)
    waiting_time.append(finish_time-arrival-service_time[aperiodic_list[i]])
    power_profile.append((start,finish_time,periodic_power + current_aperiodic.power*current_aperiodic.ctime/service_time[aperiodic_list[i]]))
    aperiodic_rate.append((start,finish_time,current_aperiodic.ctime/(finish_time-start)))
    start=finish_time

temperature_profile=lib.thermal_profile(power_profile,lib.real_temperature(periodic_power*lib.ZETA))
    
mean_rt=np.mean(response_time)
mean_rt_a=lib.mean_response(set_mean/mean_arrival,1/mean_arrival,set_variance)
print mean_rt, mean_rt_a

results=open("results",'a')
results.write(str(mean_arrival)+"\t"+str(utilization)+"\t"+str(number_of_aperiodics)+"\t"+str(mean_rt)+"\t"+str(mean_rt_a)+"\t"+str(max([x[1] for x in temperature_profile]))+"\t"+str(empty_count)+"\n")
results.close()