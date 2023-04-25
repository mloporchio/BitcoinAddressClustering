#
#	File:	makefile
#	Author: Matteo Loporchio
#

CXX=g++
CXX_FLAGS=-O3 --std=c++11 -I ~/lemon/include -L ~/lemon/lib -lemon

.PHONY: clean

builder: builder.cpp
	$(CXX) $(CXX_FLAGS) $^ -o $@

analyzer: analyzer.cpp
	$(CXX) $(CXX_FLAGS) $^ -o $@

all: builder analyzer

clean:
	rm -f builder analyzer