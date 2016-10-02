all: server client

server: server.c
	gcc -pthread -lmcrypt server.c -o server
client: client.c
	gcc -pthread -lmcrypt client.c -o client

clean:
	rm -f client server

sources = Makefile server.c client.c my.key README
dist:
	tar -czf lab1b-104477818.tar.gz $(sources)
		