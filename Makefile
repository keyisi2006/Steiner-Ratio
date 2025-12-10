plot: plot.cpp
	g++ plot.cpp -O3 -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -std=c++23 -ffast-math -o plot

split_rho: split_rho.cpp
	g++ split_rho.cpp $(wildcard geosteiner-5.3/.libs/*.o) $(wildcard geosteiner-5.3/lp_solve_2.3/*.o) -std=c++23 -O2 -Wall -o split_rho
