CC=gcc

all: server client


server: server.c tun.h tun.c utils.c utils.h
	$(CC) -o ./build/server server.c tun.c utils.c


client: client.c tun.h tun.c utils.c utils.h
	$(CC) -o ./build/client client.c tun.c utils.c

