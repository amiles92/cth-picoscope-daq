#!/bin/bash

INC=$(shell pybind11-config --includes) -I$(shell pwd)/include -I/opt/picoscope/include
LIB=$(shell python3-config --ldflags) -L/opt/picoscope/lib
FLAGS=-fPIC -shared
SRC=$(shell pwd)/src

ROOTINC=-I$(shell root-config --incdir)
ROOTLIB=$(shell root-config --libs)
ROOTFLAGS=$(shell root-config --cflags)

ANALYSISINC=$(ROOTINC) -I$(shell pwd)/include/analysis
ANALYSISLIB=$(ROOTLIB)
ANALYSISFLAGS=-Wall $(ROOTFLAGS)

default: main

main:
	g++ $(FLAGS) $(INC) $(LIB) $(SRC)/libps6000/daq6000.cpp $(SRC)/libps6000/ps6000Wrapper.cpp -lps6000 -o daq6000$(shell python3-config --extension-suffix)
	g++ $(FLAGS) $(INC) $(LIB) $(SRC)/libps6000a/daq6000a.cpp $(SRC)/libps6000a/ps6000aWrapper.cpp -lps6000a -o daq6000a$(shell python3-config --extension-suffix)
	### LEAVE SOURCE FILE AT THE BEGINNING OTHERWISE IT DOES NOT WORK !!!
	g++ $(SRC)/analysis/*.cpp $(ANALYSISFLAGS) $(ANALYSISINC) $(ANALYSISLIB) -o exec/analysis

clean:
	rm -r *.o make.out

