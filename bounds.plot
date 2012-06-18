set title "Bin boundaries versus time" font "Gill Sans,9"
set ylabel "FFT index" font "Gill Sans,9"
set xlabel "Timestep" font "Gill Sans,9"

plot "adap.log" u 4 w lines t "bound_0", \
     "adap.log" u 5 w lines t "bound_1", \
     "adap.log" u 6 w lines t "bound_2"


