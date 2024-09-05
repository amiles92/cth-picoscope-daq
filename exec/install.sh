#!/bin/bash

cd ..
# if [ ! -d "plots/" ]; then
# 	mkdir plots/
# fi
# if [ ! -d "plots/analyse" ]; then
# 	mkdir plots/analyse
# fi
if [ ! -d "pre-analyse" ]; then
	mkdir pre-analyse
fi
if [ ! -d "pre-analyse/root-files" ]; then
	mkdir pre-analyse/root-files
fi
if [ ! -d "pre-analyse/plots" ]; then
	mkdir pre-analyse/plots
fi
make analysis
cd exec
