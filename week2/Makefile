all: lab1a

lab1a: lab1a.c
	gcc -pthread lab1a.c -o lab1a

clean:
	rm -f lab1a.o lab1a

dist: lab1a-104477818.tar.gz
sources = Makefile lab1a.c README
lab1a-104477818.tar.gz: $(sources)
	tar -czf $@ $(sources)
