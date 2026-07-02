CXX = g++
CXXFLAGS = -std=c++17 -O3 -march=native -Wall -pthread -Iinclude

all: engine run_benchmarks

engine: src/main.cpp src/orderbook.cpp
	$(CXX) $(CXXFLAGS) -o engine src/main.cpp src/orderbook.cpp

run_benchmarks: src/benchmark.cpp src/orderbook.cpp
	$(CXX) $(CXXFLAGS) -o run_benchmarks src/benchmark.cpp src/orderbook.cpp

clean:
	del /f engine.exe run_benchmarks.exe src\*.o 2>nul || rm -f engine run_benchmarks src/*.o
