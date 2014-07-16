#!/bin/sh

filename = "dat".i.".dat"
plotfile = "plot".i.".png"

set output plotfile
plot filename title "Average Response Time" lt -1
set output

i=i+1
if(i <= n) reread
