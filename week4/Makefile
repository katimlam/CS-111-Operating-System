all: lab2a

lab2a: lab2a.c
	gcc -pthread lab2a.c -o lab2a

clean:
	rm -f lab2a

sources = Makefile README lab2a.c graph1.png graph2.png 
dist: 
	tar -czf lab2a-104477818.tar.gz $(sources)