CC=gcc
CFLAGS=-Wall -Wextra -pthread

all: server client

server: server.c
	$(CC) $(CFLAGS) server.c -o server -pthread

client: client.c
	$(CC) $(CFLAGS) client.c -o client

clean:
	rm -f server client
	rm -f server.log
	rm -f client.log
