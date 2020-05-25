CXX = g++
CXXFLAGS = -g -Wall -std=c++11

test:json_generator.h json_generator.cpp main.cpp 
	$(CXX) $(CXXFLAGS) json_generator.h json_generator.cpp main.cpp -o main

clean:
	rm -f *.o main