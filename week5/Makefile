all: lab2b

lab2b: lab2b.c SortedList.c
	gcc -Wall -pthread lab2b.c SortedList.c -o lab2b

clean:
	rm -f lab2b

sources = Makefile README lab2b.c SortedList.c SortedList.h graph1.png graph2.png
dist:
	tar -czf lab2b-104477818.tar.gz $(sources)