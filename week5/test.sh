#!/bin/bash
echo "" > test.txt
i=2
while [ $i -le 32 ]
do
	./lab2b --iterations=1000 --sync=m --threads=$i >> test.txt
	i=$(( i+2 ))
done

i=2
while [ $i -le 32 ]
do
	./lab2b --iterations=1000 --sync=s --threads=$i >> test.txt
	i=$(( i+2 ))
done