#!/bin/sh

filename = "dat".i.".dat"
plotfile = "plot".i.".png"

set output plotfile
plot filename
set output

i=i+1
if(i <= n) reread
