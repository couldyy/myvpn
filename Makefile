CC=gcc

all: server client


server: server.c
	$(CC) -o ./build/server server.c


client: client.c tun.h tun.c
	$(CC) -o ./build/client client.c tun.c tun.h

