#
#	File:	makefile
#	Author:	Matteo Loporchio
#

CXX=g++
CXX_FLAGS=-O3 --std=c++11 -I ~/igraph/include/igraph
LD_FLAGS=-L ~/igraph/lib -ligraph -mmacosx-version-min=11.7

.PHONY: clean

%.o: %.cpp
	$(CXX) $(CXX_FLAGS) -c $^ 

builder: builder.o
	$(CXX) $(CXX_FLAGS) $^ -o $@

clustering: clustering.o
	$(CXX) $(CXX_FLAGS) $^ -o $@ $(LD_FLAGS)

all: builder clustering

clean:
	rm -f *.o builder clustering