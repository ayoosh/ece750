# -*- coding: utf-8 -*-
"""
Created on Thu Jul 24 11:47:42 2014

@author: rehan
"""
import fractions
import numpy as np
import math
import random
import collections


R=0.36
C=0.8
DEL=0.001
RAU=0.1
AMBIENT=40.0

BETA=1 / (R * C) - (DEL / C); #ZETA=1/BETA
ZETA=1/BETA
THRESHOLD=75
GRAN=0.1

periodic_task=collections.namedtuple('pariodic_tasks',['name','index','ctime','period','power'])
aperiodic_task=collections.namedtuple('aperiodic_tasks',['name','index','ctime','weight','power'])

def read_periodic_taskset(fname):
    f1=open(fname)
    pset=[]
    for i,line in enumerate(f1):
        if i>0:
            words=line.strip().split()
            assert len(words)==4
            pset.append(periodic_task(words[0],i-1,float(words[1]), float(words[2]),float(words[3])))
    f1.close()            
    return pset  

def read_aperiodic_tasks(fname):
    f1=open(fname)
    aperiodics=[]
    for i,line in enumerate(f1):
        if i>0:
            words=line.strip().split()
            assert len(words)==4
            aperiodics.append(aperiodic_task(words[0],i-1,float(words[1]),float(words[2]),float(words[3])))
    f1.close()
    return aperiodics  
    
    
def my_mean(aperiodics, stime):
    weights=[x.weight for x in aperiodics]
    total=sum(weights)
    mean=0.00
    for i, x in enumerate(aperiodics):
        mean=mean+stime[i]*float(x.weight)/float(total)
    return mean
        
def my_variance(aperiodics, stime):
    weights=[x.weight for x in aperiodics]
    total=sum(weights)
    mean=my_mean(aperiodics,stime)
    variance=0    
    for i,x in enumerate(aperiodics):
        variance=variance+float(x.weight)/float(total) * (stime[i]-mean)**2
    return variance

def mean_response(util,rate,var):
    response=(util+ (util**2 + rate**2 * var)/(2*(1-util)))/rate
    return response
    
        

def weighted_random(aperiodics,instances):
    task_type=[]
    weights=[x.weight for x in aperiodics]
    total=sum(weights)
    for i in range(instances):        
        number = random.random() * total
        for x in aperiodics:
            if number < x.weight:
                break
            number -= x.weight
        task_type.append(x.index)
    return task_type


def corrected_temperature(temperature):
    return (temperature * C - (C * (R * RAU + AMBIENT)) / (1 - R * DEL))


CORRECTED_THRESHOLD=corrected_temperature(THRESHOLD)



def real_temperature(corrected_temperature):
    return ((corrected_temperature + (C * (R * RAU + AMBIENT)) / (1 - R * DEL))/C)

def heat(initial, start, end, power):
    temperature=initial*math.exp(-1*BETA*(end-start)*GRAN) + power/BETA *(1-math.exp(-1*BETA*(end-start)*GRAN))
    return temperature
    
def thermal_profile(pprofile, initial): #power profile format (start,end,power)
    tempp=[]
    initial=corrected_temperature(initial)
    start=0
            
    tempp.append((0,initial))    

    for x in pprofile:
        if start==0:
            tempp.append((x[1],heat(initial,x[0],x[1],x[2])))
            start=x[1]
        else: 
            if start<x[0]:
                tempp.append((x[0],heat(tempp[len(tempp)-1][1],start,x[0],0.00)))
                start=x[0]
            tempp.append((x[1],heat(tempp[len(tempp)-1][1],x[0],x[1],x[2])))
            start=x[1]
            
    tempp=[list(x) for x in tempp]
    for i in tempp:
        i[1]=real_temperature(i[1])
    return tempp
    
            
def tutil(tasks):
    for i in tasks:
        print i[0]*i[2]/(i[1]*BETA*corrected_temperature(75.0))

    
              
