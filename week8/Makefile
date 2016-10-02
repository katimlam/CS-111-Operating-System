CC = gcc
CFLAGS = -Wall -g
lab = lab3b

all: $(lab)

$(lab).o: $(lab).c
	$(CC) $(CFLAGS) -c -o $@ $<

$(lab): $(lab).o
	$(CC) $(CFLAGS) -o $@ $< 

run:  
	./$(lab)
	
clean:
	rm -f $(lab) $(lab).o

sources = Makefile README $(lab).c  
dist:
	tar -czf $(lab)-104477818.tar.gz $(sources)