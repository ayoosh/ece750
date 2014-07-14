#!/usr/bin/gnuplot
reset
set terminal png

set xlabel "Inter-arrival time"
set ylabel "Average response time"
set title "Average Response Time"

set grid
set style data linespoints

i=1
n=5
load "loop.sh"

#plot "dat1.dat" using 1:2 title "Example" 
