set title "Cross-correlations versus time" font "Gill Sans,9"
set ylabel "Bin boundary" font "Gill Sans,9"
set xlabel "Timestep" font "Gill Sans,9"
set yrange [-0.1: 0.1]

plot "adap.log" u 1 w lines t "bound_0", \
     "adap.log" u 2 w lines t "bound_1", \
     "adap.log" u 3 w lines t "bound_2"
