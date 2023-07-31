CC = gcc
CXX = g++

CFLAGS= --std=c99 -Wall -Wextra -Werror -pedantic -Wconversion
CXXFLAGS = --std=c++11 -Wall -Wextra -Werror -pedantic -Wconversion

LIB=lib
EXAMPLES=egs

all: library examples

library:
	@echo "Building libperf library..."
	@mkdir -p $(LIB)
	$(CC) -c libperf.c -o $(LIB)/libperf_c.o -g
	$(CXX) -c libperf.cpp -o $(LIB)/libperf_cxx.o -g
	ar rcs $(LIB)/libperf.a $(LIB)/libperf_c.o $(LIB)/libperf_cxx.o

examples: lib
	@echo "Building libperf examples..."
	$(CC) -g -I . $(EXAMPLES)/example.c -o $(EXAMPLES)/c_example $(LIB)/libperf.a
	$(CXX) -g -I . $(EXAMPLES)/example.cpp -o $(EXAMPLES)/cxx_example $(LIB)/libperf.a

clean:
	@echo "Deleting all builds..."
	@rm $(LIB)/* $(EXAMPLES)/c_example $(EXAMPLES)/cxx_example &> /dev/null || true
