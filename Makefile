CC=gcc

all: server client


server: server.c
	$(CC) -o ./build/server server.c


client: client.c
	$(CC) -o ./build/client client.c

