/*
 * config.h
 *
 *  Created on: Jun 16, 2013
 *      Author: rehan
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#define S_FACTOR 3
#define R 0.36
#define C 0.8
#define DEL 0.001
#define RAU 0.1
#define AMBIENT 40.0
#define THRESHOLD 75.00
//#define GRANULARITY 10000.00//100000.00 //100 us
#define GRANULARITY 10.00//100000.00 //100 us
#define RT_GRANULARITY 10000.00 //10ms should be equal to GRANULARITY. Kept different to allow proper scheduling and profiling
#define MULT_FACTOR 10//10//1000 change to 10 later
#define PROFILE_GRANULARITY 100
#define BUFSIZE	2048

#define MIN_PERIOD 10.00

#define SLACK_ENABLE 0
#define MATLAB_ENABLE 0
#define STATIC_ENABLE 0
#define MAX_ENABLE 1
#define NOCONS_ENABLE 0
#define OPT_ENABLE 0
#define INST_ENABLE 0
#define RT_ENABLE 0

#define ENABLE_PRINTS 0
#define INST_MF_PRINT 0 // print enable for instance maxfirst
#define CORRECTION_FACTOR  1.0
#define SPEED_DEBUG 0

#define PRINT_TASKSET 0
#define MAX_TASKS	20

#define MIN_SPEED  0.625
#define MAX_SPEED  1.00
#define MARGIN     0.3   //temperature margin for interval scheduling

#define W_INT		0.010000000  //worst case fair weighted fair queuing interval length
#define W_INT_HIGH  0.2

//rt****************************************
#define FINISHED 0	//task waiting for next instance
#define EXECUTING 1	//task presently executing
#define IDLE	2	//task forced to idle
#define READY	3	// task ready to start execution
#define DEBUG 0
#define TIMING 1

#define BENCH_RUN 0

#define SLACK_SCHEME_TEST 0

#define TEMP_EXPERIMENT	0
#define TEMP_TASK	0
#define TEMP_CORE	3
#define	TEMP_TIME	300000
#define IDLE_TIME	60000


#define	CORE	3

#define MAX_PERIOD  (10000)
#define MY_PRIORITY (49) /* we use 49 as the PRREMPT_RT use 50
                            as the priority of kernel tasklets
                            and interrupt handler by default */
#define MAX_SAFE_STACK (8*1024) /* The maximum stack size which is
                                   guaranteed safe to access without
                                   faulting */
#define NSEC_PER_SEC    (1000000000) /* The number of nsecs per sec. */
//#define THRESHOLD	100
#define NUM_PROCESSORS 4

#define SCHED_SCHEME	1	//1 = EDF
//2 = Temperature minimization
//3 = Temperature minimization, EDF
#define MAX_RUNTIME	60000
//#define GRANULARITY 1

#define ACTUAL_RUN 1

#define MAX_TASKS 20

#define UTIL_POWER 114.4912

#define S_L 16

//#define MAX_POWER 250//100.00
//#define MIN_POWER 30//10.00
#define MAX_POWER 100.00//100.00
#define MIN_POWER 10//10.00



#define F_COMPARE 0.000001

#define MAX_INSTANCES 10000

#define BIN_LENGTH 7

#endif /* CONFIG_H_ */
