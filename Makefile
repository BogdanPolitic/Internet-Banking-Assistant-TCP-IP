all: build

build: client server

client: client.c
	gcc -o client -lnsl client.c

server: server.c
	gcc -o server -lnsl server.c

clean:
	rm -f *.o *~
	rm -f client server
