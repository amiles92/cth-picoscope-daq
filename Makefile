#!/bin/bash

INC=$(shell pybind11-config --includes) -I$(shell pwd)/include -I/opt/picoscope/include
LIB=$(shell python3-config --ldflags) -L/opt/picoscope/lib
FLAGS=-fPIC -shared
SRC=$(shell pwd)/src

default: main

main:
	g++ $(FLAGS) $(INC) $(LIB) $(SRC)/daq.cpp $(SRC)/ps6000Wrapper.cpp -lps6000 -lpython3.10 -o daq$(shell python3-config --extension-suffix)

clean:
	rm -r *.o make.out

