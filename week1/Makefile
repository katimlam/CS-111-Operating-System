all: lab0

lab0: lab0.c
	gcc lab0.c -o lab0

test = --input=in.tmp --output=out.tmp 
check: lab0
	echo abcdefg > in.tmp
	echo abc > out.tmp 
	./lab0 $(test)
	diff in.tmp out.tmp || echo "Failed Check"
	rm -f in.tmp out.tmp

clean:
	rm -f lab0.o lab0 in.tmp out.tmp

dist: lab0-104477818.tar.gz
sources = Makefile lab0.c screenshot1.png screenshot2.png README
lab0-104477818.tar.gz: $(sources)
	tar -czf $@ $(sources)

	
