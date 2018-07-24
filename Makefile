all: numa_c numa_cpp

LIBS:= -lnuma
CFLAGS:= -Wall
CXXFLAGS:=$(CFLAGS) -std=c++14

numa_c:
	gcc -o numa numa.c $(LIBS) $(CFLAGS)

numa_cpp:
	g++ -o numa_cpp numa.cpp  $(LIBS) $(CXXFLAGS)

clean:
	@rm -f numa_cpp
	@rm -f numa
