#!/bin/bash

cd ..
if [ ! -d "plots/" ]; then
	mkdir plots/
fi
if [ ! -d "plots/pre-analyse" ]; then
	mkdir plots/pre-analyse
fi
if [ ! -d "plots/analyse" ]; then
	mkdir plots/analyse
fi
if [ ! -d "pre-analyse" ]; then
	mkdir pre-analyse
fi
if [ ! -d "pre-analyse/pre-analysed_root_files" ]; then
	mkdir pre-analyse/pre-analysed_root_files
fi
if [ ! -d "pre-analyse/plots" ]; then
	mkdir pre-analyse/plots
fi
make analysis
cd exec
