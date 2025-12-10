CC=gcc

all: server client


server: server.c tun.h tun.c utils.c utils.h address_pool.h address_pool.c thirdparty/bitarray.h
	$(CC) -o ./build/server server.c tun.c utils.c address_pool.c


client: client.c tun.h tun.c utils.c utils.h
	$(CC) -o ./build/client client.c tun.c utils.c

