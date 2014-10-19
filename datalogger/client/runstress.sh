#!/bin/sh

n=1  # Run 1 process by default
x=0

if [ $# -gt 0 ]; then
	n=$1
fi

while [ $x -lt $n ]; do
	python client.py -T stress$x,AC1000 &
	x=$(($x + 1))
done
