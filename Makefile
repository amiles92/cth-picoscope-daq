#!/bin/bash

INC=$(shell pybind11-config --includes) -I$(shell pwd)/include -I/opt/picoscope/include
LIB=$(shell python3-config --ldflags) -L/opt/picoscope/lib
FLAGS=-fPIC -shared
SRC=$(shell pwd)/src
SUF=$(shell python3-config --extension-suffix)

ROOTINC=-I$(shell root-config --incdir)
ROOTLIB=$(shell root-config --libs --glibs)
ROOTFLAGS=$(shell root-config --cflags)

ANALYSISINC=$(ROOTINC) -I$(shell pwd)/include/analysis
ANALYSISLIB=$(ROOTLIB)
ANALYSISFLAGS=-Wall $(ROOTFLAGS)

default: main

main: daq analysis

daq: 6k 6ka 3ka

3ka: 
	g++ $(FLAGS) $(INC) $(LIB) $(SRC)/ps3000a/daq3000a.cpp $(SRC)/ps3000a/ps3000aWrapper.cpp -lps3000a -o daq3000a$(SUF)

6k:
	g++ $(FLAGS) $(INC) $(LIB) $(SRC)/ps6000/daq6000.cpp $(SRC)/ps6000/ps6000Wrapper.cpp -lps6000 -o daq6000$(SUF)

6ka:
	g++ $(FLAGS) $(INC) $(LIB) $(SRC)/ps6000a/daq6000a.cpp $(SRC)/ps6000a/ps6000aWrapper.cpp -lps6000a -o daq6000a$(SUF)

analysis:
	### LEAVE SOURCE FILE AT THE BEGINNING OTHERWISE IT DOES NOT WORK !!!
	g++ $(SRC)/analysis/*.cpp $(ANALYSISFLAGS) $(ANALYSISINC) $(ANALYSISLIB) -o exec/analysis

clean:
	rm -f *$(SUF)
	rm exec/analysis

