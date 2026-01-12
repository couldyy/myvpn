CC=gcc
SERVER_OUT=myvpn-server
CLIENT_OUT=myvpn-client

all: prepare server client

COMPILE_FLAGS = -ggdb

SERVER_INCLUDES = server.c proto.c myvpn_errno.c myvpn_log.c tun.c utils.c address_pool.c \
thirdparty/cash_table.h thirdparty/bitarray.h

CLIENT_INCLUDES = client.c proto.c myvpn_errno.c myvpn_log.c tun.c utils.c thirdparty/cash_table.h thirdparty/bitarray.h 

# create dir if not exists
prepare:
	mkdir -p ./build

server: prepare $(SERVER_INCLUDES)
	$(CC) $(COMPILE_FLAGS) -o ./build/$(SERVER_OUT) $(SERVER_INCLUDES)


client: prepare $(CLIENT_INCLUDES) 
	$(CC) $(COMPILE_FLAGS) -o ./build/$(CLIENT_OUT) $(CLIENT_INCLUDES)

clean:
	rm -rf ./build
