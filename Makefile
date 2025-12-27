CC=gcc

all: server client

COMPILE_FLAGS = -ggdb

SERVER_INCLUDES = server.c proto.c myvpn_errno.c myvpn_log.c tun.c utils.c address_pool.c \
thirdparty/cash_table.h thirdparty/bitarray.h

CLIENT_INCLUDES = client.c proto.c myvpn_errno.c myvpn_log.c tun.c utils.c thirdparty/cash_table.h thirdparty/bitarray.h 

server: $(SERVER_INCLUDES)
	$(CC) $(COMPILE_FLAGS) -o ./build/server $(SERVER_INCLUDES)


client: $(CLIENT_INCLUDES) 
	$(CC) $(COMPILE_FLAGS) -o ./build/client $(CLIENT_INCLUDES)

