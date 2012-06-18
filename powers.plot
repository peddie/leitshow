set title "Channel powers versus time" font "Gill Sans,9"
set ylabel "Power" font "Gill Sans,9"
set xlabel "Timestep" font "Gill Sans,9"

plot "adap.log" u 7 w lines t "P_0", \
     "adap.log" u 8 w lines t "P_1", \
     "adap.log" u 9 w lines t "P_2", \
     "adap.log" u 10 w lines t "P_3"
