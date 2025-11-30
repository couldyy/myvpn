CC=gcc

all: server client


server: server.c
	$(CC) -o ./build/server server.c


client: client.c tun.h tun.c utils.c utils.h
	$(CC) -o ./build/client client.c tun.c utils.c

