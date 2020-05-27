CXX = g++
CXXFLAGS = -g -Wall -std=c++11

## -fsanitize=address 
##test:json_generator.h json_generator.cpp main.cpp 
##	$(CXX) $(CXXFLAGS) json_generator.h json_generator.cpp main.cpp -o main

test:test.cpp tinyjson.cpp 
	$(CXX) $(CXXFLAGS) test.cpp tinyjson.cpp -o test


clean:
	rm -f *.o main test