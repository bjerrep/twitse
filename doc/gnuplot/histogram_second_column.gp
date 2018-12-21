# based on https://stackoverflow.com/a/7454274

DATAFILE=ARG1

set xrange [0:1000000]

n=50 #number of intervals
max=1000000 #max value
min=0 #min value
width=(max-min)/n #interval width
#function used to map a value to the intervals
hist(x,width)=width*floor(x/width)+width/2.0
set boxwidth width*0.9
set style fill solid 0.5 # fill style

#count and plot
plot DATAFILE u (hist($2,width)):(1.0) smooth freq w boxes lc rgb"grey" notitle
pause mouse button1,keypress
