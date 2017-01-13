#!/bin/bash
procs = (0,1,2,3,4)
maxes = (10,100,1000,10000,100000,1000000,10000000,100000000,1000000000)
for p in "${procs[@]}"
do
	for m in "${maxes[@]}"
	do
		./sieve1 m p
	done
done