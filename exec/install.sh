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
if [ ! -d "pre-analysed_root_files" ]; then
	mkdir pre-analysed_root_files
fi
make 
cd exec
