CC = gcc
CFLAGS = -pg -Wall -pthread

all: lab2c

lab2c.o: lab2c.c
	$(CC) $(CFLAGS) -c -o lab2c.o lab2c.c

SortedList.o: SortedList.c
	$(CC) $(CFLAGS) -c -o SortedList.o SortedList.c

lab2c: lab2c.o SortedList.o
	$(CC) $(CFLAGS) -o lab2c lab2c.o SortedList.o 

clean:
	rm -f lab2c lab2c.o SortedList.o gmon.out

sources = Makefile README lab2c.c SortedList.c SortedList.h graph1.png 
dist:
	tar -czf lab2c-104477818.tar.gz $(sources)
