CFLAGS= -g -Wall

all: edf
#all: edf static ccedf

edf: edf.o io.o
#static: static.o io.o
#ccedf: ccedf.o io.o

io.o: io.c proj3.h
	${CC} ${CFLAGS} -c io.c

edf.o: edf.c proj3.h
	${CC} ${CFLAGS} -c edf.c

#static.o: static.c proj3.h
#	${CC} ${CFLAGS} -c static.c

#ccedf.o: ccedf.c proj3.h
#	${CC} ${CFLAGS} -c ccedf.c
