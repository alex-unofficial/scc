set term eps

set autoscale
unset log
unset label
set xtic auto
set ytic auto
unset title

set grid

set ylabel "time (sec)"

set format x "%0.1tx10^%T"
set format y "%0.3f"

set key left top

set output "plots/vertices.eps"
set xlabel "Number of vertices"
plot "data/serial.dat"   using 2:5 with linespoints title "serial"   lw 3 pt 6 lt rgb 'black', \
     "data/pthread.dat"  using 2:5 with linespoints title "pthread"  lw 3 pt 6 lt rgb 'green', \
     "data/opencilk.dat" using 2:5 with linespoints title "opencilk" lw 3 pt 6 lt rgb 'orange', \
     "data/openmp.dat"   using 2:5 with linespoints title "openmp"   lw 3 pt 6 lt rgb 'blue'

set output "plots/edges.eps"
set xlabel "Number of edges"
plot "data/serial.dat"   using 3:5 with linespoints title "serial"   lw 3 pt 6 lt rgb 'black', \
     "data/pthread.dat"  using 3:5 with linespoints title "pthread"  lw 3 pt 6 lt rgb 'green', \
     "data/opencilk.dat" using 3:5 with linespoints title "opencilk" lw 3 pt 6 lt rgb 'orange', \
     "data/openmp.dat"   using 3:5 with linespoints title "openmp"   lw 3 pt 6 lt rgb 'blue'

set output "plots/threads.eps"
set xlabel "Number of threads"
set key right top
unset format x
plot "data/pthread-threads.dat"  using 2:3 with linespoints title "pthread"  lw 3 pt 6 lt rgb 'green', \
     "data/opencilk-threads.dat" using 2:3 with linespoints title "opencilk" lw 3 pt 6 lt rgb 'orange', \
     "data/openmp-threads.dat"   using 2:3 with linespoints title "openmp"   lw 3 pt 6 lt rgb 'blue'
