
set title "offset measurements for wired rpi3 server and wireless rpi3 client"
set xlabel "runtime in seconds"
set ylabel "offset in microseconds"
set y2label "sample size"
set ytics -100,20
set grid ytics
set yrange [-100:100]
set y2range [0:2500]
set y2tics 0,1000
set linestyle 2 lt 2 lw 1 pt 8

DATAFILE=ARG1
set term qt size 900, 400

# the column index "time:value" pairs probably need some adjusting to whatever todays format happen to be
plot DATAFILE using 5:19 w lp t 'offset usec' axes x1y1, DATAFILE using 5:15 with imp ls 2 title 'sample size' axes x1y2
pause mouse button1,keypress

set term png size 900, 400
set output "plot.png"
replot
