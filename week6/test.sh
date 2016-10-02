#!/bin/bash
#echo "" > test.txt
#i=1
#while [ $i -le 64 ]
#do
#	./lab2c --iterations=10000 --sync=m --threads=8 --lists=$i >> test_m.txt
#	i=$(( i+1 ))
#done

i=33
while [ $i -le 64 ]
do
	./lab2c --iterations=10000 --sync=s --threads=8 --lists=$i >> test_s.txt
	i=$(( i+1 ))
done