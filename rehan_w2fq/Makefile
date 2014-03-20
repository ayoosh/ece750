#CFLAGS= -g -Wall
CC = gcc
CPP= g++
#BASE_OPTIONS     = -ansi -pedantic-errors -Wall -Wextra -Wno-long-long -std=c++0x -Wvla
BASE_OPTIONS= -ansi -Wall -Wextra -Wno-long-long -std=c++0x -Wvla
LINKER_OPT       = -lstdc++ -lm


all: edf

edf: scheduler.o intervals_vars.o util.o multi.o w2fq.o exec_interval.o dynamic_interval_speed.o hotspot_util.o base_sched.o
	 ${CPP} ${BASE_OPTIONS} -o edf scheduler.o intervals_vars.o util.o multi.o w2fq.o exec_interval.o dynamic_interval_speed.o hotspot_util.o base_sched.o

scheduler.o: scheduler.cpp scheduler.h
	${CPP} ${BASE_OPTIONS} -c scheduler.cpp

intervals_vars.o: intervals_vars.c
	${CC} -c intervals_vars.c 

util.o: util.cpp scheduler.h
	${CPP} ${BASE_OPTIONS} -c util.cpp 

w2fq.o: w2fq.cpp
	${CPP} ${BASE_OPTIONS} -c w2fq.cpp 

multi.o: multi.cpp
	${CPP} ${BASE_OPTIONS} -c multi.cpp 

exec_interval.o: exec_interval.cpp
	${CPP} ${BASE_OPTIONS} -c exec_interval.cpp

dynamic_interval_speed.o: dynamic_interval_speed.cpp
	${CPP} ${BASE_OPTIONS} -c dynamic_interval_speed.cpp

hotspot_util.o: hotspot_util.cpp
	${CPP} ${BASE_OPTIONS} -c hotspot_util.cpp

base_sched.o: base_sched.cpp
	${CPP} ${BASE_OPTIONS} -c base_sched.cpp
clean:
	rm -f *.o multi
