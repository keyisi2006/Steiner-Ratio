all: plot binsearch split_rho

plot: plot.cpp
	g++ plot.cpp -O3 -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -std=c++23 -ffast-math -o plot

binsearch: binsearch.cpp
	g++ binsearch.cpp -O3 -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -std=c++23 -ffast-math -o binsearch

bbb: bbb.cpp
	g++ -O3 -Igeosteiner-5.3/lp_solve_2.3 -Wall -o bbb bbb.cpp -std=c++20 geosteiner-5.3/memory.o -Lgeosteiner-5.3 -lgeosteiner geosteiner-5.3/lp_solve_2.3/libLPS.a -lgmp -lm

split_rho: split_rho.cpp
	g++ -O3 -Igeosteiner-5.3/lp_solve_2.3 -Wall -o split_rho split_rho.cpp -std=c++23 geosteiner-5.3/memory.o -Lgeosteiner-5.3 -lgeosteiner geosteiner-5.3/lp_solve_2.3/libLPS.a -lgmp -lm

clean:
	find ./formulas -maxdepth 1 -name 'F[0-9]*' ! -name 'F0' -delete
	rm -f plot binsearch split_rho bottleneck.txt rho.txt plot_gen_* log_*
	rm -r evolve_resp*
	
